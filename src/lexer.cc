#include "lexer.h"
#include "location.h"
#include <string>

void Lexer::dump() {
  for (auto tok : tokens) {
    tok.dump();
  }
}

void Lexer::dump_error() {
  for (auto err : errors) {
    std::cerr << err << std::endl;
  }
}

// peek current token
char Lexer::peek_token() const { return peek_token(0); }

// peek nth token
char Lexer::peek_token(unsigned int nth) const {
  if (pos + nth >= source.size())
    return 0;
  return source[pos + nth];
}

// consume current token and move to next token
void Lexer::consume_token() { consume_token(1); }

// consume n tokens
void Lexer::consume_token(int n) {
  for (int i = 0; i < n; ++i) {
    if (pos >= source.size()) {
      return;
    }
    if (source[pos] == '\n') {
      ++line;
      col = 1;
    } else {
      ++col;
    }
    ++pos;
  }
}

bool Lexer::is_eof() const { return pos >= source.size(); }

// TODO: Remove the lexemes in unecessary cases
// TODO: Can definitely refactor this code as code is repeated
void Lexer::lex() {
  while (!is_eof()) {
    auto curr_char = peek_token();
    Location loc(line, col);
    switch (curr_char) {
    case '(':
      tokens.push_back(Token(TokenKind::LPAREN, "(", loc));
      break;
    case ')':
      tokens.push_back(Token(TokenKind::RPAREN, ")", loc));
      break;
    case ' ':
    case '\n':
    case '\t':
    case '\r':
      while (peek_token() == curr_char)
        consume_token();
      continue;

    case '{':
      tokens.push_back(Token(TokenKind::LBRACE, "{", loc));
      break;
    case '}':
      tokens.push_back(Token(TokenKind::RBRACE, "}", loc));
      break;
    case '[':
      tokens.push_back(Token(TokenKind::LBRACKET, "[", loc));
      break;
    case ']':
      tokens.push_back(Token(TokenKind::RBRACKET, "]", loc));
      break;
    case ',':
      tokens.push_back(Token(TokenKind::COMMA, ",", loc));
      break;
    case '=':
      if (peek_token(1) == '=') {
        tokens.push_back(Token(TokenKind::EQUALEQUAL, "==", loc));
        consume_token();
      } else {
        tokens.push_back(Token(TokenKind::EQUALS, "=", loc));
      }
      break;
    case '_':
      if (std::isalpha(peek_token(1))) {
        handle_ident();
        continue;
      } else {
        tokens.push_back(Token(TokenKind::UNDERSCORE, "_", loc));
      }
      break;
    case '+':
      tokens.push_back(Token(TokenKind::PLUS, "+", loc));
      break;
    case '-':
      if (peek_token(1) == '>') {
        tokens.push_back(Token(TokenKind::THIN_ARROW, "->", loc));
        consume_token();
      } else
        tokens.push_back(Token(TokenKind::MINUS, "-", loc));
      break;
    case '*':
      tokens.push_back(Token(TokenKind::STAR, "*", loc));
      break;
    case '/':
      tokens.push_back(Token(TokenKind::SLASH, "/", loc));
      break;
    case '%':
      tokens.push_back(Token(TokenKind::PERCENT, "%", loc));
      break;
    case ';':
      tokens.push_back(Token(TokenKind::SEMICOLON, ";", loc));
      break;
    case '!':
      if (peek_token(1) == '=') {
        tokens.push_back(Token(TokenKind::NOTEQUAL, "!=", loc));
        consume_token();
      } else {
        tokens.push_back(Token(TokenKind::BANG, "!", loc));
      }
      break;
    case '<':
      if (peek_token(1) == '=') {
        tokens.push_back(Token(TokenKind::LESSTHANEQUAL, "<=", loc));
        consume_token();
      } else {
        tokens.push_back(Token(TokenKind::LESSTHAN, "<", loc));
      }
      break;
    case '>':
      if (peek_token(1) == '=') {
        tokens.push_back(Token(TokenKind::GREATERTHANEQUAL, ">=", loc));
        consume_token();
      } else {
        tokens.push_back(Token(TokenKind::GREATERTHAN, ">", loc));
      }
      break;
    case ':':
      tokens.push_back(Token(TokenKind::COLON, ":", loc));
      break;
    case EOF:
      break;
    case '"':
      handle_string();
      continue;
    default:
      if (isalpha(curr_char)) {
        handle_ident();
        continue;
      } else if (isdigit(curr_char)) {
        handle_number();
        continue;
      }
      break;
    }
    consume_token();
  }
  Location loc(line, col);
  tokens.push_back(Token(TokenKind::EOF_TOKEN, "EOF", loc));
}

void Lexer::handle_string() {
  consume_token();
  char curr_char = peek_token();
  unsigned int start_pos = pos;
  Location loc(line, col);
  while (curr_char != '"') {
    curr_char = peek_token();
    if (is_eof() || line != loc.line) {
      std::string err = "Non terminated String at: " + loc.to_string();
      has_error = true;
      errors.push_back(err);
      return;
    }
    consume_token();
  }
  std::string instruction(source.begin() + start_pos, source.begin() + pos - 1);
  tokens.push_back(Token(TokenKind::STRING, std::move(instruction), loc));
}

void Lexer::handle_number() {
  unsigned int start_pos = pos;
  Location loc(line, col);
  while (isdigit(peek_token())) {
    consume_token();
  }
  if (peek_token() == '.') {
    consume_token();
    while (isdigit(peek_token())) {
      consume_token();
    }
    tokens.push_back(Token(
        TokenKind::FLOAT,
        std::string(source.begin() + start_pos, source.begin() + pos), loc));
  } else {
    tokens.push_back(Token(
        TokenKind::INT,
        std::string(source.begin() + start_pos, source.begin() + pos), loc));
  }
}

void Lexer::handle_ident() {
  unsigned int start_pos = pos;
  Location loc(line, col);
  if (isalpha(peek_token()) || peek_token() == '_') {
    consume_token();
  } else {
    return;
  }
  while (isalnum(peek_token()) || peek_token() == '_') {
    consume_token();
  }
  std::string str(source.begin() + start_pos, source.begin() + pos);
  tokens.push_back(Token(TokenKind::IDENT, std::move(str), loc));
}
