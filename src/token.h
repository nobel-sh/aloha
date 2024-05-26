#ifndef TOKEN_H_
#define TOKEN_H_

#include <cassert>
#include <iostream>
#include <memory>
#include <map>

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

  INT,
  FLOAT,
  STRING,
};

class Token {
private:
 static const std::map<TokenKind, const std::string> token_to_string;
public:
  Token(unsigned int pos, TokenKind kind, const std::string lexeme)
      : pos(pos), kind(kind), lexeme(std::move(lexeme)) {}

  void dump() const ;
  const std::string to_string() const;
  TokenKind kind;
  unsigned int pos;
  const std::string lexeme;
};

#endif // TOKEN
