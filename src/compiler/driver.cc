#include "driver.h"
#include "../objgen.h"
#include "../utils/reader.h"
#include "../type.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <unistd.h>
#include <linux/limits.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Program.h>

CompilerDriver::CompilerDriver(const CompilerConfig &config)
    : config(config), lexer(nullptr), parser(nullptr) {}

CompilerDriver::~CompilerDriver()
{
    delete lexer;
    delete parser;
}

void CompilerDriver::print_separator() const
{
    for (int i = 0; i < 50; ++i)
        std::cout << "-";
    std::cout << std::endl;
}

void CompilerDriver::dump_untyped_ast() const
{
    if (!config.dump_debug_info || !ast)
        return;

    print_separator();
    std::cout << "Untyped AST" << std::endl;

    // Use parser's dump method
    if (parser)
    {
        parser->dump(ast.get());
    }
    print_separator();
}

void CompilerDriver::dump_typed_ast() const
{
    if (!config.dump_debug_info || !ast)
        return;

    std::cout << "Semantic Analyzer: No semantic errors" << std::endl;
    print_separator();
    std::cout << "Typed AST" << std::endl;

    if (parser)
    {
        parser->dump(ast.get());
    }
    print_separator();
}

void CompilerDriver::dump_unoptimized_ir() const
{
    if (!config.dump_debug_info)
        return;

    std::cout << "Unoptimized LLVM IR" << std::endl;
    codegen.dump_ir();
    print_separator();
}

void CompilerDriver::dump_optimized_ir() const
{
    if (!config.dump_debug_info || !config.enable_optimization)
        return;

    std::cout << "Optimized LLVM IR" << std::endl;
    codegen.dump_ir();
}

bool CompilerDriver::optimize_code()
{
    if (!config.enable_optimization)
    {
        if (config.dump_debug_info)
        {
            std::cout << "Optimization Turned OFF" << std::endl;
        }
        return true;
    }

    try
    {
        std::cout << "Optimization Turned ON" << std::endl;

        // work with the linked module
        codegen.module = std::move(module);
        optimize(codegen);
        module = std::move(codegen.module);

        if (config.dump_debug_info)
        {
            print_separator();
            dump_optimized_ir();
        }

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: Optimization failed: " << e.what() << std::endl;
        return false;
    }
}

bool CompilerDriver::emit_object_file()
{
    try
    {
        std::string object_name = config.file_name + ".o";
        codegen.module = std::move(module);
        objgen(codegen, object_name);
        module = std::move(codegen.module);

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: Object file generation failed: " << e.what() << std::endl;
        return false;
    }
}

std::string CompilerDriver::get_stdlib_path() const
{
    if (const char *aloha_home = std::getenv("ALOHA_HOME"))
    {
        return std::string(aloha_home) + "/lib/libaloha_stdlib.a";
    }

    // try to find relative to executable
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1)
    {
        exe_path[len] = '\0';
        std::filesystem::path exe_dir = std::filesystem::path(exe_path).parent_path();
        std::filesystem::path stdlib_path = exe_dir / "libaloha_stdlib.a";
        if (std::filesystem::exists(stdlib_path))
        {
            return stdlib_path.string();
        }
    }
    // Fallback: look in build directory
    return "../build/libaloha_stdlib.a";
}

