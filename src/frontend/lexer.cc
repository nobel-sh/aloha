#include "lexer.h"
#include <cctype>
#include <iostream>
#include <unordered_map>
#include <sstream>

// Helper function to process escape sequences in strings
static std::string process_escape_sequences(const std::string &raw_str)
{
  std::ostringstream result;
  for (size_t i = 0; i < raw_str.size(); ++i)
  {
    if (raw_str[i] == '\\' && i + 1 < raw_str.size())
    {
      char next = raw_str[i + 1];
      switch (next)
      {
      case 'n':
        result << '\n';
        break;
      case 't':
        result << '\t';
        break;
      case '"':
        result << '"';
        break;
      case '\\':
        result << '\\';
        break;
      default:
        result << '\\' << next;
        break; // Shouldn't happen if lexer validates
      }
      ++i; // Skip the next character
    }
    else
    {
      result << raw_str[i];
    }
  }
  return result.str();
}

Lexer::Lexer(std::string_view source, std::string file_path)
    : source(source), source_file(std::move(file_path)),
      current_loc(1, 1, source_file), pos(0),
      peeked_token(TokenKind::EOF_TOKEN, Location(1, 1, source_file)),
      has_peeked(false),
      eof_token(TokenKind::EOF_TOKEN, Location(1, 1, source_file)) {}

bool Lexer::is_eof() const { return pos >= source.size(); }

char Lexer::peek_token() const { return peek_token(0); }

char Lexer::peek_token(size_t nth) const
{
  if (pos + nth >= source.size())
    return '\0';
  return source[pos + nth];
}

void Lexer::consume_token()
{
  if (!is_eof())
  {
    if (source[pos] == '\n')
    {
      ++current_loc.line;
      current_loc.col = 1;
    }
    else
    {
      ++current_loc.col;
    }
    ++pos;
    current_loc.file_path = source_file;
  }
}

void Lexer::consume_token(size_t n)
{
  for (size_t i = 0; i < n && !is_eof(); ++i)
  {
    consume_token();
  }
}

void Lexer::add_error(const std::string &message)
{
  has_errors = true;
  std::cerr << source_file << ":" << current_loc.line << ":" << current_loc.col
            << ": lexer error: " << message << "\n";
}

void Lexer::handle_single_line_comment()
{
  while (peek_token() != '\n' && !is_eof())
  {
    consume_token();
  }
}

void Lexer::handle_multi_line_comment()
{
  while (!is_eof())
  {
    if (peek_token() == '*' && peek_token(1) == '/')
    {
      consume_token(2);
      return;
    }
    consume_token();
  }
  if (is_eof())
  {
    add_error("Unterminated multi-line comment");
  }
}

Token Lexer::next_token()
{
  if (has_peeked)
  {
    has_peeked = false;
    return peeked_token;
  }
  return lex_single_token();
}

Token Lexer::peek_next_token()
{
  if (!has_peeked)
  {
    peeked_token = lex_single_token();
    has_peeked = true;
  }
  return peeked_token;
}

bool Lexer::is_at_end() const
{
  return is_eof();
}

