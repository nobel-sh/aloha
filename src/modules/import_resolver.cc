#include "import_resolver.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <linux/limits.h>

namespace aloha
{

  ImportResolver::ImportResolver(AIR::TyTable &ty_table,
                                 SymbolTable &main_symbol_table,
                                 const std::string &current_file_path)
      : ty_table(ty_table),
        main_symbol_table(main_symbol_table),
        errors()
  {
    std::filesystem::path current_path(current_file_path);
    if (current_path.has_parent_path())
    {
      current_file_dir = current_path.parent_path();
    }
    else
    {
      current_file_dir = std::filesystem::current_path();
    }

    initialize_search_paths();
  }

  ImportResolver::~ImportResolver() = default;

  void ImportResolver::initialize_search_paths()
  {
    // FIXME: remove the hacky paths
    // search path priority:
    // 1. current file directory
    // 2. current working directory
    // 3. standard library directory
    // 4. ALOHA_PATH environment variable (colon-separated paths)

    search_paths.push_back(current_file_dir);
    search_paths.push_back(std::filesystem::current_path());
    auto stdlib = get_stdlib_path();
    if (!stdlib.empty() && std::filesystem::exists(stdlib))
    {
      search_paths.push_back(stdlib);
    }

    if (const char *aloha_path = std::getenv("ALOHA_PATH"))
    {
      std::stringstream ss(aloha_path);
      std::string path;
      while (std::getline(ss, path, ':'))
      {
        if (!path.empty() && std::filesystem::exists(path))
        {
          search_paths.push_back(path);
        }
      }
    }
  }

  std::filesystem::path ImportResolver::get_stdlib_path() const
  {
    if (const char *aloha_home = std::getenv("ALOHA_HOME"))
    {
      std::filesystem::path root_path = std::filesystem::path(aloha_home);
      std::filesystem::path stdlib_path = root_path / "stdlib";
      if (std::filesystem::exists(stdlib_path))
      {
        return root_path;
      }
    }

    // FIXME: very hacky for now
    // try to find stdlib relative to the compiler executable
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1)
    {
      exe_path[len] = '\0';
      std::filesystem::path exe_dir = std::filesystem::path(exe_path).parent_path();

      // try ../ (for build directory layout where exe is in build/ and stdlib is in ../stdlib)
      std::filesystem::path root_path = exe_dir.parent_path();
      std::filesystem::path stdlib_path = root_path / "stdlib";
      if (std::filesystem::exists(stdlib_path))
      {
        return root_path;
      }

      // try ./ (for installed layout where exe and stdlib are siblings)
      root_path = exe_dir;
      stdlib_path = root_path / "stdlib";
      if (std::filesystem::exists(stdlib_path))
      {
        return root_path;
      }
    }

    // fallback: check if ../stdlib exists from current directory
    std::filesystem::path root_path = std::filesystem::current_path().parent_path();
    std::filesystem::path stdlib_path = root_path / "stdlib";
    if (std::filesystem::exists(stdlib_path))
    {
      return root_path;
    }

    return "";
  }

  std::string ImportResolver::normalize_path(const std::filesystem::path &path) const
  {
    try
    {
      if (std::filesystem::exists(path))
      {
        return std::filesystem::canonical(path).string();
      }
      return std::filesystem::absolute(path).lexically_normal().string();
    }
    catch (const std::filesystem::filesystem_error &e)
    {
      return path.string();
    }
  }

  bool ImportResolver::resolve_imports(aloha::Program *ast)
  {
    if (!ast)
    {
      return false;
    }

    bool success = true;

    for (const auto &node : ast->m_nodes)
    {
      if (auto *import_node = dynamic_cast<aloha::Import *>(node.get()))
      {
        if (!resolve_import(import_node))
        {
          success = false;
        }
      }
    }

    return success;
  }

  bool ImportResolver::resolve_import(aloha::Import *import_node)
  {
    if (!import_node)
    {
      return false;
    }

    std::string import_path = import_node->m_path;
    Location import_loc = import_node->m_loc;

    std::string file_path = resolve_import_path(import_path, import_loc);
    if (file_path.empty())
    {
      errors.add_error(import_loc, "Cannot find import: '" + import_path + "'");
      return false;
    }

    std::string normalized_path = normalize_path(file_path);

    if (already_imported.count(normalized_path) > 0)
    {
      return true;
    }

    if (currently_importing.count(normalized_path) > 0)
    {
      errors.add_error(import_loc, "Circular import detected: '" + import_path + "'");
      return false;
    }

    currently_importing.insert(normalized_path);
    bool success = process_imported_file(normalized_path, import_loc);
    currently_importing.erase(normalized_path);

    if (success)
    {
      already_imported.insert(normalized_path);
      resolved_import_paths.push_back(normalized_path);
    }

    return success;
  }

  std::string ImportResolver::resolve_import_path(const std::string &import_path,
                                                  const Location &loc)
  {
    for (const auto &search_dir : search_paths)
    {
      std::filesystem::path candidate = search_dir / import_path;

      if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate))
      {
        return candidate.string();
      }
    }

    std::filesystem::path abs_path(import_path);
    if (abs_path.is_absolute() && std::filesystem::exists(abs_path))
    {
      return abs_path.string();
    }

    return "";
  }

  bool ImportResolver::process_imported_file(const std::string &file_path,
                                             const Location &import_loc)
  {
    try
    {
      std::ifstream file(file_path);
      if (!file.is_open())
      {
        errors.add_error(import_loc, "Cannot open import file: '" + file_path + "'");
        return false;
      }

      std::string source((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
      file.close();

      if (source.empty())
      {
        return true;
      }

      Lexer lexer(source);
      Parser parser(lexer);

      std::unique_ptr<aloha::Program> imported_ast = parser.parse();
      if (!imported_ast)
      {
        errors.add_error(import_loc, "Failed to parse import: '" + file_path + "'");
        return false;
      }

      ImportResolver nested_resolver(ty_table, main_symbol_table, file_path);
      if (!nested_resolver.resolve_imports(imported_ast.get()))
      {
        for (const auto &error : nested_resolver.get_errors().get_errors())
        {
          errors.add_error(import_loc, error);
        }
        return false;
      }

      // collect definitions from the imported file directly into the main symbol table
      // this ensures var_ids, func_ids, and struct_ids are consistent across all modules
      SymbolBinder imported_def_collector(ty_table);
      imported_def_collector.set_symbol_table(&main_symbol_table);

      if (!imported_def_collector.bind(imported_ast.get()))
      {
        for (const auto &error : imported_def_collector.get_errors().get_errors())
        {
          errors.add_error(import_loc, "In import '" + file_path + "': " + error);
        }
        return false;
      }

      // store the imported AST so it can be processed by later stages
      imported_asts.push_back(std::move(imported_ast));

      // merge nested imported ASTs into our list
      for (auto &nested_ast : nested_resolver.imported_asts)
      {
        imported_asts.push_back(std::move(nested_ast));
      }

      return true;
    }
    catch (const std::exception &e)
    {
      errors.add_error(import_loc, "Exception while processing import '" + file_path + "': " + std::string(e.what()));
      return false;
    }
  }

} // namespace aloha
