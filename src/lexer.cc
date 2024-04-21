#include "lexer.h"

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
unsigned char Lexer::peek_token() const { return peek_token(0); }

// peek nth token
unsigned char Lexer::peek_token(int nth) const {
  if (pos + nth >= source.size())
    return EOF;
  return source[pos + nth];
}

// consume current token and move to next token
void Lexer::consume_token() { consume_token(1); }

// consume n tokens
void Lexer::consume_token(int n) {
  if (pos + n >= source.size())
    pos = source.size();
  else
    pos += n;
}

bool Lexer::is_eof() const { return pos >= source.size(); }

// TODO: Remove the lexemes in unecessary cases
// TODO: Can definitely refactor this code as code is repeated

void Lexer::lex() {
  while (!is_eof()) {
    auto curr_char = peek_token();
    switch (curr_char) {
    case '(':
      tokens.push_back(Token(pos, TokenKind::LPAREN, "("));
      break;
    case ')':
      tokens.push_back(Token(pos, TokenKind::RPAREN, ")"));
      break;
    case ' ':
    case '\n':
    case '\t':
    case '\r':
      while (peek_token() == curr_char)
        consume_token();
      continue;

    case '{':
      tokens.push_back(Token(pos, TokenKind::LBRACE, "{"));
      break;
    case '}':
      tokens.push_back(Token(pos, TokenKind::RBRACE, "}"));
      break;
    case ',':
      tokens.push_back(Token(pos, TokenKind::COMMA, ","));
      break;
    case '=':
      tokens.push_back(Token(pos, TokenKind::EQUALS, "="));
      break;
    case '_':
      tokens.push_back(Token(pos, TokenKind::UNDERSCORE, "_"));
      break;
    case '+':
      tokens.push_back(Token(pos, TokenKind::PLUS, "+"));
      break;
    case '-':
      tokens.push_back(Token(pos, TokenKind::MINUS, "-"));
      break;
    case '*':
      tokens.push_back(Token(pos, TokenKind::STAR, "*"));
      break;
    case '/':
      tokens.push_back(Token(pos, TokenKind::SLASH, "/"));
      break;
    case '%':
      tokens.push_back(Token(pos, TokenKind::PERCENT, "%"));
      break;
    case ';':
      tokens.push_back(Token(pos, TokenKind::SEMICOLON, ";"));
      break;
    case '!':
      if (peek_token(1) == '=') {
        tokens.push_back(Token(pos, TokenKind::NOTEQUAL, "!="));
        consume_token();
      } else {
        tokens.push_back(Token(pos, TokenKind::BANG, "!"));
      }
      break;
    case '<':
      if (peek_token(1) == '=') {
        tokens.push_back(Token(pos, TokenKind::LESSTHANEQUAL, "<="));
        consume_token();
      } else {
        tokens.push_back(Token(pos, TokenKind::LESSTHAN, "<"));
      }
      break;
    case '>':
      if (peek_token(1) == '=') {
        tokens.push_back(Token(pos, TokenKind::GREATERTHANEQUAL, ">="));
        consume_token();
      } else {
        tokens.push_back(Token(pos, TokenKind::GREATERTHAN, ">"));
      }
      break;
    case ':':
      tokens.push_back(Token(pos, TokenKind::COLON, ":"));
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
  tokens.push_back(Token(pos, TokenKind::EOF_TOKEN, "EOF"));
}

void Lexer::handle_string() {
  consume_token();
  char curr_char = peek_token();
  auto start_pos = pos;
  while (curr_char != '"') {
    curr_char = peek_token();
    if (is_eof()) {
      std::string err = "Non terminated String at: " + start_pos;
      has_error = true;
      errors.push_back(err);
      return;
    }
    consume_token();
  }
  std::string instruction(source.begin() + start_pos, source.begin() + pos - 1);
  tokens.push_back(Token(start_pos, TokenKind::STRING, std::move(instruction)));
}

void Lexer::handle_number() {
  auto start_pos = pos;
  while (isdigit(peek_token())) {
    consume_token();
  }
  if (peek_token() == '.') {
    consume_token();
    while (isdigit(peek_token())) {
      consume_token();
    }
    tokens.push_back(
        Token(start_pos, TokenKind::FLOAT,
              std::string(source.begin() + start_pos, source.begin() + pos)));
  } else {
    tokens.push_back(
        Token(start_pos, TokenKind::INT,
              std::string(source.begin() + start_pos, source.begin() + pos)));
  }
}

void Lexer::handle_ident() {
  auto start_pos = pos;
  while (isalnum(peek_token()) || peek_token() == '_') {
    consume_token();
  }
  std::string str(source.begin() + start_pos, source.begin() + pos);
  tokens.push_back(Token(start_pos, TokenKind::IDENT, std::move(str)));
}
