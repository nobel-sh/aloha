#include "parser.h"
#include "ast/ast.h"
#include "token.h"
#include "type.h"
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

Parser::Parser(Lexer &lexer)
    : lexer(&lexer),
      current_token(TokenKind::EOF_TOKEN, Location(1, 1)),
      next_token(TokenKind::EOF_TOKEN, Location(1, 1))
{
  // Initialize with first two tokens
  current_token = lexer.next_token();
  next_token = lexer.next_token();
}

bool Parser::is_eof() const
{
  return current_token.kind == TokenKind::EOF_TOKEN;
}

void Parser::panic_parser(const std::string &message)
{
  std::cout << message << std::endl;
  peek()->dump();
  exit(1);
}

Location Parser::current_location() const { return peek()->loc; }

void Parser::advance()
{
  if (!is_eof())
  {
    current_token = next_token;
    next_token = lexer->next_token();
  }
}

template <typename T>
void Parser::consume(const T &value, std::string message)
{
  if (match(value, false))
  {
    advance();
  }
  else
  {
    peek()->dump();
    report_error(message);
    if (!this->get_errors().empty())
    {
      for (auto x : this->get_errors())
      {
        std::cerr << x << std::endl;
      }
      exit(1);
    }
  }
}

std::optional<Token> Parser::peek() const { return get_token(false); }

std::optional<Token> Parser::next() const { return get_token(true); }

std::optional<Token> Parser::get_token(bool use_next) const
{
  return use_next ? next_token : current_token;
}

bool Parser::match(std::string value, bool use_next)
{
  std::optional<Token> token = get_token(use_next);
  return token && token->lexeme == value;
}

bool Parser::match(TokenKind value, bool use_next)
{
  std::optional<Token> token = get_token(use_next);
  return token && token->kind == value;
}
void Parser::report_error(const std::string &message)
{
  const std::string loc_message = message + " at :" + peek()->loc.to_string();
  error_collector.add_error(loc_message);
}

const std::vector<std::string> &Parser::get_errors() const
{
  return error_collector.get_errors();
}

std::unique_ptr<aloha::Program> Parser::parse()
{
  auto program = std::make_unique<aloha::Program>(current_location());
  while (!is_eof())
  {
    if (match("struct"))
    {
      program->m_nodes.push_back(parse_struct_decl());
    }
    else
      program->m_nodes.push_back(parse_function());
  }
  return program;
}

std::unique_ptr<aloha::Function> Parser::parse_function()
{
  Location loc = current_location();
  consume("fun", "Expected 'fun' keyword");
  auto identifier = expect_identifier();
  consume(TokenKind::LEFT_PAREN, "Expected '(' after function name");
  auto parameters = parse_parameters();
  consume(TokenKind::RIGHT_PAREN, "Expected ')' after parameters");
  consume(TokenKind::THIN_ARROW, "Expected '->' before return type");
  auto return_type = parse_type();
  consume(TokenKind::LEFT_BRACE, "Expected '{' keyword before function body");
  auto statements = parse_statements();
  return std::make_unique<aloha::Function>(
      loc, std::move(identifier), std::move(parameters), std::move(return_type),
      std::move(statements));
}

std::vector<aloha::StructField> Parser::parse_struct_field()
{
  std::vector<aloha::StructField> fields;
  while (!match(TokenKind::RIGHT_BRACE) && !is_eof())
  {
    auto identifier = expect_identifier();
    consume(TokenKind::COLON, "Expected ':' after field name");
    auto type = parse_type();
    aloha::StructField field(identifier->m_name, type);
    fields.push_back(field);
    if (!match(TokenKind::RIGHT_BRACE))
    {
      consume(TokenKind::COMMA, "Expected ',' or '}' after field declaration");
    }
  }
  return fields;
}

std::unique_ptr<aloha::Expression> Parser::parse_struct_field_access()
{
  Location loc = current_location();
  auto struct_name = expect_identifier();
  consume(TokenKind::THIN_ARROW, "Expected '->' for a struct field access");
  auto field_name = expect_identifier()->m_name;
  return std::make_unique<aloha::StructFieldAccess>(loc, std::move(struct_name),
                                                    std::move(field_name));
}

