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

public:
  TokenKind kind;
  const std::string lexeme;
  Location loc;

public:
  Token(TokenKind kind, const std::string lexeme, unsigned int pos)
      : kind(kind), lexeme(std::move(lexeme)), loc(pos, pos) {}

  Token(TokenKind kind, const std::string lexeme, Location loc)
      : kind(kind), lexeme(std::move(lexeme)), loc(loc) {}

  void dump() const;
  const std::string to_string() const;
};

#endif // TOKEN
