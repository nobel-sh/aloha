#ifndef PARSER_H_
#define PARSER_H_

#include "ast.h"
#include "error/parser_error.h"
#include "lexer.h"
#include "token.h"
#include "type.h"
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <vector>

class Parser {
public:
  using prefix_parser_func =
      std::function<std::unique_ptr<aloha::Expression>(Parser &)>;
  using infix_parser_func = std::function<std::unique_ptr<aloha::Expression>(
      Parser &, std::unique_ptr<aloha::Expression>)>;

  explicit Parser(const std::vector<Token> &tokens);

  std::unique_ptr<aloha::Program> parse();
  void dump(aloha::Program *p) const;
  const std::vector<std::string> &get_errors() const;

  // exposed for testing
  std::unique_ptr<aloha::StatementBlock> parse_statements();
  std::unique_ptr<aloha::Statement> parse_statement();
  std::unique_ptr<aloha::Expression> parse_expression(int min_precedence);

private:
  const std::vector<Token> tokens;
  size_t current;
  ErrorCollector error_collector;
  void report_error(const std::string &message);
  static std::map<std::string, prefix_parser_func> prefix_parsers;
  static std::map<std::string, infix_parser_func> infix_parsers;

private:
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

  void panic_parser(const std::string &message);

  std::unique_ptr<aloha::Identifier> expect_identifier();
  AlohaType::Type parse_type();
  bool is_reserved_ident() const;
  bool is_reserved_ident(Token t) const;

  std::unique_ptr<aloha::Function> parse_function();
  std::vector<aloha::Parameter> parse_parameters();
  std::unique_ptr<aloha::StructDecl> parse_struct_decl();
  std::vector<aloha::StructField> parse_struct_field();
  std::unique_ptr<aloha::Expression> parse_struct_field_access();
  std::unique_ptr<aloha::Statement> parse_struct_field_assignment();
  std::unique_ptr<aloha::Expression> parse_struct_instantiation();
  std::unique_ptr<aloha::Statement> parse_variable_declaration();
  std::unique_ptr<aloha::Statement> parse_variable_assignment();
  std::unique_ptr<aloha::Statement> parse_return_statement();
  std::unique_ptr<aloha::Statement> parse_if_statement();
  std::unique_ptr<aloha::Statement> parse_while_loop();

  std::unique_ptr<aloha::Expression>
  parse_infix_expressions(std::unique_ptr<aloha::Expression> left,
                          int min_precedence);
  std::unique_ptr<aloha::Expression> parse_primary();
  std::unique_ptr<aloha::Expression> parse_function_call();
  std::unique_ptr<aloha::Statement> parse_expression_statement();
};

#endif // PARSER_H_
