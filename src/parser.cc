#include "parser.h"
#include "ast.h"
#include "token.h"
#include "type.h"
#include <iostream>
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

std::unique_ptr<Aloha::Program> Parser::parse() {
  auto program = std::make_unique<Aloha::Program>();
  while (!is_eof()) {
    if (match("struct")) {
      program->nodes.push_back(parse_struct_decl());
    } else
      program->nodes.push_back(parse_function());
  }
  return program;
}

std::shared_ptr<Aloha::Function> Parser::parse_function() {

  consume("fun", "Expected 'fun' keyword");
  auto identifier = expect_identifier();
  consume(TokenKind::LPAREN, "Expected '(' after function name");
  auto parameters = parse_parameters();
  consume(TokenKind::RPAREN, "Expected ')' after parameters");
  consume(TokenKind::THIN_ARROW, "Expected '->' before return type");
  auto returnType = parse_type();
  consume(TokenKind::LBRACE, "Expected '{' keyword before function body");
  auto statements = parse_statements();
  return std::make_shared<Aloha::Function>(identifier, std::move(parameters),
                                           std::move(returnType),
                                           std::move(statements));
}

std::vector<Aloha::StructField> Parser::parse_struct_field() {
  std::vector<Aloha::StructField> fields;
  while (!match(TokenKind::RBRACE) && !is_eof()) {
    auto identifier = expect_identifier();
    consume(":", "Expected ':' after field name");
    auto type = parse_type();
    Aloha::StructField field(identifier->name, type);
    fields.push_back(field);
    if (!match(TokenKind::RBRACE)) {
      consume(",", "Expected ',' or '}' after field declaration");
    }
  }
  return fields;
}

std::shared_ptr<Aloha::Expression> Parser::parse_struct_field_access() {
  auto struct_name = expect_identifier();
  consume(TokenKind::THIN_ARROW, "Expected '->' for a struct field access");
  auto field_name = expect_identifier()->name;
  return std::make_shared<Aloha::StructFieldAccess>(struct_name, field_name);
}

std::shared_ptr<Aloha::StructDecl> Parser::parse_struct_decl() {

  consume("struct", "Expected 'struct' keyword");
  auto identifier = expect_identifier();
  consume(TokenKind::LBRACE, "Expected '{' after function name");
  auto fields = parse_struct_field();
  consume(TokenKind::RBRACE, "Expected '}' after parameters");
  return std::make_shared<Aloha::StructDecl>(identifier->name,
                                             std::move(fields));
}

std::shared_ptr<Aloha::Expression> Parser::parse_struct_instantiation() {
  auto structName = expect_identifier();
  consume(TokenKind::LBRACE, "Expected '{' after struct name");

  std::vector<Aloha::ExprPtr> fieldValues;
  while (!match(TokenKind::RBRACE) && !is_eof()) {
    fieldValues.push_back(parse_expression(0));
    if (!match(TokenKind::RBRACE)) {
      consume(TokenKind::COMMA, "Expected ',' or '}' after field value");
    }
  }

  consume(TokenKind::RBRACE, "Expected '}' after struct instantiation");
  return std::make_shared<Aloha::StructInstantiation>(structName->name,
                                                      std::move(fieldValues));
}

std::vector<Aloha::Parameter> Parser::parse_parameters() {
  std::vector<Aloha::Parameter> parameters;
  while (peek() && peek()->lexeme != ")") {
    auto identifier = expect_identifier();
    consume(":", "Expected ':' after parameter name");
    auto type = parse_type();
    Aloha::Parameter param(identifier->name, type);
    parameters.push_back(param);
    if (!match(")")) {
      consume(",", "Expected ',' or ')' after parameter declaration");
    }
  }

  return parameters;
}

std::shared_ptr<Aloha::Statement> Parser::parse_statement() {
  if (match("mut") || match("imut")) {
    return parse_variable_declaration();
  } else if (match("return")) {
    return parse_return_statement();
  } else if (match("if")) {
    return parse_if_statement();
  } else if (match("while")) {
    return parse_while_loop();
  } else {
    if (peek()->kind == TokenKind::IDENT) {
      if (next()->kind == TokenKind::EQUALS) {
        return parse_variable_assignment();
      } else if (next()->kind == TokenKind::LPAREN) {
        return parse_expression_statement();
      } else if (next()->kind == TokenKind::THIN_ARROW) {
        return parse_struct_field_assignment();
      } else {
        std::cout << "Unexpected keyword found" << std::endl;
        peek()->dump();
        exit(1);
      }
    }
    // TODO: better handle this error
    //  report_error();
    // if (is_eof()) {
    //   return nullptr;
    // }
    std::cout << "Unknown or unimplemented statement kind" << std::endl;
    peek()->dump();
    exit(1);
  }
}

std::shared_ptr<Aloha::Statement> Parser::parse_expression_statement() {
  auto expr = parse_expression(0);
  return std::make_shared<Aloha::ExpressionStatement>(expr);
}

