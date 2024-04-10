#include "token.h"

void Token::dump() {
  std::cout << "Token {" << std::endl;
  std::cout << "\tkind: " << this->to_string() << std::endl;
  std::cout << "\tpos: " << pos << std::endl;
  std::cout << "\tLexeme: " << lexeme << std::endl;
  std::cout << "}" << std::endl;
}

std::string Token::to_string() {
  auto it = token_to_string.find(kind);
  assert(it != token_to_string.end());
  return it->second;
}