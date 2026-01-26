#ifndef TOKEN_H_
#define TOKEN_H_

#include "location.h"
#include <array>
#include <iostream>
#include <optional>
#include <string>

#define TOKEN_KINDS      \
  X(BANG, "!")           \
  X(COLON, ":")          \
  X(COMMA, ",")          \
  X(EQUAL_EQUAL, "==")   \
  X(EQUAL, "=")          \
  X(EOF_TOKEN, "EOF")    \
  X(GREATER_THAN, ">")   \
  X(GREATER_EQUAL, ">=") \
  X(IDENT, "")           \
  X(LESS_THAN, "<")      \
  X(LESS_EQUAL, "<=")    \
  X(LEFT_PAREN, "(")     \
  X(LEFT_BRACE, "{")     \
  X(LEFT_BRACKET, "[")   \
  X(MINUS, "-")          \
  X(NOT_EQUAL, "!=")     \
  X(PERCENT, "%")        \
  X(PLUS, "+")           \
  X(RIGHT_BRACE, "}")    \
  X(RIGHT_PAREN, ")")    \
  X(RIGHT_BRACKET, "]")  \
  X(SEMICOLON, ";")      \
  X(SLASH, "/")          \
  X(STAR, "*")           \
  X(UNDERSCORE, "_")     \
  X(THIN_ARROW, "->")    \
  X(FAT_ARROW, "=>")     \
  X(INT, "")             \
  X(FLOAT, "")           \
  X(STRING, "")

enum class TokenKind
{
#define X(kind, _) kind,
  TOKEN_KINDS
#undef X
};

class Token
{
public:
  TokenKind kind;
  std::optional<std::string> lexeme;
  Location loc;

  Token(TokenKind kind, Location loc)
      : kind(kind), lexeme(std::nullopt), loc(loc) {}

  Token(TokenKind kind, std::string lexeme, Location loc)
      : kind(kind), lexeme(std::move(lexeme)), loc(loc) {}

  void dump() const;
  const std::string to_string() const;
  const std::string get_lexeme() const;

private:
  static constexpr std::array<const char *,
                              static_cast<size_t>(TokenKind::STRING) + 1>
      token_strings = {

#define X(kind, str) #kind,
          TOKEN_KINDS
#undef X

  };

  static constexpr std::array<const char *,
                              static_cast<size_t>(TokenKind::STRING) + 1>
      token_lexemes = {
#define X(kind, str) str,
          TOKEN_KINDS
#undef X
  };
};

#endif // TOKEN_H_
