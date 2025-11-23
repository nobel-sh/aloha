#include "cli.h"
#include <filesystem>
#include <iostream>
#include <sstream>

void CommandLineParser::print_help()
{
    std::cout << "\nUsage: aloha [filepath] [options]\n"
              << "Options:\n"
              << "  --help, -h        Show this help message\n"
              << "  --dump            Dump extra debug information\n"
              << "  --no-optimize     Turn off optimization\n";
}

void CommandLineParser::print_version()
{
    std::cout << "Aloha Programming Language Compiler\n";
}

static std::vector<std::string> split_on(const std::string &str, char delimiter)
{
    std::vector<std::string> parts;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delimiter))
    {
        parts.push_back(item);
    }
    return parts;
}

CompilerConfig CommandLineParser::parse_args(int argc, char *argv[])
{
    CompilerConfig config;

    if (argc < 2)
    {
        std::cerr << "ERROR: no input provided to the compiler." << std::endl;
        print_help();
        std::exit(1);
    }

    std::string first_arg = argv[1];
    if (first_arg == "--help" || first_arg == "-h")
    {
        print_help();
        std::exit(0);
    }
    if (first_arg == "--version" || first_arg == "-v")
    {
        print_version();
        std::exit(0);
    }

    std::filesystem::path input_filename = argv[1];
    config.input_file = input_filename.string();
    config.file_name = split_on(config.input_file, '.').front();

    // Parse remaining options
    for (int i = 2; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h")
        {
            print_help();
            std::exit(0);
        }
        else if (arg == "--version")
        {
            print_version();
            std::exit(0);
        }
        else if (arg == "--dump")
        {
            config.dump_debug_info = true;
        }
        else if (arg == "--no-optimize")
        {
            config.enable_optimization = false;
        }
        else
        {
            std::cerr << "ERROR: unknown option: " << arg << std::endl;
            print_help();
            std::exit(1);
        }
    }

    return config;
}
