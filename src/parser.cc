#include "parser.h"
#include "ast.h"
#include "token.h"
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

Parser::Parser(std::vector<Token> tokens)
    : tokens(std::move(tokens)), current(0) {}

bool Parser::is_eof() const {
  return current >= tokens.size() || peek()->kind == TokenKind::EOF_TOKEN;
}

void Parser::advance() {
  if (!is_eof()) {
    ++current;
  }
}

template <typename T>
void Parser::consume(const T &value, std::string message) {
  if (match(value, false)) {
    advance();
  } else {
    std::cout << "DEBUG: Match Failed" << std::endl;
    std::cout << "DEBUG: Found:" << std::endl;
    peek()->dump();
    report_error(message);
    if (!this->get_errors().empty()) {
      for (auto x : this->get_errors()) {
        std::cerr << x << std::endl;
      }
      exit(1);
    }
  }
}

std::optional<Token> Parser::peek() const { return get_token(false); }

std::optional<Token> Parser::next() const { return get_token(true); }

std::optional<Token> Parser::get_token(bool use_next) const {
  size_t index = current + (use_next ? 1 : 0);
  if (index < tokens.size()) {
    return tokens[index];
  }
  return std::nullopt;
}

bool Parser::match(std::string value, bool use_next) {
  std::optional<Token> token = get_token(use_next);
  return token && token->lexeme == value;
}

bool Parser::match(TokenKind value, bool use_next) {
  std::optional<Token> token = get_token(use_next);
  return token && token->kind == value;
}
void Parser::report_error(const std::string &message) {
  error_collector.add_error(message);
}

const std::vector<std::string> &Parser::get_errors() const {
  return error_collector.get_errors();
}

std::unique_ptr<Program> Parser::parse() {
  std::cerr << "DEBUG: Parsing Program" << std::endl;
  auto program = std::make_unique<Program>();
  while (!is_eof()) {
    program->nodes.push_back(parse_function());
  }
  return program;
}

std::shared_ptr<Function> Parser::parse_function() {
  std::cerr << "DEBUG: Parsing Function" << std::endl;

  consume("fun", "Expected 'fun' keyword");
  auto identifier = expect_identifier();
  consume(TokenKind::LPAREN, "Expected '(' after function name");
  auto parameters = parse_parameters();
  consume(TokenKind::RPAREN, "Expected ')' after parameters");
  consume(TokenKind::THIN_ARROW, "Expected '->' before return type");
  auto returnType = parse_type();
  consume("do", "Expected 'do' keyword before function body");
  auto statements = parse_statements();
  consume("end", "Expected 'end' keyword to close function");
  return std::make_shared<Function>(identifier, std::move(parameters),
                                    std::move(returnType),
                                    std::move(statements));
}

std::vector<Parameter> Parser::parse_parameters() {
  std::cerr << "DEBUG: Parsing parameters" << std::endl;
  std::vector<Parameter> parameters;
  while (peek() && peek()->lexeme != ")") {
    std::cout << peek()->lexeme << std::endl;
    auto identifier = expect_identifier();
    consume(":", "Expected ':' after parameter name");
    auto type = parse_type();
    auto pair = std::make_pair(identifier->name, type);
    parameters.emplace_back(Parameter(pair));
    if (!match(")"))
      consume(",", "Expected ',' or ')' after parameter declaration");
    break;
  }
  return parameters;
}

std::shared_ptr<StatementList> Parser::parse_statements() {
  std::cerr << "DEBUG: Parsing statements" << std::endl;
  std::vector<StmtPtr> stmts;
  while (peek() && peek()->lexeme != "end") {

    stmts.push_back(parse_statement());
  }
  return std::make_shared<StatementList>(stmts);
}

std::shared_ptr<Statement> Parser::parse_statement() {
  std::cerr << "DEBUG: Parsing statement" << std::endl;
  if (match("var")) {
    return parse_variable_declaration();
  } else if (match("return")) {
    return parse_return_statement();
  } else if (match("if")) {
    return parse_if_statement();
  } else if (match("while")) {
    return parse_while_loop();
  } else {
    //TODO: better handle this error
    // report_error("Unknown or unimplemented statement kind");
    std::cout << "Unknown or unimplemented statement kind" << std::endl;
    exit(1);
    // return nullptr;
  }
}

std::shared_ptr<Statement> Parser::parse_variable_declaration() {
  std::cerr << "DEBUG: Parsing variable declaration" << std::endl;
  consume("var", "Expected 'var' keyword");
  auto identifier = expect_identifier();
  consume("=", "Expected '=' after variable declaration");
  auto expression = parse_expression(0);
  return std::make_shared<Assignment>(identifier->name, expression);
}

std::shared_ptr<Statement> Parser::parse_return_statement() {
  advance();
  auto expression = parse_expression(0);
  return std::make_shared<ReturnStatement>(expression);
}