Token Lexer::lex_single_token()
{
  static const std::unordered_map<char, TokenKind> single_char_tokens = {
      {'(', TokenKind::LEFT_PAREN}, {')', TokenKind::RIGHT_PAREN}, {'{', TokenKind::LEFT_BRACE}, {'}', TokenKind::RIGHT_BRACE}, {'[', TokenKind::LEFT_BRACKET}, {']', TokenKind::RIGHT_BRACKET}, {',', TokenKind::COMMA}, {'+', TokenKind::PLUS}, {'*', TokenKind::STAR}, {'%', TokenKind::PERCENT}, {';', TokenKind::SEMICOLON}, {':', TokenKind::COLON}};

  // skip whitespace
  while (!is_eof() && std::isspace(peek_token()))
  {
    consume_token();
  }

  if (is_eof())
  {
    eof_token.loc = current_loc;
    return eof_token;
  }

  Location token_loc = current_loc;
  char curr_char = peek_token();

  // single character tokens
  auto it = single_char_tokens.find(curr_char);
  if (it != single_char_tokens.end())
  {
    consume_token();
    return Token(it->second, token_loc);
  }

  // two-character operators
  auto make_two_char_token = [&](TokenKind double_kind, TokenKind single_kind) -> Token
  {
    consume_token(2);
    return Token(double_kind, token_loc);
  };

  auto make_single_token = [&](TokenKind kind) -> Token
  {
    consume_token();
    return Token(kind, token_loc);
  };

  // multi-character tokens and complex cases
  switch (curr_char)
  {
  case '/':
    if (peek_token(1) == '/')
    {
      handle_single_line_comment();
      return lex_single_token();
    }
    else if (peek_token(1) == '*')
    {
      consume_token(2);
      handle_multi_line_comment();
      return lex_single_token();
    }
    return make_single_token(TokenKind::SLASH);

  case '=':
    return peek_token(1) == '=' ? make_two_char_token(TokenKind::EQUAL_EQUAL, TokenKind::EQUAL)
                                : make_single_token(TokenKind::EQUAL);

  case '-':
    return peek_token(1) == '>' ? make_two_char_token(TokenKind::THIN_ARROW, TokenKind::MINUS)
                                : make_single_token(TokenKind::MINUS);

  case '!':
    return peek_token(1) == '=' ? make_two_char_token(TokenKind::NOT_EQUAL, TokenKind::BANG)
                                : make_single_token(TokenKind::BANG);

  case '<':
    return peek_token(1) == '=' ? make_two_char_token(TokenKind::LESS_EQUAL, TokenKind::LESS_THAN)
                                : make_single_token(TokenKind::LESS_THAN);

  case '>':
    return peek_token(1) == '=' ? make_two_char_token(TokenKind::GREATER_EQUAL, TokenKind::GREATER_THAN)
                                : make_single_token(TokenKind::GREATER_THAN);

  case '_':
    if (std::isalpha(peek_token(1)))
    {
      size_t start_pos = pos;
      while (std::isalnum(peek_token()) || peek_token() == '_')
      {
        consume_token();
      }
      return Token(TokenKind::IDENT, std::string(source.substr(start_pos, pos - start_pos)), token_loc);
    }
    return make_single_token(TokenKind::UNDERSCORE);

  case '"':
  {
    size_t start_pos = pos;
    consume_token();

    while (peek_token() != '"' && !is_eof())
    {
      if (peek_token() == '\n')
      {
        add_error("Unterminated string (newline in string)");
        return Token(TokenKind::EOF_TOKEN, token_loc);
      }
      if (peek_token() == '\\')
      {
        consume_token();
        if (peek_token() == 'n' || peek_token() == 't' ||
            peek_token() == '"' || peek_token() == '\\')
        {
          consume_token();
        }
        else
        {
          add_error("Invalid escape sequence: \\" + std::string(1, peek_token()));
          return Token(TokenKind::EOF_TOKEN, token_loc);
        }
      }
      else
      {
        consume_token();
      }
    }

    // If we exited the loop due to EOF instead of closing quote
    if (is_eof())
    {
      add_error("Unterminated string (unexpected end of file)");
      return Token(TokenKind::EOF_TOKEN, token_loc);
    }

    consume_token();
    std::string raw_str = std::string(source.substr(start_pos + 1, pos - start_pos - 2));
    std::string processed_str = process_escape_sequences(raw_str);
    return Token(TokenKind::STRING, processed_str, token_loc);
  }

  default:
    if (std::isalpha(curr_char))
    {
      size_t start_pos = pos;
      while (std::isalnum(peek_token()) || peek_token() == '_')
      {
        consume_token();
      }
      return Token(TokenKind::IDENT, std::string(source.substr(start_pos, pos - start_pos)), token_loc);
    }
    else if (std::isdigit(curr_char))
    {
      size_t start_pos = pos;
      bool is_float = false;

      while (std::isdigit(peek_token()))
      {
        consume_token();
      }

      if (peek_token() == '.' && std::isdigit(peek_token(1)))
      {
        is_float = true;
        consume_token();
        while (std::isdigit(peek_token()))
        {
          consume_token();
        }
      }
      return Token(is_float ? TokenKind::FLOAT : TokenKind::INT,
                   std::string(source.substr(start_pos, pos - start_pos)), token_loc);
    }
    else
    {
      add_error("Unexpected character");
      consume_token();
      return lex_single_token();
    }
  }
}
