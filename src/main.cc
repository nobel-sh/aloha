#include "codegen.h"
#include "lexer.h"
#include "objgen.h"
#include "parser.h"
#include "reader.h"
#include "semantic_analyzer.h"
#include "type.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <vector>

void print_help() {
  std::cout << "\nUsage: aloha [filepath] [options]\n"
            << "Options:\n"
            << "  --help, -h   Show this help message\n"
            << "  --version    Show program version\n"
            << "  --dump       Dump extra debug information\n";
}

void print_dotted_lines(int count, bool newline = true) {
  while (count--)
    std::cout << "-";
  if (newline)
    std::cout << std::endl;
}

void link_objects(std::string object_name, std::string executable_name) {
  std::string object_files = object_name + " stdlib.o";
  std::string command = "clang++ " + object_files + " -o " + executable_name;
  int result = std::system(command.c_str());
  if (result == 0) {
    std::cout << "Linking successful, executable created: " << executable_name
              << std::endl;
  } else {
    std::cerr << "Linking failed with error code: " << result << std::endl;
  }
}

std::vector<std::string> split_on(const std::string &str, char delimiter) {
  std::vector<std::string> parts;
  std::stringstream ss(str);
  std::string item;
  while (std::getline(ss, item, delimiter)) {
    parts.push_back(item);
  }
  return parts;
}

int main(int argc, char *argv[]) {
  try {
    if (argc < 2) {
      std::cerr << "ERROR: no input provided to the compiler." << std::endl;
      print_help();
      return 1;
    }

    std::filesystem::path input_filename = argv[1];
    std::string file_name = split_on(input_filename.string(), '.').front();
    std::optional<bool> dump_flag = std::nullopt;

    for (int i = 2; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        print_help();
        return 0;
      } else if (arg == "--dump") {
        dump_flag = true;
      } else if (arg == "--version") {
        std::cout << "Aloha version 0.1\n";
        return 0;
      }
    }

    auto source = Aloha::FileReader(input_filename.string()).as_bytes();
    Lexer lexer(source);
    lexer.lex();
    if (lexer.has_error) {
      lexer.dump_error();
      return 1;
    }
    if (dump_flag.value_or(false)) {
      std::cout << "Lexer Dump" << std::endl;
      lexer.dump();
      print_dotted_lines(50);
    }

    Parser parser(lexer.tokens);
    auto p = parser.parse();
    if (dump_flag.value_or(false)) {
      std::cout << "Untyped AST" << std::endl;
      parser.dump(p.get());
      print_dotted_lines(50);
    }

    SemanticAnalyzer analyzer;
    analyzer.analyze(p.get());
    if (dump_flag.value_or(false)) {
      std::cout << "Semantic Analyzer: No semantic errors" << std::endl;
      print_dotted_lines(50);
      std::cout << "Typed AST" << std::endl;
      parser.dump(p.get());
      print_dotted_lines(50);
    }

    CodeGen codegen;
    codegen.generate_code(p.get());

    optimize(codegen);

    // if (!status)
    auto object_name = file_name + ".o";
    if (dump_flag.value_or(false)) {
      print_dotted_lines(50);
      codegen.dump_ir();
    }

    objgen(codegen, object_name);

    auto executable_name = file_name + ".out";
    link_objects(object_name, executable_name);
  } catch (const TypeError &e) {
    e.print_error();
  } catch (const std::runtime_error &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
  }

  return 0;
}
