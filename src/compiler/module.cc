#include "module.h"

namespace Aloha
{
    void ExportedSymbols::merge(const ExportedSymbols &other)
    {
        for (const auto &[name, info] : other.functions)
        {
            // later function imports override earlier ones.
            functions[name] = info;
        }

        for (const auto &[name, info] : other.structs)
        {
            structs[name] = info;
        }
    }

    void ModuleContext::add_module(std::unique_ptr<Module> module)
    {
        modules.push_back(std::move(module));
    }

    Module *ModuleContext::get_module(const std::string &file_path)
    {
        for (auto &module : modules)
        {
            if (module->file_path == file_path)
            {
                return module.get();
            }
        }
        return nullptr;
    }

    bool ModuleContext::all_compiled() const
    {
        for (const auto &module : modules)
        {
            if (!module->is_compiled)
            {
                return false;
            }
        }
        return true;
    }

} // namespace Aloha
