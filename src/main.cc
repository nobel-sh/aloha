#include "compiler/cli.h"
#include "compiler/driver.h"
#include <iostream>

int main(int argc, char *argv[])
{
  try
  {
    CompilerConfig config = CommandLineParser::parse_args(argc, argv);

    CompilerDriver driver(config);

    return driver.compile();
  }
  catch (const std::exception &e)
  {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
    return 1;
  }
}