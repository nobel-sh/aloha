#ifndef IMPORT_RESOLVER_H_
#define IMPORT_RESOLVER_H_

#include "../ast/ast.h"
#include "../compiler/module.h"
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Aloha
{

    class ImportResolver
    {
    public:
        ImportResolver() = default;

        // parse the main file and all imports into modules
        bool process_imports(const std::string &main_file, ModuleContext &context);

        const std::set<std::string> &get_imported_files() const { return imported_files; }

    private:
        std::set<std::string> imported_files;

        std::string resolve_import_path(const std::string &import_path,
                                        const std::string &current_file);

        bool process_file(const std::string &file_path,
                          ModuleContext &context,
                          std::vector<std::string> &import_stack);
    };

};

#endif // IMPORT_RESOLVER_H_