std::shared_ptr<Statement> Parser::parse_if_statement() {
  advance();
  auto condition = parse_expression(0);
  consume("then", "Expected 'then' keyword after condition");
  auto thenBranch = parse_statements();
  std::shared_ptr<StatementList> elseBranch = nullptr;
  consume("end", "Expected 'end' keyword after if statement");
  if (match("else")) {
    advance();
    elseBranch = parse_statements();
    consume("end", "Expected 'end' keyword after else statement");
  }
  return std::make_shared<IfStatement>(condition, thenBranch, elseBranch);
}

std::shared_ptr<Statement> Parser::parse_while_loop() {
  advance();
  auto condition = parse_expression(0);
  consume("do", "Expected 'do' keyword after condition");
  auto body = parse_statements();
  consume("end", "Expected 'end' keyword after while loop");
  return std::make_shared<WhileLoop>(condition, body->statements);
}

enum Precedence {
  PREC_ASSIGNMENT = 1,
  PREC_CONDITIONAL,
  PREC_SUM,
  PREC_PRODUCT,
  PREC_PREFIX,
  PREC_POSTFIX,
  PREC_CALL
};

std::map<std::string, Precedence> precedence = {
    {"+", PREC_SUM},
    {"-", PREC_SUM},
    {"*", PREC_PRODUCT},
    {"/", PREC_PRODUCT},
};

std::map<std::string, Parser::PrefixParserFunc> Parser::prefixParsers = {
    {"-",
     [](Parser &parser) {
       return std::make_shared<UnaryExpression>(
           "-", parser.parse_expression(PREC_PREFIX));
     }},
};

std::map<std::string, Parser::InfixParserFunc> Parser::infixParsers = {
    {"+",
     [](Parser &parser, auto left) {
       return std::make_shared<BinaryExpression>(
           left, "+", parser.parse_expression(PREC_SUM));
     }},
    {"-",
     [](Parser &parser, auto left) {
       return std::make_shared<BinaryExpression>(
           left, "-", parser.parse_expression(PREC_SUM));
     }},
    {"*",
     [](Parser &parser, auto left) {
       return std::make_shared<BinaryExpression>(
           left, "*", parser.parse_expression(PREC_PRODUCT));
     }},
    {"/",
     [](Parser &parser, auto left) {
       return std::make_shared<BinaryExpression>(
           left, "/", parser.parse_expression(PREC_PRODUCT));
     }},
};

std::shared_ptr<Expression> Parser::parse_expression(int min_precedence) {
  auto token = next();
  if (!token) {
    report_error("Unexpected end of input");
    return nullptr;
  }
  auto lexeme = token->lexeme;

  auto prefixParserIter = prefixParsers.find(lexeme);
  if (prefixParserIter != prefixParsers.end()) {
    auto prefixParser = prefixParserIter->second;
    return parse_infix_expressions(prefixParser(*this), min_precedence);
  }

  auto left = parse_primary();
  return parse_infix_expressions(left, min_precedence);
}

std::shared_ptr<Expression>
Parser::parse_infix_expressions(std::shared_ptr<Expression> left,
                                int min_precedence) {
  while (auto token = peek()) {
    auto lexeme = token->lexeme;
    if (precedence.find(lexeme) == precedence.end() ||
        precedence[lexeme] < min_precedence)
      break;
    if (auto infixParserIter = infixParsers.find(lexeme);
        infixParserIter != infixParsers.end()) {
      advance(); // Consume the operator token
      left = infixParserIter->second(*this, left);
    } else {
      report_error("No infix parser found for operator: " + lexeme);
      return nullptr;
    }
  }
  return left;
}

std::shared_ptr<Expression> Parser::parse_primary() {
  if (auto token = peek()) {
    if (token->kind == TokenKind::LPAREN) {
      advance(); // Consume '('
      auto expression = parse_expression(PREC_ASSIGNMENT);
      consume(")", "Expected ')' after expression");
      return expression;
    } else if (token->kind == TokenKind::IDENT && token->lexeme == "null") {
      advance();
      return std::make_shared<Number>("null");
    } else if (token->kind == TokenKind::IDENT) {
      advance();
      return std::make_shared<Identifier>(token->lexeme);
    } else if (token->kind == TokenKind::INT) {
      advance();
      return std::make_shared<Number>(token->lexeme);
    }
  }

  report_error("Expected expression");
  return nullptr;
}

std::shared_ptr<Identifier> Parser::expect_identifier() {
  auto token = peek();
  if (token && token->kind == TokenKind::IDENT) {
    advance();
    return std::make_shared<Identifier>(token->lexeme);
  } else {
    report_error("Expected identifier");
    return nullptr;
  }
}

std::string Parser::parse_type() {
  auto token = peek();
  if (token && token->kind == TokenKind::IDENT) {
    advance();
    return token->lexeme;
  } else {
    report_error("Expected Identifier");
    return "Unknown";
  }
}
