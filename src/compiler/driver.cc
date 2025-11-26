#include "driver.h"
#include "../objgen.h"
#include "../utils/reader.h"
#include "../type.h"
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <linux/limits.h>

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

bool CompilerDriver::parse_and_resolve_imports()
{
    try
    {
        // Parse main file and resolve all imports in one step
        ast = import_resolver.process_imports(config.input_file);

        if (!ast)
        {
            return false;
        }

        dump_untyped_ast();
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: Parsing and import resolution failed: " << e.what() << std::endl;
        return false;
    }
}

bool CompilerDriver::analyze_semantics()
{
    try
    {
        if (!ast)
        {
            std::cerr << "ERROR: Invalid state for semantic analysis" << std::endl;
            return false;
        }

        analyzer.analyze(ast.get());
        dump_typed_ast();
        return true;
    }
    catch (const TypeError &e)
    {
        e.print_error();
        return false;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: Semantic analysis failed: " << e.what() << std::endl;
        return false;
    }
}

bool CompilerDriver::generate_code()
{
    try
    {
        if (!ast)
        {
            std::cerr << "ERROR: Invalid state for code generation" << std::endl;
            return false;
        }

        codegen.generate_code(ast.get());
        print_separator();
        dump_unoptimized_ir();
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: Code generation failed: " << e.what() << std::endl;
        return false;
    }
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
        if (config.dump_debug_info)
        {
            std::cout << "Optimization Turned ON" << std::endl;
            optimize(codegen);
            print_separator();
            dump_optimized_ir();
        }
        else
        {
            optimize(codegen);
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
        objgen(codegen, object_name);
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: Object file generation failed: " << e.what() << std::endl;
        return false;
    }
}

bool CompilerDriver::link_executable()
{
    try
    {
        std::string object_name = config.file_name + ".o";
        std::string executable_name = config.file_name + ".out";

        // HACK: just hardcode the path to the standard library archive for now
        // TODO: fix this
        char exe_path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1); // symlink to the current executable
        if (len == -1)
        {
            std::cerr << "Error: Could not determine compiler executable path" << std::endl;
            return false;
        }
        exe_path[len] = '\0';

        std::string exe_dir(exe_path);
        size_t last_slash = exe_dir.find_last_of('/');
        if (last_slash != std::string::npos)
        {
            exe_dir = exe_dir.substr(0, last_slash);
        }

        std::string stdlib_path = exe_dir + "/libaloha_stdlib.a";
        std::string command = "clang++ " + object_name + " " + stdlib_path + " -o " + executable_name;

        int result = std::system(command.c_str());
        if (result == 0)
        {
            std::cout << "Linking successful, executable created: " << executable_name
                      << std::endl;
            return true;
        }
        else
        {
            std::cerr << "Linking failed with error code: " << result << std::endl;
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
    // Execute compilation pipeline
    if (!parse_and_resolve_imports())
        return 1;
    if (!analyze_semantics())
        return 1;
    if (!generate_code())
        return 1;
    if (!optimize_code())
        return 1;
    if (!emit_object_file())
        return 1;
    if (!link_executable())
        return 1;

    return 0;
}