std::unique_ptr<aloha::Statement> Parser::parse_struct_decl()
{
  Location loc = current_location();
  consume("struct", "Expected 'struct' keyword");
  auto identifier = expect_identifier();
  consume(TokenKind::LEFT_BRACE, "Expected '{' after function name");
  auto fields = parse_struct_field();
  consume(TokenKind::RIGHT_BRACE, "Expected '}' after parameters");
  return std::make_unique<aloha::StructDecl>(loc, std::move(identifier->m_name),
                                             std::move(fields));
}

std::unique_ptr<aloha::Expression> Parser::parse_struct_instantiation()
{
  Location loc = current_location();
  auto struct_ident = expect_identifier();
  consume(TokenKind::LEFT_BRACE, "Expected '{' after struct name");

  std::vector<aloha::ExprPtr> field_values;
  while (!match(TokenKind::RIGHT_BRACE) && !is_eof())
  {
    field_values.push_back(parse_expression(0));
    if (!match(TokenKind::RIGHT_BRACE))
    {
      consume(TokenKind::COMMA, "Expected ',' or '}' after field value");
    }
  }

  consume(TokenKind::RIGHT_BRACE, "Expected '}' after struct instantiation");
  return std::make_unique<aloha::StructInstantiation>(
      loc, std::move(struct_ident->m_name), std::move(field_values));
}

std::vector<aloha::Parameter> Parser::parse_parameters()
{
  std::vector<aloha::Parameter> parameters;
  while (peek() && !match(TokenKind::RIGHT_PAREN))
  {
    auto identifier = expect_identifier();
    consume(TokenKind::COLON, "Expected ':' after parameter name");
    auto type = parse_type();
    aloha::Parameter param(identifier->m_name, type);
    parameters.push_back(param);
    if (!match(TokenKind::RIGHT_PAREN))
    {
      consume(TokenKind::COMMA,
              "Expected ',' or ')' after parameter declaration");
    }
  }

  return parameters;
}

std::unique_ptr<aloha::Statement> Parser::parse_statement()
{
  if (match("mut") || match("imut"))
  {
    return parse_variable_declaration();
  }
  if (match("return"))
  {
    return parse_return_statement();
  }
  if (match("if"))
  {
    return parse_if_statement();
  }
  if (match("while"))
  {
    return parse_while_loop();
  }

  if (match(TokenKind::IDENT))
  {
    if (match(TokenKind::EQUAL, true))
    {
      return parse_variable_assignment();
    }
    if (match(TokenKind::LEFT_PAREN, true))
    {
      return parse_expression_statement();
    }
    if (match(TokenKind::THIN_ARROW, true))
    {
      return parse_struct_field_assignment();
    }
  }
  return nullptr;
}

std::unique_ptr<aloha::Statement> Parser::parse_expression_statement()
{
  Location loc = current_location();
  auto expr = parse_expression(0);
  return std::make_unique<aloha::ExpressionStatement>(loc, std::move(expr));
}

std::unique_ptr<aloha::StatementBlock> Parser::parse_statements()
{
  Location loc = current_location();
  auto statements = std::make_unique<aloha::StatementBlock>(loc);
  while (!match(TokenKind::RIGHT_BRACE) && !is_eof())
  {
    auto stmt = parse_statement();
    if (!stmt)
    {
      panic_parser("Unknown or unimplemented statement kind");
    }
    statements->m_statements.push_back(std::move(stmt));
  }
  if (!is_eof())
  {
    consume(TokenKind::RIGHT_BRACE,
            "expected '}' at the end of block statement");
  }
  return statements;
}