// link against libc and libaloha_stdlib.a
bool CompilerDriver::link_executable()
{
    try
    {
        std::string object_name = config.file_name + ".o";
        std::string executable_name = config.file_name + ".out";
        std::string stdlib_path = get_stdlib_path();

        std::string linker;
        const char *candidates[] = {"lld", "ld.lld", "ld"};
        for (const char *candidate : candidates)
        {
            if (auto linker_path = llvm::sys::findProgramByName(candidate))
            {
                linker = linker_path.get();
                break;
            }
        }

        if (linker.empty())
        {
            std::cerr << "ERROR: No linker found (tried: lld, ld.lld, ld)" << std::endl;
            return false;
        }

        std::cout << "Linking with: " << linker << std::endl;

        std::vector<llvm::StringRef> args = {
            linker,
            "-o",
            executable_name,
            "/usr/lib/x86_64-linux-gnu/crt1.o", // C runtime 1 startup
            "/usr/lib/x86_64-linux-gnu/crti.o", // C runtime initialization
            object_name,
            stdlib_path,
            "/usr/lib/x86_64-linux-gnu/crtn.o", // C runtime termination
            "-lc",                              // C standard library
            "-dynamic-linker",
            "/lib64/ld-linux-x86-64.so.2" // Dynamic linker for x86_64 Linux
        };

        std::string error_msg;
        int result = llvm::sys::ExecuteAndWait(
            linker,
            args,
            std::nullopt,
            {},
            0,
            0,
            &error_msg);

        if (result == 0)
        {
            std::cout << "Linking successful, executable created: " << executable_name << std::endl;
            return true;
        }
        else
        {
            std::cerr << "Linking failed with error code: " << result << std::endl;
            if (!error_msg.empty())
            {
                std::cerr << "Error: " << error_msg << std::endl;
            }
            return false;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: Linking failed: " << e.what() << std::endl;
        return false;
    }
}

int CompilerDriver::compile()
{
    print_separator();

    if (!parse_and_resolve_imports())
        return 1;

    if (!compile_modules())
        return 1;

    if (!link_modules())
        return 1;

    if (!optimize_code())
        return 1;

    if (!emit_object_file())
        return 1;

    if (!link_executable())
        return 1;

    return 0;
}

bool CompilerDriver::parse_and_resolve_imports()
{
    try
    {
        std::cout << "Parsing modules...\n";
        if (!import_resolver.process_imports(config.input_file, module_context))
        {
            return false;
        }

        std::cout << "Parsed " << module_context.get_modules().size() << " module(s)\n";
        print_separator();
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: Module parsing failed: " << e.what() << std::endl;
        return false;
    }
}

bool CompilerDriver::compile_modules()
{
    try
    {
        std::cout << "Compiling modules...\n";

        auto &modules = module_context.get_modules();

        for (auto &mod : modules)
        {
            std::cout << "  Compiling: " << mod->module_name << std::endl;

            Aloha::ExportedSymbols imported_symbols;
            for (const auto &dep_path : mod->dependencies)
            {
                auto *dep_module = module_context.get_module(dep_path);
                if (dep_module && dep_module->is_compiled)
                {
                    imported_symbols.merge(dep_module->exported_symbols);
                }
            }

            SemanticAnalyzer module_analyzer;
            module_analyzer.import_symbols(imported_symbols.functions, imported_symbols.structs);
            module_analyzer.analyze(mod->ast.get());

            module_analyzer.export_symbols(mod->exported_symbols.functions, mod->exported_symbols.structs);

            auto llvm_module = std::make_unique<llvm::Module>(mod->module_name, codegen.context);

            // temporarily set the module
            auto original_module = std::move(codegen.module);
            codegen.module = std::move(llvm_module);

            // declare all imported functions as external
            for (const auto &[fn_name, fn_info] : imported_symbols.functions)
            {
                codegen.declare_fn_external(fn_name, fn_info);
            }

            if (!codegen.generate_code(mod->ast.get()))
            {
                std::cerr << "ERROR: Code generation failed for " << mod->module_name << std::endl;
                return false;
            }

            // store the compiled module and restore the original
            mod->llvm_module = std::move(codegen.module);
            mod->is_compiled = true;
            codegen.module = std::move(original_module);
        }

        print_separator();
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: Module compilation failed: " << e.what() << std::endl;
        return false;
    }
}

bool CompilerDriver::link_modules()
{
    try
    {
        std::cout << "Linking modules...\n";

        auto &modules = module_context.get_modules();
        if (modules.empty())
        {
            std::cerr << "ERROR: No modules to link\n";
            return false;
        }

        // find the main module and the last one is the entry point
        module = std::move(modules.back()->llvm_module);

        if (!module)
        {
            std::cerr << "ERROR: Entry point module does not exist\n";
            return false;
        }

        // link all other modules in reverse dependency order
        llvm::Linker linker(*module);
        for (size_t i = 0; i < modules.size() - 1; ++i)
        {
            if (!modules[i]->llvm_module)
            {
                std::cerr << "ERROR: Module " << modules[i]->module_name << " is null\n";
                return false;
            }

            std::cout << "  Linking: " << modules[i]->module_name << std::endl;

            if (linker.linkInModule(std::move(modules[i]->llvm_module)))
            {
                std::cerr << "ERROR: Failed to link module: " << modules[i]->module_name << std::endl;
                return false;
            }
        }

        std::cout << "Successfully linked " << modules.size() << " module(s)\n";
        print_separator();

        if (config.dump_debug_info)
        {
            std::cout << "Linked LLVM IR:\n";
            codegen.module = std::move(module);
            codegen.dump_ir();
            module = std::move(codegen.module);
            print_separator();
        }

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: Module linking failed: " << e.what() << std::endl;
        return false;
    }
}
