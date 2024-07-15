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

std::unique_ptr<aloha::Program> Parser::parse() {
  auto program = std::make_unique<aloha::Program>();
  while (!is_eof()) {
    if (match("struct")) {
      program->m_nodes.push_back(parse_struct_decl());
    } else
      program->m_nodes.push_back(parse_function());
  }
  return program;
}

std::unique_ptr<aloha::Function> Parser::parse_function() {

  consume("fun", "Expected 'fun' keyword");
  auto identifier = expect_identifier();
  consume(TokenKind::LPAREN, "Expected '(' after function name");
  auto parameters = parse_parameters();
  consume(TokenKind::RPAREN, "Expected ')' after parameters");
  consume(TokenKind::THIN_ARROW, "Expected '->' before return type");
  auto returnType = parse_type();
  consume(TokenKind::LBRACE, "Expected '{' keyword before function body");
  auto statements = parse_statements();
  return std::make_unique<aloha::Function>(
      std::move(identifier), std::move(parameters), std::move(returnType),
      std::move(statements));
}

std::vector<aloha::StructField> Parser::parse_struct_field() {
  std::vector<aloha::StructField> fields;
  while (!match(TokenKind::RBRACE) && !is_eof()) {
    auto identifier = expect_identifier();
    consume(":", "Expected ':' after field name");
    auto type = parse_type();
    aloha::StructField field(identifier->m_name, type);
    fields.push_back(field);
    if (!match(TokenKind::RBRACE)) {
      consume(",", "Expected ',' or '}' after field declaration");
    }
  }
  return fields;
}

std::unique_ptr<aloha::Expression> Parser::parse_struct_field_access() {
  auto struct_name = expect_identifier();
  consume(TokenKind::THIN_ARROW, "Expected '->' for a struct field access");
  auto field_name = expect_identifier()->m_name;
  return std::make_unique<aloha::StructFieldAccess>(std::move(struct_name),
                                                    std::move(field_name));
}

std::unique_ptr<aloha::StructDecl> Parser::parse_struct_decl() {
  consume("struct", "Expected 'struct' keyword");
  auto identifier = expect_identifier();
  consume(TokenKind::LBRACE, "Expected '{' after function name");
  auto fields = parse_struct_field();
  consume(TokenKind::RBRACE, "Expected '}' after parameters");
  return std::make_unique<aloha::StructDecl>(std::move(identifier->m_name),
                                             std::move(fields));
}

std::unique_ptr<aloha::Expression> Parser::parse_struct_instantiation() {
  auto structName = expect_identifier();
  consume(TokenKind::LBRACE, "Expected '{' after struct name");

  std::vector<aloha::ExprPtr> fieldValues;
  while (!match(TokenKind::RBRACE) && !is_eof()) {
    fieldValues.push_back(parse_expression(0));
    if (!match(TokenKind::RBRACE)) {
      consume(TokenKind::COMMA, "Expected ',' or '}' after field value");
    }
  }

  consume(TokenKind::RBRACE, "Expected '}' after struct instantiation");
  return std::make_unique<aloha::StructInstantiation>(
      std::move(structName->m_name), std::move(fieldValues));
}

std::vector<aloha::Parameter> Parser::parse_parameters() {
  std::vector<aloha::Parameter> parameters;
  while (peek() && peek()->lexeme != ")") {
    auto identifier = expect_identifier();
    consume(":", "Expected ':' after parameter name");
    auto type = parse_type();
    aloha::Parameter param(identifier->m_name, type);
    parameters.push_back(param);
    if (!match(")")) {
      consume(",", "Expected ',' or ')' after parameter declaration");
    }
  }

  return parameters;
}

