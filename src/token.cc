#include "token.h"

void Token::dump()
{
  std::cout << "Token {" << std::endl;
  std::cout << "\tkind: " << to_string(kind) << std::endl;
  std::cout << "\tpos: " << pos << std::endl;
  std::cout << "\tLexeme: " << lexeme << std::endl;
  std::cout << "}" << std::endl;
}

std::string
Token::to_string(TokenKind kind)
{
  auto it = token_to_string.find(kind);
  if (it != token_to_string.end())
  {
    return it->second;
  }
  else
  {
    std::cerr << "Unknown token given for token to string map\n";
  }
  return "Unknown Token";
}