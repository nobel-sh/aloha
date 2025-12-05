#ifndef MODULE_H_
#define MODULE_H_

#include "../ast/ast.h"
#include "../symbol_table.h"
#include <llvm/IR/Module.h>
#include <memory>
#include <string>
#include <vector>

namespace Aloha
{
    struct ExportedSymbols
    {
        std::unordered_map<std::string, FunctionInfo> functions;
        std::unordered_map<std::string, StructInfo> structs;

        void merge(const ExportedSymbols &other);
    };

    struct Module
    {
        std::string file_path;
        std::string module_name;
        std::unique_ptr<aloha::Program> ast;
        std::unique_ptr<llvm::Module> llvm_module;

        std::vector<std::string> dependencies;
        bool is_compiled = false;

        ExportedSymbols exported_symbols;

        Module(const std::string &path, const std::string &name)
            : file_path(path), module_name(name) {}
    };

    class ModuleContext
    {
    public:
        ModuleContext() = default;

        void add_module(std::unique_ptr<Module> module);

        Module *get_module(const std::string &file_path);

        const std::vector<std::unique_ptr<Module>> &get_modules() const { return modules; }

        bool all_compiled() const;

    private:
        std::vector<std::unique_ptr<Module>> modules;
    };

} // namespace Aloha

#endif // MODULE_H_
