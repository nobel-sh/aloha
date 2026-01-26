#include "token.h"

void Token::dump() const
{
  std::cout << "Token { Kind: " << to_string() << "\tLexeme: `" << get_lexeme()
            << "`"
            << "\tLocation: " << loc.to_string() << " }\n";
}

const std::string Token::to_string() const
{
  return token_strings[static_cast<size_t>(kind)];
}

const std::string Token::get_lexeme() const
{
  const char *lexeme_c_str;
  if (lexeme)
  {
    lexeme_c_str = lexeme->c_str();
  }
  else
  {
    lexeme_c_str = token_lexemes[static_cast<size_t>(kind)];
  }
  return std::string(lexeme_c_str);
}
