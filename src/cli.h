#ifndef CLI_H_
#define CLI_H_

#include "driver.h"
#include <string>

class CommandLineParser
{
public:
    static void print_help();
    static void print_version();
    static CompilerConfig parse_args(int argc, char *argv[]);
};

#endif // CLI_H_
