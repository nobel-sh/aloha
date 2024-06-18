#include "codegen.h"
#include "file.h"
#include "lexer.h"
#include "objgen.h"
#include "parser.h"
#include "semantic_analyzer.h"
#include "type.h"
#include <iostream>
#include <vector>

int main() {
  auto source = AlohaReader("test").as_bytes();
  Lexer lexer(source);
  lexer.lex();
  if (lexer.has_error) {
    lexer.dump_error();
    exit(1);
  }
  lexer.dump();
  Parser parser(lexer.tokens);
  auto p = parser.parse();
  p->write(std::cout, 2);

  try {
    SemanticAnalyzer analyzer;
    analyzer.analyze(p.get());
    std::cout << "No semantic errors" << std::endl;
    std::cout << "------------------" << std::endl;
    std::cout << "Typed AST" << std::endl;
    p->write(std::cout, 2);
    std::cout << "------------------" << std::endl;
    CodeGen codegen;
    auto status = codegen.generateCode(p.get());
    if (!status)
      objgen(codegen, "output.o");
  } catch (TypeError e) {
    e.print_error();
  }
}
