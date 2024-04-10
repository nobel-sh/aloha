#ifndef TOKEN_H_
#define TOKEN_H_

#include <iostream>
#include <memory>
#include <map>

enum TokenKind
{
    BANG,
    COLON,
    COMMA,
    EQUALEQUAL,
    EQUALS,
    GREATERTHAN,
    GREATERTHANEQUAL,
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
    IDENT,
};

static const std::map<TokenKind, std::string> token_to_string = {
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
};

class Token
{
public:
  Token(unsigned int pos, TokenKind kind, std::string &&lexeme)
      : pos(pos), kind(kind), lexeme(std::move(lexeme)) {}

  void dump();
  std::string to_string(TokenKind kind);

private:
  TokenKind kind;
  unsigned int pos;
  std::string lexeme;
};

#endif // TOKEN