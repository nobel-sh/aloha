#include "lexer.h"
#include <iostream>
#include <vector>

int main() {
  std::string test = "x = 10 + (20 * 30);";
  std::vector<char> source(test.begin(), test.end());
  Lexer lexer(source);
  lexer.lex();
  if (lexer.has_error) {
    lexer.dump_error();
    exit(1);
  }
  lexer.dump();
}
