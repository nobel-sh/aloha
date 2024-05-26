#include "token.h"
#include <iostream>

const std::map<TokenKind, const std::string> Token::token_to_string = {
    {TokenKind::BANG, "BANG"},
    {TokenKind::COLON, "COLON"},
    {TokenKind::COMMA, "COMMA"},
    {TokenKind::EQUALEQUAL, "EQUALEQUAL"},
    {TokenKind::EQUALS, "EQUALS"},
    {TokenKind::GREATERTHAN, "GREATERTHAN"},
    {TokenKind::GREATERTHANEQUAL, "GREATERTHANEQUAL"},
    {TokenKind::LESSTHAN, "LESSTHAN"},
    {TokenKind::LESSTHANEQUAL, "LESSTHANEQUAL"},
    {TokenKind::LPAREN, "LPAREN"},
    {TokenKind::LBRACE, "LBRACE"},
    {TokenKind::MINUS, "MINUS"},
    {TokenKind::NOTEQUAL, "NOTEQUAL"},
    {TokenKind::PERCENT, "PERCENT"},
    {TokenKind::PLUS, "PLUS"},
    {TokenKind::RBRACE, "RBRACE"},
    {TokenKind::RPAREN, "RPAREN"},
    {TokenKind::SEMICOLON, "SEMICOLON"},
    {TokenKind::SLASH, "SLASH"},
    {TokenKind::STAR, "STAR"},
    {TokenKind::IDENT, "IDENT"},
    {TokenKind::INT, "INT"},
    {TokenKind::FLOAT, "FLOAT"},
    {TokenKind::STRING, "STRING"},
    {TokenKind::UNDERSCORE, "UNDERSCORE"},
    {TokenKind::EOF_TOKEN, "EOF"},
};

void Token::dump() {
  std::cout << "Token {" << std::endl;
  std::cout << "\tkind: " << this->to_string() << std::endl;
  std::cout << "\tpos: " << pos << std::endl;
  std::cout << "\tlexeme: " << lexeme << std::endl;
  std::cout << "}" << std::endl;
}

const std::string Token::to_string() {
  auto it = token_to_string.find(kind);
  assert(it != token_to_string.end());
  return it->second;
}
