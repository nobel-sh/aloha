#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <cassert>
#include <vector>

class Lexer {
public:
  Lexer(std::vector<char> source)
      : source(std::move(source)), pos(0), has_error(false) {}

  void dump();
  void lex();
  std::vector<Token> tokens;

  bool has_error;
  void dump_error();

private:
  std::vector<char> source;
  int pos;

  bool is_eof() const;

  void handle_string();
  void handle_number();
  void handle_ident();

  unsigned char peek_token() const;
  unsigned char peek_token(int nth) const;
  void consume_token();
  void consume_token(int n);

  std::vector<std::string> errors;
};
#endif