std::shared_ptr<Aloha::StatementList> Parser::parse_statements() {
  std::vector<Aloha::StmtPtr> stmts;
  while (peek() && !match(TokenKind::RBRACE) && !is_eof()) {
    auto stmt = parse_statement();
    stmts.push_back(stmt);
  }
  if (!is_eof()) {
    consume(TokenKind::RBRACE, "expected '}' at the end of block statement");
  }
  return std::make_shared<Aloha::StatementList>(stmts);
}

std::shared_ptr<Aloha::Statement> Parser::parse_variable_declaration() {
  bool is_mutable;
  if (match("mut")) {
    is_mutable = true;
    advance();
  } else {
    is_mutable = false;
    consume("imut",
            "Expected 'mut' or 'imut' keyword to start variable declaration.");
  }
  auto identifier = expect_identifier();
  auto type = optional_type();
  std::shared_ptr<Aloha::Expression> expression = nullptr;
  if (match("=")) {
    advance();
    if (peek()->kind == TokenKind::IDENT && next()->kind == TokenKind::LBRACE) {
      expression = parse_struct_instantiation();
    } else {
      expression = parse_expression(0);
    }
  }
  return std::make_shared<Aloha::Declaration>(identifier->name, type,
                                              expression, is_mutable);
}

std::shared_ptr<Aloha::Statement> Parser::parse_variable_assignment() {
  auto identifier = expect_identifier();
  consume("=", "Expected '=' after variable declaration");
  std::shared_ptr<Aloha::Expression> expression = parse_expression(0);
  return std::make_shared<Aloha::Assignment>(identifier->name, expression);
}

std::shared_ptr<Aloha::Statement> Parser::parse_struct_field_assignment() {
  peek()->dump();
  auto struct_expr = expect_identifier();
  peek()->dump();
  consume(TokenKind::THIN_ARROW, "Expected '->' for struct field assignment");
  auto field_name = expect_identifier()->name;
  consume(TokenKind::EQUALS, "Expected '=' in struct field assignment");
  auto value = parse_expression(0);
  return std::make_shared<Aloha::StructFieldAssignment>(struct_expr, field_name,
                                                        value);
}

std::shared_ptr<Aloha::Statement> Parser::parse_return_statement() {
  advance();
  std::shared_ptr<Aloha::Expression> expression = parse_expression(0);
  return std::make_shared<Aloha::ReturnStatement>(expression);
}

std::shared_ptr<Aloha::Statement> Parser::parse_if_statement() {
  advance();
  auto condition = parse_expression(0);
  consume(TokenKind::LBRACE, "Expected '{' after condition");
  std::shared_ptr<Aloha::StatementList> then_branch = parse_statements();
  std::shared_ptr<Aloha::StatementList> else_branch = nullptr;
  if (match("else")) {
    advance();
    if (match("if")) {
      else_branch = std::make_shared<Aloha::StatementList>();
      else_branch->statements.push_back(parse_if_statement());
    } else {
      consume(TokenKind::LBRACE, "expected '{' or 'if' after 'else' keyword");
      else_branch = parse_statements();
    }
  }
  return std::make_shared<Aloha::IfStatement>(condition, then_branch,
                                              else_branch);
}

