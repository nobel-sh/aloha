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
    {TokenKind::LBRACKET, "LBRACKET"},
    {TokenKind::MINUS, "MINUS"},
    {TokenKind::NOTEQUAL, "NOTEQUAL"},
    {TokenKind::PERCENT, "PERCENT"},
    {TokenKind::PLUS, "PLUS"},
    {TokenKind::RBRACE, "RBRACE"},
    {TokenKind::RPAREN, "RPAREN"},
    {TokenKind::RBRACKET, "RBRACKET"},
    {TokenKind::SEMICOLON, "SEMICOLON"},
    {TokenKind::SLASH, "SLASH"},
    {TokenKind::STAR, "STAR"},
    {TokenKind::IDENT, "IDENT"},
    {TokenKind::INT, "INT"},
    {TokenKind::FLOAT, "FLOAT"},
    {TokenKind::STRING, "STRING"},
    {TokenKind::UNDERSCORE, "UNDERSCORE"},
    {TokenKind::THIN_ARROW, "THIN ARROW"},
    {TokenKind::FAT_ARROW, "FAT ARROW"},
    {TokenKind::EOF_TOKEN, "EOF"},
};

void Token::dump() const {
  std::cout << "Token { ";
  std::cout << "Kind: " << this->to_string();
  std::cout << "\tLexeme: " << "`" << lexeme << "`";
  std::cout << "\tLocation: " << loc.to_string();
  std::cout << " }" << std::endl;
}

const std::string Token::to_string() const {
  auto it = token_to_string.find(kind);
  assert(it != token_to_string.end());
  return it->second;
}
