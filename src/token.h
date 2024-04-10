#ifndef TOKEN_H_
#define TOKEN_H_

#include <assert.h>
#include <iostream>
#include <memory>
#include <unordered_map>

enum TokenKind {
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

static const std::unordered_map<TokenKind, std::string> token_to_string = {
    {BANG, "BANG"},
    {COLON, "COLON"},
    {COMMA, "COMMA"},
    {EQUALEQUAL, "EQUALEQUAL"},
    {EQUALS, "EQUALS"},
    {GREATERTHAN, "GREATERTHAN"},
    {GREATERTHANEQUAL, "GREATERTHANEQUAL"},
    {LESSTHAN, "LESSTHAN"},
    {LESSTHANEQUAL, "LESSTHANEQUAL"},
    {LPAREN, "LPAREN"},
    {LBRACE, "LBRACE"},
    {MINUS, "MINUS"},
    {NOTEQUAL, "NOTEQUAL"},
    {PERCENT, "PERCENT"},
    {PLUS, "PLUS"},
    {RBRACE, "RBRACE"},
    {RPAREN, "RPAREN"},
    {SEMICOLON, "SEMICOLON"},
    {SLASH, "SLASH"},
    {STAR, "STAR"},
    {IDENT, "IDENT"},
    {INT, "INT"},
    {FLOAT, "FLOAT"},
    {STRING, "STRING"},
    {UNDERSCORE, "UNDERSCORE"},
    {EOF_TOKEN, "EOF"},

};

class Token {
public:
  Token(unsigned int pos, TokenKind kind, std::string &&lexeme)
      : pos(pos), kind(kind), lexeme(std::move(lexeme)) {}

  void dump();
  std::string to_string();

private:
  TokenKind kind;
  unsigned int pos;
  std::string lexeme;
};

#endif // TOKEN