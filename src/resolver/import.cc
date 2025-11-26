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

        // try relative to current file directory first
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

    bool ImportResolver::process_file(aloha::Program *program,
                                      const std::string &current_file,
                                      std::vector<std::string> &import_stack)
    {
        std::vector<std::string> imports_to_process;
        std::vector<aloha::NodePtr> non_import_nodes;

        for (auto &node : program->m_nodes)
        {
            if (auto *import = dynamic_cast<aloha::Import *>(node.get()))
            {
                imports_to_process.push_back(import->m_path);
            }
            else
            {
                non_import_nodes.push_back(std::move(node));
            }
        }

        std::vector<aloha::NodePtr> all_nodes;

        for (const auto &import_path : imports_to_process)
        {
            std::string resolved_path = resolve_import_path(import_path, current_file);

            if (resolved_path.empty())
            {
                std::cerr << "ERROR: Could not find import: " << import_path
                          << "\n  imported from: " << current_file << std::endl;
                return false;
            }

            // Cycle detection
            if (std::find(import_stack.begin(), import_stack.end(), resolved_path) != import_stack.end())
            {
                std::cerr << "ERROR: Circular import detected:\n";
                for (const auto &f : import_stack)
                {
                    std::cerr << "  " << f << " ->\n";
                }
                std::cerr << "  " << resolved_path << std::endl;
                return false;
            }

            if (imported_files.count(resolved_path) > 0)
            {
                continue;
            }

            imported_files.insert(resolved_path);
            import_stack.push_back(resolved_path);

            SrcReader reader(resolved_path);
            std::string import_source = reader.as_string();
            std::string_view import_view(import_source);

            Lexer import_lexer(import_view);
            Parser import_parser(import_lexer);
            auto import_ast = import_parser.parse();

            if (import_lexer.has_error())
            {
                std::cerr << "ERROR: Lexer errors in imported file: " << resolved_path
                          << "\n  imported from: " << current_file << std::endl;
                import_lexer.dump_errors();
                import_stack.pop_back();
                return false;
            }

            // recursively process imports in the imported file
            if (!process_file(import_ast.get(), resolved_path, import_stack))
            {
                import_stack.pop_back();
                return false;
            }

            for (auto &node : import_ast->m_nodes)
            {
                all_nodes.push_back(std::move(node));
            }

            import_stack.pop_back();
        }

        for (auto &node : non_import_nodes)
        {
            all_nodes.push_back(std::move(node));
        }

        program->m_nodes = std::move(all_nodes);

        return true;
    }

    std::unique_ptr<aloha::Program> ImportResolver::process_imports(const std::string &main_file)
    {
        try
        {
            // Parse the main file first
            std::string normalized_main = std::filesystem::canonical(main_file).string();

            SrcReader reader(normalized_main);
            std::string main_source = reader.as_string();
            std::string_view main_view(main_source);

            Lexer lexer(main_view);
            Parser parser(lexer);
            auto ast = parser.parse();

            if (lexer.has_error())
            {
                std::cerr << "ERROR: Lexer errors in main file: " << normalized_main << std::endl;
                lexer.dump_errors();
                return nullptr;
            }

            std::vector<std::string> import_stack;
            import_stack.push_back(normalized_main);
            imported_files.insert(normalized_main);

            if (!process_file(ast.get(), normalized_main, import_stack))
            {
                return nullptr;
            }

            return ast;
        }
        catch (const std::exception &e)
        {
            std::cerr << "ERROR: Import processing failed: " << e.what() << std::endl;
            return nullptr;
        }
    }

}
