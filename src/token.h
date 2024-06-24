#ifndef TOKEN_H_
#define TOKEN_H_

#include "location.h"
#include <cassert>
#include <iostream>
#include <map>
#include <memory>

enum class TokenKind {
  BANG,
  COLON,
  COMMA,
  EQUALEQUAL,
  EQUALS,
  EOF_TOKEN, // EOF is illegal
  GREATERTHAN,
  GREATERTHANEQUAL,
  IDENT,
  LESSTHAN,
  LESSTHANEQUAL,
  LPAREN,
  LBRACE,
  MINUS,
  NOTEQUAL,
  PERCENT,
  PLUS,
  RBRACE,
  RPAREN,
  SEMICOLON,
  SLASH,
  STAR,
  UNDERSCORE,

  THIN_ARROW, // ->
  FAT_ARROW,  // =>

  INT,
  FLOAT,
  STRING,
};

class Token {
private:
  static const std::map<TokenKind, const std::string> token_to_string;
  Location loc;

public:
  TokenKind kind;
  const std::string lexeme;

public:
  Token(unsigned int pos, TokenKind kind, const std::string lexeme)
      : loc(pos, pos), kind(kind), lexeme(lexeme) {}

  Token(Location loc, TokenKind kind, const std::string lexeme)
      : loc(loc), kind(kind), lexeme(std::move(lexeme)) {}

  void dump() const;
  const std::string to_string() const;
};

#endif // TOKEN
