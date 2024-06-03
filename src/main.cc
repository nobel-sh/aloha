#include "file.h"
#include "lexer.h"
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
  // lexer.dump();
  Parser parser(lexer.tokens);
  auto p = parser.parse();
  p->write(std::cout, 2);

  try {
    SemanticAnalyzer analyzer;
    analyzer.analyze(p.get());
    std::cout << "No semantic errors" << std::endl;
  }catch(TypeError e){
    std::cerr << e.what() << std::endl;
  }
}
