#ifndef LEXER_H_
#define LEXER_H_

#include "location.h"
#include "token.h"
#include <string>
#include <string_view>
#include <vector>

class Lexer {
public:
  explicit Lexer(std::string_view source);
  void lex();
  void dump_errors() const;
  void dump_tokens() const;

  const std::vector<std::string> &get_errors() const { return errors; }
  const std::vector<Token> &get_tokens() const { return tokens; }
  bool has_error() const { return !errors.empty(); }
  size_t tokens_count() const;

private:
  std::string_view source;
  std::vector<std::string> errors;
  std::vector<Token> tokens;
  Location current_loc;
  size_t pos;

  bool is_eof() const;
  void handle_string();
  void handle_number();
  void handle_ident();
  char peek_token() const;
  char peek_token(size_t nth) const;
  void consume_token();
  void consume_token(size_t n);
  void add_token(TokenKind kind);
  void add_token(TokenKind kind, std::string_view lexeme);
  void add_error(const std::string &message);
};

#endif // LEXER_H_
