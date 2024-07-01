#ifndef LEXER_H
#define LEXER_H

#include "location.h"
#include "token.h"
#include <cassert>
#include <vector>

class Lexer {
public:
  Lexer(std::vector<char> source)
      : has_error(false), source(std::move(source)), line(1), col(1), pos(0) {}
  void lex();
  void dump_error();
  void dump();

public:
  std::vector<std::string> errors;
  std::vector<Token> tokens;
  bool has_error;

private:
  std::vector<char> source;
  unsigned int line;
  unsigned int col;
  unsigned int pos;

private:
  bool is_eof() const;
  void handle_string();
  void handle_number();
  void handle_ident();
  char peek_token() const;
  char peek_token(unsigned int nth) const;
  void consume_token();
  void consume_token(int n);
};

#endif
