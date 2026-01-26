#include "import_resolver.h"
#include "../utils/paths.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace aloha
{

  ImportResolver::ImportResolver(AIR::TyTable &ty_table,
                                 SymbolTable &main_symbol_table,
                                 aloha::TySpecArena &type_arena,
                                 aloha::DiagnosticEngine &diag,
                                 const std::string &current_file_path,
                                 bool skip_prelude_injection)
      : ty_table(ty_table),
        main_symbol_table(main_symbol_table),
        type_arena(type_arena),
        diagnostics(diag),
        skip_prelude_injection(skip_prelude_injection),
        currently_importing(new std::unordered_set<std::string>()),
        already_imported(new std::unordered_set<std::string>()),
        owns_import_sets(true)
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

  ImportResolver::~ImportResolver()
  {
    if (owns_import_sets)
    {
      delete currently_importing;
      delete already_imported;
    }
  }

  void ImportResolver::initialize_search_paths()
  {

    // add files relative to the file being compiled
    search_paths.push_back(current_file_dir);

    // add standard library directory
    auto stdlib = get_stdlib_path();
    if (!stdlib.empty() && std::filesystem::exists(stdlib))
    {
      search_paths.push_back(stdlib);
    }
  }

  std::filesystem::path ImportResolver::get_stdlib_path() const
  {
    return aloha::utils::get_aloha_root();
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

  bool ImportResolver::inject_prelude()
  {
    std::string prelude_path = "stdlib/prelude.alo";
    Location prelude_loc;

    std::string file_path = resolve_import_path(prelude_path, prelude_loc);
    if (file_path.empty())
    {
      diagnostics.error(DiagnosticPhase::SymbolBinding, prelude_loc, "Cannot find prelude: '" + prelude_path + "'");
      return false;
    }

    std::string normalized_path = normalize_path(file_path);

    if (already_imported->count(normalized_path) > 0)
    {
      return true;
    }

    if (currently_importing->count(normalized_path) > 0)
    {
      diagnostics.error(DiagnosticPhase::SymbolBinding, prelude_loc, "Circular import detected in prelude: '" + prelude_path + "'");
      return false;
    }

    // mark as importing and process
    currently_importing->insert(normalized_path);
    bool success = process_imported_file(normalized_path, prelude_loc);
    currently_importing->erase(normalized_path);

    if (success)
    {
      already_imported->insert(normalized_path);
      resolved_import_paths.push_back(normalized_path);
    }

    return success;
  }

  bool ImportResolver::resolve_imports(aloha::Program *ast)
  {
    if (!ast)
    {
      return false;
    }

    // inject prelude for top-level program
    if (!skip_prelude_injection)
    {
      if (!inject_prelude())
      {
        return false;
      }
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
      diagnostics.error(DiagnosticPhase::SymbolBinding, import_loc, "Cannot find import: '" + import_path + "'");
      return false;
    }

    std::string normalized_path = normalize_path(file_path);

    if (already_imported->count(normalized_path) > 0)
    {
      return true;
    }

    if (currently_importing->count(normalized_path) > 0)
    {
      diagnostics.error(DiagnosticPhase::SymbolBinding, import_loc, "Circular import detected: '" + import_path + "'");
      return false;
    }

    currently_importing->insert(normalized_path);
    bool success = process_imported_file(normalized_path, import_loc);
    currently_importing->erase(normalized_path);

    if (success)
    {
      already_imported->insert(normalized_path);
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
        diagnostics.error(DiagnosticPhase::SymbolBinding, import_loc, "Cannot open import file: '" + file_path + "'");
        return false;
      }

      std::string source((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
      file.close();

      if (source.empty())
      {
        return true;
      }

      Lexer lexer(source, file_path);
      Parser parser(lexer, type_arena, diagnostics);

      std::unique_ptr<aloha::Program> imported_ast = parser.parse();
      if (!imported_ast || diagnostics.has_errors())
      {
        diagnostics.error(DiagnosticPhase::SymbolBinding, import_loc, "Failed to parse import: '" + file_path + "'");
        return false;
      }

      ImportResolver nested_resolver(ty_table, main_symbol_table, type_arena, diagnostics, file_path, true);

      // share import tracking sets with nested resolver
      nested_resolver.currently_importing = this->currently_importing;
      nested_resolver.already_imported = this->already_imported;
      nested_resolver.owns_import_sets = false;

      if (!nested_resolver.resolve_imports(imported_ast.get()))
      {
        return false;
      }

      // collect definitions from the imported file directly into the main symbol table
      SymbolBinder imported_def_collector(ty_table, diagnostics);
      imported_def_collector.set_symbol_table(&main_symbol_table);

      if (!imported_def_collector.bind(imported_ast.get(), type_arena))
      {
        // Errors already reported to diagnostics
        return false;
      }

      imported_asts.push_back(std::move(imported_ast));
      for (auto &nested_ast : nested_resolver.imported_asts)
      {
        if (nested_ast)
          imported_asts.push_back(std::move(nested_ast));
      }
      nested_resolver.imported_asts.clear();

      for (auto &path : nested_resolver.resolved_import_paths)
      {
        resolved_import_paths.push_back(std::move(path));
      }
      nested_resolver.resolved_import_paths.clear();

      return true;
    }
    catch (const std::exception &e)
    {
      diagnostics.error(DiagnosticPhase::SymbolBinding, import_loc, "Exception while processing import '" + file_path + "': " + std::string(e.what()));
      return false;
    }
  }

} // namespace aloha
