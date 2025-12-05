#include "import.h"
#include "../lexer.h"
#include "../parser.h"
#include "../utils/reader.h"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <unistd.h>
#include <linux/limits.h>

namespace Aloha
{

    std::string ImportResolver::resolve_import_path(const std::string &import_path,
                                                    const std::string &current_file)
    {
        // absolute path for consistency
        auto normalize_path = [](const std::filesystem::path &p) -> std::string
        {
            return std::filesystem::canonical(p).string();
        };

        // First try relative to current file directory
        std::filesystem::path current_dir = std::filesystem::path(current_file).parent_path();
        std::filesystem::path resolved = current_dir / import_path;

        if (std::filesystem::exists(resolved))
        {
            return normalize_path(resolved);
        }

        // relative to compiler executable for stdlib. FIXME: this is hacky
        char exe_path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (len != -1)
        {
            exe_path[len] = '\0';
            std::filesystem::path exe_dir = std::filesystem::path(exe_path).parent_path().parent_path();
            resolved = exe_dir / import_path;

            if (std::filesystem::exists(resolved))
            {
                return normalize_path(resolved);
            }
        }

        return "";
    }

    bool ImportResolver::process_file(const std::string &file_path,
                                      ModuleContext &context,
                                      std::vector<std::string> &import_stack)
    {
        std::string normalized_path = std::filesystem::canonical(file_path).string();

        // check if this module was already processed
        if (context.get_module(normalized_path) != nullptr)
        {
            return true;
        }

        SrcReader reader(normalized_path);
        std::string source = reader.as_string();
        std::string_view source_view(source);

        Lexer lexer(source_view);
        Parser parser(lexer);
        auto ast = parser.parse();

        if (lexer.has_error())
        {
            std::cerr << "ERROR: Lexer errors in: " << normalized_path << std::endl;
            lexer.dump_errors();
            return false;
        }

        std::string module_name = std::filesystem::path(normalized_path).stem().string();
        auto mod = std::make_unique<Module>(normalized_path, module_name);
        mod->ast = std::move(ast);

        // extract import nodes
        for (auto &node : mod->ast->m_nodes)
        {
            if (auto *import_node = dynamic_cast<aloha::Import *>(node.get()))
            {
                std::string resolved_import = resolve_import_path(import_node->m_path, normalized_path);

                if (resolved_import.empty())
                {
                    std::cerr << "ERROR: Could not resolve import: " << import_node->m_path << std::endl;
                    return false;
                }

                // cycle detection
                if (std::find(import_stack.begin(), import_stack.end(), resolved_import) != import_stack.end())
                {
                    std::cerr << "ERROR: Circular import detected:\n";
                    for (const auto &f : import_stack)
                    {
                        std::cerr << "  " << f << " ->\n";
                    }
                    std::cerr << "  " << resolved_import << std::endl;
                    return false;
                }

                mod->dependencies.push_back(resolved_import);

                // recursively process import
                import_stack.push_back(resolved_import);
                if (!process_file(resolved_import, context, import_stack))
                {
                    return false;
                }
                import_stack.pop_back();
            }
        }

        context.add_module(std::move(mod));
        imported_files.insert(normalized_path);
        return true;
    }

    bool ImportResolver::process_imports(const std::string &main_file, ModuleContext &context)
    {
        try
        {
            imported_files.clear();
            std::vector<std::string> import_stack;
            return process_file(main_file, context, import_stack);
        }
        catch (const std::exception &e)
        {
            std::cerr << "ERROR: Import processing failed: " << e.what() << std::endl;
            return false;
        }
    }

}
