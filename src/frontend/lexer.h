#ifndef LEXER_H_
#define LEXER_H_

#include "location.h"
#include "token.h"
#include <string>
#include <string_view>
#include <vector>

class Lexer
{
public:
  explicit Lexer(std::string_view source);
  void dump_errors() const;

  const std::vector<std::string> &get_errors() const { return errors; }
  bool has_error() const { return !errors.empty(); }

  Token next_token();
  Token peek_next_token();
  bool is_at_end() const;

private:
  std::string_view source;
  std::vector<std::string> errors;
  Location current_loc;
  size_t pos;

  Token peeked_token;
  bool has_peeked;
  Token eof_token;

  bool is_eof() const;
  char peek_token() const;
  char peek_token(size_t nth) const;
  void consume_token();
  void consume_token(size_t n);
  void add_error(const std::string &message);
  void handle_single_line_comment();
  void handle_multi_line_comment();

  Token lex_single_token();
};

#endif // LEXER_H_