std::unique_ptr<aloha::Statement> Parser::parse_variable_declaration()
{
  Location loc = current_location();
  bool is_mutable = match("mut");
  if (is_mutable)
  {
    advance();
  }
  else
  {
    consume("imut",
            "Expected 'mut' or 'imut' keyword to start variable declaration.");
  }

  auto identifier = expect_identifier();
  std::optional<AlohaType::Type> type = optional_type();
  std::unique_ptr<aloha::Expression> expression = nullptr;

  if (match(TokenKind::EQUAL))
  {
    advance();
    if (match(TokenKind::LEFT_BRACKET))
    {
      expression = parse_array();
    }
    else if (peek()->kind == TokenKind::IDENT &&
             next()->kind == TokenKind::LEFT_BRACE)
    {
      expression = parse_struct_instantiation();
    }
    else
    {
      expression = parse_expression(0);
    }
  }
  return std::make_unique<aloha::Declaration>(
      loc, std::move(identifier->m_name), std::move(type),
      std::move(expression), std::move(is_mutable));
}

std::unique_ptr<aloha::Statement> Parser::parse_variable_assignment()
{
  Location loc = current_location();
  auto identifier = expect_identifier();
  consume(TokenKind::EQUAL, "Expected '=' after variable declaration");
  std::unique_ptr<aloha::Expression> expression = parse_expression(0);
  return std::make_unique<aloha::Assignment>(loc, std::move(identifier->m_name),
                                             std::move(expression));
}

std::unique_ptr<aloha::Statement> Parser::parse_struct_field_assignment()
{
  Location loc = current_location();
  peek()->dump();
  auto struct_expr = expect_identifier();
  peek()->dump();
  consume(TokenKind::THIN_ARROW, "Expected '->' for struct field assignment");
  auto field_name = expect_identifier()->m_name;
  consume(TokenKind::EQUAL, "Expected '=' in struct field assignment");
  auto value = parse_expression(0);
  return std::make_unique<aloha::StructFieldAssignment>(
      loc, std::move(struct_expr), std::move(field_name), std::move(value));
}

std::unique_ptr<aloha::Statement> Parser::parse_return_statement()
{
  Location loc = current_location();
  consume("return", "Expected 'return' keyword");
  std::unique_ptr<aloha::Expression> expression = parse_expression(0);
  return std::make_unique<aloha::ReturnStatement>(loc, std::move(expression));
}

std::unique_ptr<aloha::Statement> Parser::parse_if_statement()
{
  Location loc = current_location();
  consume("if", "Expected 'if' keyword");
  auto condition = parse_expression(0);
  consume(TokenKind::LEFT_BRACE, "Expected '{' after condition");
  std::unique_ptr<aloha::StatementBlock> then_branch = parse_statements();
  std::unique_ptr<aloha::StatementBlock> else_branch = nullptr;
  if (match("else"))
  {
    advance();
    if (match(TokenKind::IDENT) && match("if"))
    {
      std::vector<aloha::StmtPtr> else_stmts;
      loc = current_location();
      else_stmts.push_back(parse_if_statement());
      else_branch =
          std::make_unique<aloha::StatementBlock>(loc, std::move(else_stmts));
    }
    else
    {
      consume(TokenKind::LEFT_BRACE,
              "expected '{' or 'if' after 'else' keyword");
      else_branch = parse_statements();
    }
  }
  return std::make_unique<aloha::IfStatement>(loc, std::move(condition),
                                              std::move(then_branch),
                                              std::move(else_branch));
}

std::unique_ptr<aloha::Statement> Parser::parse_while_loop()
{
  Location loc = current_location();
  consume("while", "Expected 'while' keyword");
  auto condition = parse_expression(0);
  consume(TokenKind::LEFT_BRACE, "Expected '{' keyword after condition");
  auto body = parse_statements();
  return std::make_unique<aloha::WhileLoop>(loc, std::move(condition),
                                            std::move(body));
}

enum Precedence
{
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
    {"+", PREC_SUM},
    {"-", PREC_SUM},
    {"*", PREC_PRODUCT},
    {"/", PREC_PRODUCT},
    {"<", PREC_COMPARISON},
    {">", PREC_COMPARISON},
    {"==", PREC_COMPARISON},
    {"<=", PREC_COMPARISON},
    {">=", PREC_COMPARISON},
    {"!=", PREC_COMPARISON},
};

