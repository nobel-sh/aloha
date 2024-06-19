#include "codegen.h"
#include "file.h"
#include "lexer.h"
#include "objgen.h"
#include "parser.h"
#include "semantic_analyzer.h"
#include "type.h"
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

int main(int argc, char *argv[]) {
  try {
    if (argc < 2) {
      std::cerr << "ERROR: no input provided to the compiler." << std::endl;
      print_help();
      return 1;
    }

    std::filesystem::path filename = argv[1];
    std::optional<bool> dump_flag = std::nullopt;

    for (int i = 2; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        print_help();
        return 0;
      } else if (arg == "--dump") {
        dump_flag = true;
      } else if (arg == "--version") {
        std::cout << "Program version 1.0\n";
        return 0;
      }
    }

    auto source = AlohaReader(filename.string()).as_bytes();
    Lexer lexer(source);
    lexer.lex();

    if (lexer.has_error) {
      lexer.dump_error();
      return 1;
    }

    Parser parser(lexer.tokens);
    auto p = parser.parse();

    SemanticAnalyzer analyzer;
    analyzer.analyze(p.get());

    CodeGen codegen;
    auto status = codegen.generateCode(p.get());

    // if (!status)
    objgen(codegen, "output.o");

    if (dump_flag.value_or(false)) {
      std::cout << std::endl;
      lexer.dump();
      print_dotted_lines(50);
      std::cout << "Untyped AST" << std::endl;
      p->write(std::cout, 2);
      print_dotted_lines(50);
      std::cout << "Semantic Analyzer: No semantic errors" << std::endl;
      print_dotted_lines(50);
      std::cout << "Typed AST" << std::endl;
      print_dotted_lines(50);
      p->write(std::cout, 2);
      print_dotted_lines(50);
      codegen.dumpIR();
    }
  } catch (const TypeError &e) {
    e.print_error();
  } catch (const std::runtime_error &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
  }

  return 0;
}
