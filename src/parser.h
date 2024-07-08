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
  using prefix_parser_func =
      std::function<std::shared_ptr<Aloha::Expression>(Parser &)>;
  using infix_parser_func = std::function<std::shared_ptr<Aloha::Expression>(
      Parser &, std::shared_ptr<Aloha::Expression>)>;

  explicit Parser(std::vector<Token> tokens);

  std::unique_ptr<Aloha::Program> parse();
  void dump(Aloha::Program *p) const;
  const std::vector<std::string> &get_errors() const;

  // exposed for testing
  std::shared_ptr<Aloha::StatementList> parse_statements();
  std::shared_ptr<Aloha::Statement> parse_statement();
  std::shared_ptr<Aloha::Expression> parse_expression(int min_precedence);

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

  std::shared_ptr<Aloha::Identifier> expect_identifier();
  AlohaType::Type parse_type();
  bool is_reserved_ident() const;
  bool is_reserved_ident(Token t) const;

  std::shared_ptr<Aloha::Function> parse_function();
  std::vector<Aloha::Parameter> parse_parameters();
  std::shared_ptr<Aloha::StructDecl> parse_struct_decl();
  std::vector<Aloha::StructField> parse_struct_field();
  std::shared_ptr<Aloha::Expression> parse_struct_field_access();
  std::shared_ptr<Aloha::Statement> parse_struct_field_assignment();
  std::shared_ptr<Aloha::Expression> parse_struct_instantiation();
  std::shared_ptr<Aloha::Statement> parse_variable_declaration();
  std::shared_ptr<Aloha::Statement> parse_variable_assignment();
  std::shared_ptr<Aloha::Statement> parse_return_statement();
  std::shared_ptr<Aloha::Statement> parse_if_statement();
  std::shared_ptr<Aloha::Statement> parse_while_loop();

  std::shared_ptr<Aloha::Expression>
  parse_infix_expressions(std::shared_ptr<Aloha::Expression> left,
                          int min_precedence);
  std::shared_ptr<Aloha::Expression> parse_primary();
  std::shared_ptr<Aloha::Expression> parse_function_call();
  std::shared_ptr<Aloha::Statement> parse_expression_statement();
};

#endif // PARSER_H_