std::map<std::string, Parser::prefix_parser_func> Parser::prefix_parsers = {
    {"-",
     [](Parser &parser)
     {
       parser.advance();
       return std::make_unique<aloha::UnaryExpression>(
           parser.current_location(), "-",
           parser.parse_expression(PREC_PREFIX));
     }},
};

std::map<std::string, Parser::infix_parser_func> Parser::infix_parsers = {
    {"+",
     [](Parser &parser, auto left)
     {
       return std::make_unique<aloha::BinaryExpression>(
           parser.current_location(), std::move(left), "+",
           std::move(parser.parse_expression(PREC_SUM)));
     }},
    {"-",
     [](Parser &parser, auto left)
     {
       return std::make_unique<aloha::BinaryExpression>(
           parser.current_location(), std::move(left), "-",
           std::move(parser.parse_expression(PREC_SUM)));
     }},
    {"*",
     [](Parser &parser, auto left)
     {
       return std::make_unique<aloha::BinaryExpression>(
           parser.current_location(), std::move(left), "*",
           std::move(parser.parse_expression(PREC_PRODUCT)));
     }},
    {"/",
     [](Parser &parser, auto left)
     {
       return std::make_unique<aloha::BinaryExpression>(
           parser.current_location(), std::move(left), "/",
           std::move(parser.parse_expression(PREC_PRODUCT)));
     }},
    {"<",
     [](Parser &parser, auto left)
     {
       return std::make_unique<aloha::BinaryExpression>(
           parser.current_location(), std::move(left), "<",
           std::move(parser.parse_expression(PREC_COMPARISON)));
     }},
    {">",
     [](Parser &parser, auto left)
     {
       return std::make_unique<aloha::BinaryExpression>(
           parser.current_location(), std::move(left), ">",
           std::move(parser.parse_expression(PREC_COMPARISON)));
     }},
    {"==",
     [](Parser &parser, auto left)
     {
       return std::make_unique<aloha::BinaryExpression>(
           parser.current_location(), std::move(left),
           "==", std::move(parser.parse_expression(PREC_COMPARISON)));
     }},
    {"<=",
     [](Parser &parser, auto left)
     {
       return std::make_unique<aloha::BinaryExpression>(
           parser.current_location(), std::move(left),
           "<=", std::move(parser.parse_expression(PREC_COMPARISON)));
     }},
    {">=",
     [](Parser &parser, auto left)
     {
       return std::make_unique<aloha::BinaryExpression>(
           parser.current_location(), std::move(left),
           ">=", std::move(parser.parse_expression(PREC_COMPARISON)));
     }},
    {"!=",
     [](Parser &parser, auto left)
     {
       return std::make_unique<aloha::BinaryExpression>(
           parser.current_location(), std::move(left),
           "!=", std::move(parser.parse_expression(PREC_COMPARISON)));
     }},

};

std::unique_ptr<aloha::Expression>
Parser::parse_expression(int min_precedence)
{
  auto left = parse_primary();
  while (!is_eof())
  {
    auto operator_literal = peek()->get_lexeme();
    auto next_precedence = precedence[operator_literal];
    if (next_precedence <= min_precedence)
    {
      break;
    }
    auto infix_parser = infix_parsers.find(operator_literal);
    if (infix_parser == infix_parsers.end())
    {
      break;
    }
    advance();
    left = infix_parser->second(*this, std::move(left));
  }
  return left;
}

