#include "lexer.h"
#include <iostream>
#include <vector>
#include "parser.h"
#include "file.h"

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
}