std::unique_ptr<aloha::Statement> Parser::parse_statement() {
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

std::unique_ptr<aloha::Statement> Parser::parse_expression_statement() {
  auto expr = parse_expression(0);
  return std::make_unique<aloha::ExpressionStatement>(std::move(expr));
}

std::unique_ptr<aloha::StatementBlock> Parser::parse_statements() {
  std::vector<aloha::StmtPtr> stmts;
  while (peek() && !match(TokenKind::RBRACE) && !is_eof()) {
    auto stmt = parse_statement();
    stmts.push_back(std::move(stmt));
  }
  if (!is_eof()) {
    consume(TokenKind::RBRACE, "expected '}' at the end of block statement");
  }
  return std::make_unique<aloha::StatementBlock>(std::move(stmts));
}

std::unique_ptr<aloha::Statement> Parser::parse_variable_declaration() {
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
  std::unique_ptr<aloha::Expression> expression = nullptr;
  if (match("=")) {
    advance();
    if (peek()->kind == TokenKind::IDENT && next()->kind == TokenKind::LBRACE) {
      expression = parse_struct_instantiation();
    } else {
      expression = parse_expression(0);
    }
  }
  return std::make_unique<aloha::Declaration>(
      std::move(identifier->m_name), std::move(type), std::move(expression),
      std::move(is_mutable));
}

std::unique_ptr<aloha::Statement> Parser::parse_variable_assignment() {
  auto identifier = expect_identifier();
  consume("=", "Expected '=' after variable declaration");
  std::unique_ptr<aloha::Expression> expression = parse_expression(0);
  return std::make_unique<aloha::Assignment>(std::move(identifier->m_name),
                                             std::move(expression));
}

std::unique_ptr<aloha::Statement> Parser::parse_struct_field_assignment() {
  peek()->dump();
  auto struct_expr = expect_identifier();
  peek()->dump();
  consume(TokenKind::THIN_ARROW, "Expected '->' for struct field assignment");
  auto field_name = expect_identifier()->m_name;
  consume(TokenKind::EQUALS, "Expected '=' in struct field assignment");
  auto value = parse_expression(0);
  return std::make_unique<aloha::StructFieldAssignment>(
      std::move(struct_expr), std::move(field_name), std::move(value));
}

std::unique_ptr<aloha::Statement> Parser::parse_return_statement() {
  advance();
  std::unique_ptr<aloha::Expression> expression = parse_expression(0);
  return std::make_unique<aloha::ReturnStatement>(std::move(expression));
}

std::unique_ptr<aloha::Statement> Parser::parse_if_statement() {
  advance();
  auto condition = parse_expression(0);
  consume(TokenKind::LBRACE, "Expected '{' after condition");
  std::unique_ptr<aloha::StatementBlock> then_branch = parse_statements();
  std::unique_ptr<aloha::StatementBlock> else_branch = nullptr;
  if (match("else")) {
    advance();
    if (match("if")) {
      std::vector<aloha::StmtPtr> else_stmts;
      else_stmts.push_back(parse_if_statement());
      else_branch =
          std::make_unique<aloha::StatementBlock>(std::move(else_stmts));
      // else_branch = std::make_unique<aloha::StatementBlock>();
      // else_branch->statements.push_back(parse_if_statement());
    } else {
      consume(TokenKind::LBRACE, "expected '{' or 'if' after 'else' keyword");
      else_branch = parse_statements();
    }
  }
  return std::make_unique<aloha::IfStatement>(
      std::move(condition), std::move(then_branch), std::move(else_branch));
}

std::unique_ptr<aloha::Statement> Parser::parse_while_loop() {
  advance();
  auto condition = parse_expression(0);
  consume(TokenKind::LBRACE, "Expected '{' keyword after condition");
  auto body = parse_statements();
  return std::make_unique<aloha::WhileLoop>(std::move(condition),
                                            std::move(body));
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
       return std::make_unique<aloha::UnaryExpression>(
           "-", parser.parse_expression(PREC_PREFIX));
     }},
};