std::shared_ptr<Aloha::Statement> Parser::parse_while_loop() {
  advance();
  auto condition = parse_expression(0);
  consume(TokenKind::LBRACE, "Expected '{' keyword after condition");
  auto body = parse_statements();
  return std::make_shared<Aloha::WhileLoop>(condition, body);
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

std::map<std::string, Parser::prefix_parser_func> Parser::prefix_parsers = {
    {"-",
     [](Parser &parser) {
       parser.advance();
       return std::make_shared<Aloha::UnaryExpression>(
           "-", parser.parse_expression(PREC_PREFIX));
     }},
};

std::map<std::string, Parser::infix_parser_func> Parser::infix_parsers = {
    {"+",
     [](Parser &parser, auto left) {
       return std::make_shared<Aloha::BinaryExpression>(
           left, "+", parser.parse_expression(PREC_SUM));
     }},
    {"-",
     [](Parser &parser, auto left) {
       return std::make_shared<Aloha::BinaryExpression>(
           left, "-", parser.parse_expression(PREC_SUM));
     }},
    {"*",
     [](Parser &parser, auto left) {
       return std::make_shared<Aloha::BinaryExpression>(
           left, "*", parser.parse_expression(PREC_PRODUCT));
     }},
    {"/",
     [](Parser &parser, auto left) {
       return std::make_shared<Aloha::BinaryExpression>(
           left, "/", parser.parse_expression(PREC_PRODUCT));
     }},
    {"<",
     [](Parser &parser, auto left) {
       return std::make_shared<Aloha::BinaryExpression>(
           left, "<", parser.parse_expression(PREC_COMPARISON));
     }},
    {">",
     [](Parser &parser, auto left) {
       return std::make_shared<Aloha::BinaryExpression>(
           left, ">", parser.parse_expression(PREC_COMPARISON));
     }},
    {"==",
     [](Parser &parser, auto left) {
       return std::make_shared<Aloha::BinaryExpression>(
           left, "==", parser.parse_expression(PREC_COMPARISON));
     }},
    {"<=",
     [](Parser &parser, auto left) {
       return std::make_shared<Aloha::BinaryExpression>(
           left, "<=", parser.parse_expression(PREC_COMPARISON));
     }},
    {">=",
     [](Parser &parser, auto left) {
       return std::make_shared<Aloha::BinaryExpression>(
           left, ">=", parser.parse_expression(PREC_COMPARISON));
     }},
    {"!=",
     [](Parser &parser, auto left) {
       return std::make_shared<Aloha::BinaryExpression>(
           left, "!=", parser.parse_expression(PREC_COMPARISON));
     }},

};

std::shared_ptr<Aloha::Expression>
Parser::parse_expression(int parent_precedence) {
  auto left = parse_primary();
  while (!is_eof()) {
    if (!peek()) {
      break;
    }
    auto token = peek();
    // if (token->kind != TokenKind::OPERATOR) {
    //   break;
    // }
    auto op = token->lexeme;
    auto prec = precedence[op];
    if (prec <= parent_precedence) {
      break;
    }
    auto infix = infix_parsers[op];
    if (!infix) {
      break;
    }
    advance();
    left = infix(*this, left);
  }
  return left;
}

// std::shared_ptr<Aloha::Expression>
// Parser::parse_infix_expressions(std::shared_ptr<Aloha::Expression> left,
//                                 int min_precedence) {
//   while (auto token = peek()) {
//     auto lexeme = token->lexeme;
//     if (precedence.find(lexeme) == precedence.end() ||
//         precedence[lexeme] < min_precedence) {
//       break;
//     }
//     if (auto infix_parsers_iter = infix_parsers.find(lexeme);
//         infix_parsers_iter != infix_parsers.end()) {
//       advance();
//       left = infix_parsers_iter->second(*this, left);
//     } else {
//       report_error("No infix parser found for operator: " + lexeme);
//       return nullptr;
//     }
//   }
//   return left;
// }

std::shared_ptr<Aloha::Expression> Parser::parse_primary() {
  std::optional<Token> token = peek();
  if (!token) {
    report_error("Unexpected end of input");
    return nullptr;
  }
  if (token->kind == TokenKind::LPAREN) {
    advance();
    auto expression = parse_expression(0);
    consume(TokenKind::RPAREN, "Expected ')' after expression");
    return expression;
  } else if (token->kind == TokenKind::IDENT) {
    if (next()->kind == TokenKind::LPAREN) {
      return parse_function_call();
    } else if (next()->kind == TokenKind::LBRACE) {
      return parse_struct_instantiation();
    } else if (next()->kind == TokenKind::THIN_ARROW) {
      return parse_struct_field_access();
    }
    advance();
    if (is_reserved_ident(*token)) {
      if (token->lexeme == "null")
        return std::make_shared<Aloha::Number>("null");
      else {
        auto value = token->lexeme == "true" ? true : false;
        return std::make_shared<Aloha::Boolean>(value);
      }
    }
    return std::make_shared<Aloha::Identifier>(token->lexeme);
  } else if (token->kind == TokenKind::INT || token->kind == TokenKind::FLOAT) {
    advance();
    return std::make_shared<Aloha::Number>(token->lexeme);
  } else if (token->kind == TokenKind::STRING) {
    advance();
    return std::make_shared<Aloha::String>(token->lexeme);
  } else if (token->kind == TokenKind::MINUS) {
    auto prefix = prefix_parsers[token->lexeme];
    if (prefix) {
      return prefix(*this);
    }
    report_error("Unexpected expor");
    return nullptr;
  } else {
    report_error("Expected expression");
    return nullptr;
  }
}

std::shared_ptr<Aloha::Expression> Parser::parse_function_call() {
  auto name = expect_identifier();
  consume(TokenKind::LPAREN, "function call must be followed by `(`");
  std::vector<Aloha::ExprPtr> args;
  if (match(TokenKind::RPAREN)) {
    advance();
    return std::make_shared<Aloha::FunctionCall>(name, args);
  }
  args.push_back(parse_expression(0));
  while (match(TokenKind::COMMA)) {
    advance();
    auto arg = parse_expression(0);
    args.push_back(arg);
  }
  consume(TokenKind::RPAREN, "function call must be end with `)`");
  return std::make_shared<Aloha::FunctionCall>(name, args);
}

std::shared_ptr<Aloha::Identifier> Parser::expect_identifier() {
  auto token = peek();
  if (token && token->kind == TokenKind::IDENT) {
    advance();
    return std::make_shared<Aloha::Identifier>(token->lexeme);
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
  if (lexeme == "true" || lexeme == "false" || lexeme == "null") {
    return true;
  }
  return false;
}

bool Parser::is_reserved_ident() const {
  auto lexeme = peek()->lexeme;
  if (lexeme == "true" || lexeme == "false" || lexeme == "null") {
    return true;
  }
  return false;
}

void Parser::dump(Aloha::Program *p) const { p->write(std::cout, 2); }
