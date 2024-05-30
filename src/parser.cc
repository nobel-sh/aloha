#include "parser.h"
#include <utility>

Parser::Parser(std::vector<Token> tokens)
    : tokens(std::move(tokens)), current(0) {}

bool Parser::is_eof() const {
  return current >= tokens.size();
}

void Parser::advance() {
  if (!is_eof()) {
    ++current;
  }
}

std::optional<Token> Parser::peek() const {
  return get_token(false);
}

std::optional<Token> Parser::next() const {
  return get_token(true);
}

std::optional<Token> Parser::get_token(bool use_next) const {
  size_t index = current + (use_next ? 1 : 0);
  if (index < tokens.size()) {
    return tokens[index];
  }
  return std::nullopt;
}

template<typename T>
bool Parser::match(const T &value, bool use_next) {
  std::optional<Token> token = get_token(use_next);
  if constexpr (std::is_same_v<T, std::string>) {
    return token && token->lexeme == value;
  } else if constexpr (std::is_same_v<T, TokenKind>) {
    return token && token->kind == value;
  } else {
    report_error("Unsupported type for match");
    return false;
  }
}

void Parser::report_error(const std::string& message) {
  error_collector.add_error(message);
}

const std::vector<std::string>& Parser::get_errors() const {
  return error_collector.get_errors();
}
