#include "compiler/driver.h"
#include <iostream>
#include <cstring>

void print_help()
{
  std::cout << "\nAloha Programming Language Compiler\n\n"
            << "Usage: aloha [filepath] [options]\n\n"
            << "Options:\n"
            << "  --help, -h          Show this help message\n"
            << "  --version           Show version information\n"
            << "  --verbose, -v       Enable verbose output\n"
            << "  --output, -o FILE   Specify output file\n"
            << "  --optimize, -O      Enable LLVM optimizations\n"
            << "  --dump-ast          Print the abstract syntax tree\n"
            << "  --dump-air          Print the AIR intermediate representation\n"
            << "  --dump-ir           Print the LLVM IR to console\n"
            << "  --emit-llvm         Write LLVM IR to .ll file\n"
            << "  --emit-object       Write object file (.o) [default: true]\n"
            << "  --no-link           Skip linking (object file only)\n\n"
            << "Examples:\n"
            << "  aloha program.alo              Compile and link program\n"
            << "  aloha program.alo -o myapp     Compile with custom output name\n"
            << "  aloha program.alo -O           Compile with optimizations\n"
            << "  aloha program.alo --dump-ir    View generated LLVM IR\n"
            << "  aloha program.alo --verbose    Show detailed compilation steps\n";
}

void print_version()
{
  std::cout << "Aloha Programming Language Compiler\n"
            << "Version: 0.1.0 (AIR-based pipeline)\n"
            << "LLVM Backend: Enabled\n";
}

int main(int argc, char *argv[])
{
  try
  {
    AlohaPipeline::CompilerOptions options;

    if (argc < 2)
    {
      std::cerr << "ERROR: no input provided to the compiler." << std::endl;
      print_help();
      return 1;
    }

    std::string first_arg = argv[1];

    // handle help/version flags as first argument
    if (first_arg == "--help" || first_arg == "-h")
    {
      print_help();
      return 0;
    }

    if (first_arg == "--version")
    {
      print_version();
      return 0;
    }

    // first argument is the input file
    options.input_file = argv[1];

    for (int i = 2; i < argc; ++i)
    {
      std::string arg = argv[i];

      if (arg == "--help" || arg == "-h")
      {
        print_help();
        return 0;
      }
      else if (arg == "--version")
      {
        print_version();
        return 0;
      }
      else if (arg == "--dump-ast")
      {
        options.dump_ast = true;
      }
      else if (arg == "--dump-air")
      {
        options.dump_air = true;
      }
      else if (arg == "--dump-ir")
      {
        options.dump_ir = true;
      }
      else if (arg == "--emit-llvm")
      {
        options.emit_llvm = true;
      }
      else if (arg == "--emit-object")
      {
        options.emit_object = true;
      }
      else if (arg == "--no-link")
      {
        options.emit_executable = false;
      }
      else if (arg == "--optimize" || arg == "-O")
      {
        options.enable_optimization = true;
      }
      else if (arg == "--output" || arg == "-o")
      {
        if (i + 1 < argc)
        {
          options.output_file = argv[++i];
        }
        else
        {
          std::cerr << "ERROR: --output requires a filename argument" << std::endl;
          return 1;
        }
      }
      else if (arg == "--verbose" || arg == "-v")
      {
        options.verbose = true;
      }
      else
      {
        std::cerr << "ERROR: unknown option: " << arg << std::endl;
        print_help();
        return 1;
      }
    }

    AlohaPipeline::CompilerDriver driver(options);
    return driver.compile();
  }
  catch (const std::exception &e)
  {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
    return 1;
  }
}