#ifndef PARSER_H_
#define PARSER_H_

#include "ast.h"
#include "error/parser_error.h"
#include "token.h"
#include "type.h"
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <vector>

class Parser {
public:
  using PrefixParserFunc = std::function<std::shared_ptr<Expression>(Parser &)>;
  using InfixParserFunc = std::function<std::shared_ptr<Expression>(
      Parser &, std::shared_ptr<Expression>)>;

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
  // template <typename T>
  // [[nodiscard]] bool match(const T &value, bool use_next = false);
  [[nodiscard]] bool match(std::string value, bool use_next = false);
  [[nodiscard]] bool match(TokenKind value, bool use_next = false);
  template <typename T> void consume(const T &value, std::string message);
  [[nodiscard]] std::optional<Token> get_token(bool use_next) const;
  [[nodiscard]] std::optional<AlohaType::Type> optional_type();

  std::shared_ptr<Identifier> expect_identifier();
  AlohaType::Type parse_type();

  std::shared_ptr<Function> parse_function();
  std::vector<Parameter> parse_parameters();
  std::shared_ptr<StatementList> parse_statements();
  std::shared_ptr<Statement> parse_statement();
  std::shared_ptr<Statement> parse_variable_declaration();
  std::shared_ptr<Statement> parse_return_statement();
  std::shared_ptr<Statement> parse_if_statement();
  std::shared_ptr<Statement> parse_while_loop();

  std::shared_ptr<Expression> parse_expression(int min_precedence);
  std::shared_ptr<Expression>
  parse_infix_expressions(std::shared_ptr<Expression> left, int min_precedence);
  std::shared_ptr<Expression> parse_primary();

  static std::map<std::string, PrefixParserFunc> prefixParsers;
  static std::map<std::string, InfixParserFunc> infixParsers;
};

#endif // PARSER_H_
