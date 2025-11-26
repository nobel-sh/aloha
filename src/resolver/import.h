#ifndef IMPORT_RESOLVER_H_
#define IMPORT_RESOLVER_H_

#include "../ast/ast.h"
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

        // parse the main file and resolve all imports, returning complete AST
        std::unique_ptr<aloha::Program> process_imports(const std::string &main_file);

        const std::set<std::string> &get_imported_files() const { return imported_files; }

    private:
        std::set<std::string> imported_files;

        std::string resolve_import_path(const std::string &import_path,
                                        const std::string &current_file);

        bool process_file(aloha::Program *program,
                          const std::string &current_file,
                          std::vector<std::string> &import_stack);
    };

};

#endif // IMPORT_RESOLVER_H_