std::unique_ptr<aloha::Expression> Parser::parse_primary()
{
  Location loc = current_location();
  std::optional<Token> token = peek();
  if (!token)
  {
    report_error("Unexpected end of input");
    return nullptr;
  }

  if (match(TokenKind::INT) || match(TokenKind::FLOAT))
  {
    advance();
    return std::make_unique<aloha::Number>(loc, token->get_lexeme());
  }

  if (match(TokenKind::STRING))
  {
    advance();
    return std::make_unique<aloha::String>(loc, token->get_lexeme());
  }

  if (match(TokenKind::MINUS))
  {
    auto prefix = prefix_parsers[token->get_lexeme()];
    if (prefix)
    {
      return prefix(*this);
    }
  }

  if (match(TokenKind::LEFT_PAREN))
  {
    advance();
    auto expression = parse_expression(0);
    consume(TokenKind::RIGHT_PAREN, "Expected ')' after expression");
    return expression;
  }

  if (match(TokenKind::IDENT))
  {
    if (match(TokenKind::LEFT_PAREN, true))
    {
      return parse_function_call();
    }
    if (match(TokenKind::LEFT_BRACE, true))
    {
      return parse_struct_instantiation();
    }
    if (match(TokenKind::THIN_ARROW, true))
    {
      return parse_struct_field_access();
    }
    advance();
    if (is_reserved_ident(*token))
    {
      if (token->lexeme == "true" || token->lexeme == "false")
      {
        auto value = token->lexeme == "true" ? true : false;
        return std::make_unique<aloha::Boolean>(loc, value);
      }

      // TODO: represent null in better structure
      return std::make_unique<aloha::Number>(loc, "null");
    }

    return std::make_unique<aloha::Identifier>(loc, token->get_lexeme());
  }
  report_error("Unexpected token in primary expression");
  return nullptr;
}

std::unique_ptr<aloha::Expression> Parser::parse_function_call()
{
  Location loc = current_location();
  auto name = expect_identifier();
  consume(TokenKind::LEFT_PAREN, "function call must be followed by `(`");

  std::vector<aloha::ExprPtr> args;
  if (match(TokenKind::RIGHT_PAREN))
  {
    advance();
    return std::make_unique<aloha::FunctionCall>(loc, std::move(name),
                                                 std::move(args));
  }
  args.push_back(parse_expression(0));
  while (match(TokenKind::COMMA))
  {
    advance();
    auto arg = parse_expression(0);
    args.push_back(std::move(arg));
  }
  consume(TokenKind::RIGHT_PAREN, "function call must be end with `)`");
  return std::make_unique<aloha::FunctionCall>(loc, std::move(name),
                                               std::move(args));
}

std::unique_ptr<aloha::Expression> Parser::parse_array()
{
  Location loc = current_location();
  consume(TokenKind::LEFT_BRACKET, "Expected '[' at the start of array");

  std::vector<aloha::ExprPtr> members;
  while (!match(TokenKind::RIGHT_BRACKET) && !is_eof())
  {
    members.push_back(parse_expression(0));
    if (!match(TokenKind::RIGHT_BRACKET))
    {
      consume(TokenKind::COMMA, "Expected ',' or ']' after array element");
    }
  }

  consume(TokenKind::RIGHT_BRACKET, "Expected ']' at the end of array");

  return std::make_unique<aloha::Array>(loc, std::move(members));
}

std::unique_ptr<aloha::Identifier> Parser::expect_identifier()
{
  Location loc = current_location();
  if (match(TokenKind::IDENT))
  {
    auto token = peek();
    advance();
    return std::make_unique<aloha::Identifier>(loc, token->get_lexeme());
  }
  report_error("Expected identifier");
  return nullptr;
}

AlohaType::Type Parser::parse_type()
{
  auto token = peek();
  if (match(TokenKind::IDENT))
  {
    advance();
    return AlohaType::from_string(token->get_lexeme());
  }
  report_error("Expected type");
  return AlohaType::Type::UNKNOWN;
}

std::optional<AlohaType::Type> Parser::optional_type()
{
  auto token = peek();
  if (match(TokenKind::COLON))
  {
    advance();
    return parse_type();
  }
  return std::nullopt;
}

bool Parser::is_reserved_ident(Token t) const
{
  auto lexeme = t.lexeme;
  if (lexeme == "true" || lexeme == "false" || lexeme == "null")
  {
    return true;
  }
  return false;
}

bool Parser::is_reserved_ident() const
{
  auto lexeme = peek()->get_lexeme();
  if (lexeme == "true" || lexeme == "false" || lexeme == "null")
  {
    return true;
  }
  return false;
}

void Parser::dump(aloha::Program *p) const { p->write(std::cout, 2); }