std::map<std::string, Parser::infix_parser_func> Parser::infix_parsers = {
    {"+",
     [](Parser &parser, auto left) {
       return std::make_unique<aloha::BinaryExpression>(
           std::move(left), "+", std::move(parser.parse_expression(PREC_SUM)));
     }},
    {"-",
     [](Parser &parser, auto left) {
       return std::make_unique<aloha::BinaryExpression>(
           std::move(left), "-", std::move(parser.parse_expression(PREC_SUM)));
     }},
    {"*",
     [](Parser &parser, auto left) {
       return std::make_unique<aloha::BinaryExpression>(
           std::move(left), "*",
           std::move(parser.parse_expression(PREC_PRODUCT)));
     }},
    {"/",
     [](Parser &parser, auto left) {
       return std::make_unique<aloha::BinaryExpression>(
           std::move(left), "/",
           std::move(parser.parse_expression(PREC_PRODUCT)));
     }},
    {"<",
     [](Parser &parser, auto left) {
       return std::make_unique<aloha::BinaryExpression>(
           std::move(left), "<",
           std::move(parser.parse_expression(PREC_COMPARISON)));
     }},
    {">",
     [](Parser &parser, auto left) {
       return std::make_unique<aloha::BinaryExpression>(
           std::move(left), ">",
           std::move(parser.parse_expression(PREC_COMPARISON)));
     }},
    {"==",
     [](Parser &parser, auto left) {
       return std::make_unique<aloha::BinaryExpression>(
           std::move(left),
           "==", std::move(parser.parse_expression(PREC_COMPARISON)));
     }},
    {"<=",
     [](Parser &parser, auto left) {
       return std::make_unique<aloha::BinaryExpression>(
           std::move(left),
           "<=", std::move(parser.parse_expression(PREC_COMPARISON)));
     }},
    {">=",
     [](Parser &parser, auto left) {
       return std::make_unique<aloha::BinaryExpression>(
           std::move(left),
           ">=", std::move(parser.parse_expression(PREC_COMPARISON)));
     }},
    {"!=",
     [](Parser &parser, auto left) {
       return std::make_unique<aloha::BinaryExpression>(
           std::move(left),
           "!=", std::move(parser.parse_expression(PREC_COMPARISON)));
     }},

};

std::unique_ptr<aloha::Expression>
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
    left = infix(*this, std::move(left));
  }
  return left;
}

// std::unique_ptr<aloha::Expression>
// Parser::parse_infix_expressions(std::unique_ptr<aloha::Expression> left,
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

std::unique_ptr<aloha::Expression> Parser::parse_primary() {
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
        return std::make_unique<aloha::Number>("null");
      else {
        auto value = token->lexeme == "true" ? true : false;
        return std::make_unique<aloha::Boolean>(value);
      }
    }
    return std::make_unique<aloha::Identifier>(token->lexeme);
  } else if (token->kind == TokenKind::INT || token->kind == TokenKind::FLOAT) {
    advance();
    return std::make_unique<aloha::Number>(token->lexeme);
  } else if (token->kind == TokenKind::STRING) {
    advance();
    return std::make_unique<aloha::String>(token->lexeme);
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

std::unique_ptr<aloha::Expression> Parser::parse_function_call() {
  auto name = expect_identifier();
  consume(TokenKind::LPAREN, "function call must be followed by `(`");
  std::vector<aloha::ExprPtr> args;
  if (match(TokenKind::RPAREN)) {
    advance();
    return std::make_unique<aloha::FunctionCall>(std::move(name),
                                                 std::move(args));
  }
  args.push_back(parse_expression(0));
  while (match(TokenKind::COMMA)) {
    advance();
    auto arg = parse_expression(0);
    args.push_back(std::move(arg));
  }
  consume(TokenKind::RPAREN, "function call must be end with `)`");
  return std::make_unique<aloha::FunctionCall>(std::move(name),
                                               std::move(args));
}

std::unique_ptr<aloha::Identifier> Parser::expect_identifier() {
  auto token = peek();
  if (token && token->kind == TokenKind::IDENT) {
    advance();
    return std::make_unique<aloha::Identifier>(token->lexeme);
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

void Parser::dump(aloha::Program *p) const { p->write(std::cout, 2); }
