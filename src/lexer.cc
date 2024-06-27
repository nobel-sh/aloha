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
      tokens.push_back(Token(loc, TokenKind::LPAREN, "("));
      break;
    case ')':
      tokens.push_back(Token(loc, TokenKind::RPAREN, ")"));
      break;
    case ' ':
    case '\n':
    case '\t':
    case '\r':
      while (peek_token() == curr_char)
        consume_token();
      continue;

    case '{':
      tokens.push_back(Token(loc, TokenKind::LBRACE, "{"));
      break;
    case '}':
      tokens.push_back(Token(loc, TokenKind::RBRACE, "}"));
      break;
    case ',':
      tokens.push_back(Token(loc, TokenKind::COMMA, ","));
      break;
    case '=':
      if (peek_token(1) == '=') {
        tokens.push_back(Token(pos, TokenKind::EQUALEQUAL, "=="));
        consume_token();
      } else {
        tokens.push_back(Token(loc, TokenKind::EQUALS, "="));
      }
      break;
    case '_':
      tokens.push_back(Token(loc, TokenKind::UNDERSCORE, "_"));
      break;
    case '+':
      tokens.push_back(Token(loc, TokenKind::PLUS, "+"));
      break;
    case '-':
      if (peek_token(1) == '>') {
        tokens.push_back(Token(loc, TokenKind::THIN_ARROW, "->"));
        consume_token();
      } else
        tokens.push_back(Token(loc, TokenKind::MINUS, "-"));
      break;
    case '*':
      tokens.push_back(Token(loc, TokenKind::STAR, "*"));
      break;
    case '/':
      tokens.push_back(Token(loc, TokenKind::SLASH, "/"));
      break;
    case '%':
      tokens.push_back(Token(loc, TokenKind::PERCENT, "%"));
      break;
    case ';':
      tokens.push_back(Token(loc, TokenKind::SEMICOLON, ";"));
      break;
    case '!':
      if (peek_token(1) == '=') {
        tokens.push_back(Token(loc, TokenKind::NOTEQUAL, "!="));
        consume_token();
      } else {
        tokens.push_back(Token(loc, TokenKind::BANG, "!"));
      }
      break;
    case '<':
      if (peek_token(1) == '=') {
        tokens.push_back(Token(pos, TokenKind::LESSTHANEQUAL, "<="));
        consume_token();
      } else {
        tokens.push_back(Token(loc, TokenKind::LESSTHAN, "<"));
      }
      break;
    case '>':
      if (peek_token(1) == '=') {
        tokens.push_back(Token(loc, TokenKind::GREATERTHANEQUAL, ">="));
        consume_token();
      } else {
        tokens.push_back(Token(loc, TokenKind::GREATERTHAN, ">"));
      }
      break;
    case ':':
      tokens.push_back(Token(loc, TokenKind::COLON, ":"));
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
  tokens.push_back(Token(loc, TokenKind::EOF_TOKEN, "EOF"));
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
  tokens.push_back(Token(loc, TokenKind::STRING, std::move(instruction)));
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
    tokens.push_back(
        Token(pos, TokenKind::FLOAT,
              std::string(source.begin() + start_pos, source.begin() + pos)));
  } else {
    tokens.push_back(
        Token(pos, TokenKind::INT,
              std::string(source.begin() + start_pos, source.begin() + pos)));
  }
}

void Lexer::handle_ident() {
  unsigned int start_pos = pos;
  Location loc(line, col);
  while (isalnum(peek_token()) || peek_token() == '_') {
    consume_token();
  }
  std::string str(source.begin() + start_pos, source.begin() + pos);
  tokens.push_back(Token(loc, TokenKind::IDENT, std::move(str)));
}
