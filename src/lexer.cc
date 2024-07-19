#include "lexer.h"
#include <cctype>
#include <iostream>
#include <unordered_map>

Lexer::Lexer(std::string_view source)
    : source(source), current_loc(1, 1), pos(0) {}

void Lexer::dump_errors() const {
  for (const auto &err : errors) {
    std::cerr << err << '\n';
  }
}

void Lexer::dump_tokens() const {
  for (const auto &tok : tokens) {
    tok.dump();
  }
}

size_t Lexer::tokens_count() const { return tokens.size(); }

bool Lexer::is_eof() const { return pos >= source.size(); }

char Lexer::peek_token() const { return peek_token(0); }

char Lexer::peek_token(size_t nth) const {
  if (pos + nth >= source.size())
    return '\0';
  return source[pos + nth];
}

void Lexer::consume_token() {
  if (!is_eof()) {
    if (source[pos] == '\n') {
      ++current_loc.line;
      current_loc.col = 1;
    } else {
      ++current_loc.col;
    }
    ++pos;
  }
}

void Lexer::consume_token(size_t n) {
  for (size_t i = 0; i < n && !is_eof(); ++i) {
    consume_token();
  }
}

void Lexer::add_token(TokenKind kind) {
  tokens.emplace_back(kind, current_loc);
}

void Lexer::add_token(TokenKind kind, std::string_view lexeme) {
  tokens.emplace_back(kind, std::string(lexeme), current_loc);
}

void Lexer::add_error(const std::string &message) {
  errors.push_back(message + " at " + current_loc.to_string());
}

void Lexer::lex() {
  static const std::unordered_map<char, TokenKind> single_char_tokens = {
      {'(', TokenKind::LEFT_PAREN},   {')', TokenKind::RIGHT_PAREN},
      {'{', TokenKind::LEFT_BRACE},   {'}', TokenKind::RIGHT_BRACE},
      {'[', TokenKind::LEFT_BRACKET}, {']', TokenKind::RIGHT_BRACKET},
      {',', TokenKind::COMMA},        {'+', TokenKind::PLUS},
      {'*', TokenKind::STAR},         {'/', TokenKind::SLASH},
      {'%', TokenKind::PERCENT},      {';', TokenKind::SEMICOLON},
      {':', TokenKind::COLON}};

  while (!is_eof()) {
    char curr_char = peek_token();

    if (std::isspace(curr_char)) {
      consume_token();
      continue;
    }

    auto it = single_char_tokens.find(curr_char);
    if (it != single_char_tokens.end()) {
      add_token(it->second);
      consume_token();
      continue;
    }

    switch (curr_char) {
    case '=':
      if (peek_token(1) == '=') {
        add_token(TokenKind::EQUAL_EQUAL);
        consume_token(2);
      } else {
        add_token(TokenKind::EQUAL);
        consume_token();
      }
      break;
    case '_':
      if (std::isalpha(peek_token(1))) {
        handle_ident();
      } else {
        add_token(TokenKind::UNDERSCORE);
        consume_token();
      }
      break;
    case '-':
      if (peek_token(1) == '>') {
        add_token(TokenKind::THIN_ARROW);
        consume_token(2);
      } else {
        add_token(TokenKind::MINUS);
        consume_token();
      }
      break;
    case '!':
      if (peek_token(1) == '=') {
        add_token(TokenKind::NOT_EQUAL);
        consume_token(2);
      } else {
        add_token(TokenKind::BANG);
        consume_token();
      }
      break;
    case '<':
      if (peek_token(1) == '=') {
        add_token(TokenKind::LESS_EQUAL);
        consume_token(2);
      } else {
        add_token(TokenKind::LESS_THAN);
        consume_token();
      }
      break;
    case '>':
      if (peek_token(1) == '=') {
        add_token(TokenKind::GREATER_EQUAL);
        consume_token(2);
      } else {
        add_token(TokenKind::GREATER_THAN);
        consume_token();
      }
      break;
    case '"':
      handle_string();
      break;
    default:
      if (std::isalpha(curr_char)) {
        handle_ident();
      } else if (std::isdigit(curr_char)) {
        handle_number();
      } else {
        add_error("Unexpected character");
        consume_token();
      }
      break;
    }
  }

  add_token(TokenKind::EOF_TOKEN);
}

void Lexer::handle_string() {
  size_t start_pos = pos;
  Location start_loc = current_loc;
  consume_token();

  while (peek_token() != '"' && !is_eof()) {
    if (peek_token() == '\n') {
      add_error("Unterminated string");
      return;
    }
    if (peek_token() == '\\') {
      consume_token();
      if (peek_token() == 'n') {
        consume_token();
      } else if (peek_token() == 't') {
        consume_token();
      } else if (peek_token() == '"' || peek_token() == '\\') {
        consume_token();
      } else {
        add_error("Invalid escape sequence: \\" + std::string(1, peek_token()));
        return;
      }
    } else {
      consume_token();
    }
  }

  if (is_eof()) {
    add_error("Unterminated string");
    return;
  }

  consume_token();
  add_token(TokenKind::STRING,
            source.substr(start_pos + 1, pos - start_pos - 2));
}

void Lexer::handle_number() {
  size_t start_pos = pos;
  bool is_float = false;

  while (std::isdigit(peek_token())) {
    consume_token();
  }

  if (peek_token() == '.' && std::isdigit(peek_token(1))) {
    is_float = true;
    consume_token();
    while (std::isdigit(peek_token())) {
      consume_token();
    }
  }
  add_token(is_float ? TokenKind::FLOAT : TokenKind::INT,
            source.substr(start_pos, pos - start_pos));
}

void Lexer::handle_ident() {
  size_t start_pos = pos;
  while (std::isalnum(peek_token()) || peek_token() == '_') {
    consume_token();
  }
  add_token(TokenKind::IDENT, source.substr(start_pos, pos - start_pos));
}
