#ifndef PARSER_H_
#define PARSER_H_

#include "ast.h"
#include "error/parser_error.h"
#include "token.h"
#include <memory>
#include <optional>
#include <vector>

class Parser {
public:
  explicit Parser(std::vector<Token> tokens);

  std::unique_ptr<Program> parse();
  const std::vector<std::string> &get_errors() const;

private:
  const std::vector<Token> tokens;
  size_t current;

  ErrorCollector error_collector;
  void report_error(const std::string &message);

  [[nodiscard]] bool is_eof() const;
  void advance();
  [[nodiscard]] std::optional<Token> peek() const;
  [[nodiscard]] std::optional<Token> next() const;
  template <typename T>
  [[nodiscard]] bool match(const T &value, bool use_next = false);
  [[nodiscard]] std::optional<Token> get_token(bool use_next) const;


};

#endif // PARSER_H_
