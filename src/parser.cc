#include "parser.h"
#include "ast.h"
#include "token.h"
#include "type.h"
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
  if (!is_eof() && next()) {
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
  return std::make_shared<Function>(identifier, std::move(parameters),
                                    std::move(returnType),
                                    std::move(statements));
}

std::vector<Parameter> Parser::parse_parameters() {
  std::cerr << "DEBUG: Parsing parameters" << std::endl;
  std::vector<Parameter> parameters;
  while (peek() && peek()->lexeme != ")") {
    auto identifier = expect_identifier();
    consume(":", "Expected ':' after parameter name");
    auto type = parse_type();
    Parameter param(identifier->name, type);
    parameters.push_back(param);
    if (!match(")"))
      consume(",", "Expected ',' or ')' after parameter declaration");
  }

  return parameters;
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
    // TODO: better handle this error
    //  report_error();
    if (is_eof()) {
      return nullptr;
    }
    std::cout << "Unknown or unimplemented statement kind" << std::endl;
    peek()->dump();
    exit(1);
  }
}

std::shared_ptr<StatementList> Parser::parse_statements() {
  std::cerr << "DEBUG: Parsing statements" << std::endl;
  std::vector<StmtPtr> stmts;
  while (peek() && peek()->lexeme != "end" && !is_eof()) {
    auto stmt = parse_statement();
    stmts.push_back(stmt);
  }
  if (!is_eof()) {
    consume("end", "expected 'end' at the end of block statement");
  }
  return std::make_shared<StatementList>(stmts);
}

std::shared_ptr<Statement> Parser::parse_variable_declaration() {
  std::cerr << "DEBUG: Parsing variable declaration" << std::endl;
  consume("var", "Expected 'var' keyword");
  auto identifier = expect_identifier();
  auto type = optional_type();
  consume("=", "Expected '=' after variable declaration");
  auto expression = parse_expression(0);
  return std::make_shared<Declaration>(identifier->name, type, expression);
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
  if (match("else")) {
    advance();
    if (!match("then") && !match("if")) {
      peek()->dump();
      report_error("expected 'then' or 'if' after else block");
    }
    if (match("then")) {
      advance();
    }
    elseBranch = parse_statements();
  }
  return std::make_shared<IfStatement>(condition, thenBranch, elseBranch);
}

std::shared_ptr<Statement> Parser::parse_while_loop() {
  advance();
  auto condition = parse_expression(0);
  consume("do", "Expected 'do' keyword after condition");
  auto body = parse_statements();
  return std::make_shared<WhileLoop>(condition, body);
}

enum Precedence {
  PREC_ASSIGNMENT = 1,
  PREC_CONDITIONAL,
  PREC_SUM,
  PREC_PRODUCT,
  PREC_PREFIX,
  PREC_COMPARISON,
  PREC_POSTFIX,
  PREC_CALL
};

std::map<std::string, Precedence> precedence = {
    {"+", PREC_SUM},         {"-", PREC_SUM},         {"*", PREC_PRODUCT},
    {"/", PREC_PRODUCT},     {"<", PREC_COMPARISON},  {">", PREC_COMPARISON},
    {"==", PREC_COMPARISON}, {"<=", PREC_COMPARISON}, {">=", PREC_COMPARISON},
    {"!=", PREC_COMPARISON},
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
    {"<",
     [](Parser &parser, auto left) {
       return std::make_shared<BinaryExpression>(
           left, "<", parser.parse_expression(PREC_COMPARISON));
     }},
    {">",
     [](Parser &parser, auto left) {
       return std::make_shared<BinaryExpression>(
           left, ">", parser.parse_expression(PREC_COMPARISON));
     }},
    {"==",
     [](Parser &parser, auto left) {
       return std::make_shared<BinaryExpression>(
           left, "==", parser.parse_expression(PREC_COMPARISON));
     }},
    {"<=",
     [](Parser &parser, auto left) {
       return std::make_shared<BinaryExpression>(
           left, "<=", parser.parse_expression(PREC_COMPARISON));
     }},
    {">=",
     [](Parser &parser, auto left) {
       return std::make_shared<BinaryExpression>(
           left, ">=", parser.parse_expression(PREC_COMPARISON));
     }},
    {"!=",
     [](Parser &parser, auto left) {
       return std::make_shared<BinaryExpression>(
           left, "!=", parser.parse_expression(PREC_COMPARISON));
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
      advance();
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
      advance();
      auto expression = parse_expression(PREC_ASSIGNMENT);
      consume(")", "Expected ')' after expression");
      return expression;
    } else if (token->kind == TokenKind::IDENT) {
      if (next()->kind == TokenKind::LPAREN) {
        return parse_function_call();
      }
      advance();
      if (is_reserved_ident(*token)) {
        if (token->lexeme == "null")
          return std::make_shared<Number>("null");
        else {
          auto value = token->lexeme == "true" ? true : false;
          return std::make_shared<Boolean>(value);
        }
      }
      return std::make_shared<Identifier>(token->lexeme);
    } else if (token->kind == TokenKind::INT) {
      advance();
      return std::make_shared<Number>(token->lexeme);
    } else if (token->kind == TokenKind::FLOAT) {
      advance();
      return std::make_shared<Number>(token->lexeme);
    }
  }
  report_error("Expected expression");
  return nullptr;
}

std::shared_ptr<Expression> Parser::parse_function_call() {
  auto name = expect_identifier();
  consume(TokenKind::LPAREN, "function call must be followed by `(`");
  std::vector<ExprPtr> args;
  if (match(TokenKind::RPAREN)) {
    advance();
    return std::make_shared<FunctionCall>(name, args);
  }
  args.push_back(parse_expression(0));
  while (match(TokenKind::COMMA)) {
    advance();
    auto arg = parse_expression(0);
    args.push_back(arg);
  }
  consume(TokenKind::RPAREN, "function call must be end with `)`");
  return std::make_shared<FunctionCall>(name, args);
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

AlohaType::Type Parser::parse_type() {
  auto token = peek();
  if (token && token->kind == TokenKind::IDENT) {
    advance();
    return AlohaType::from_string(token->lexeme);
  } else {
    report_error("Expected type");
    return AlohaType::from_string("UNKNOWN");
  }
}

std::optional<AlohaType::Type> Parser::optional_type() {
  auto token = peek();
  if (token && token->kind == TokenKind::COLON) {
    advance();
    return parse_type();
  }
  return std::nullopt;
}

bool Parser::is_reserved_ident(Token t) const {
  auto lexeme = t.lexeme;
  if (lexeme == "true" || lexeme == "false" || lexeme == "null")
    return true;
  return false;
}

bool Parser::is_reserved_ident() const {
  auto lexeme = peek()->lexeme;
  if (lexeme == "true" || lexeme == "false" || lexeme == "null")
    return true;
  return false;
}
