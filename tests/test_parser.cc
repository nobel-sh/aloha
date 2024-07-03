#include "../src/parser.h"
#include "../src/token.h"
#include <gtest/gtest.h>
#include <vector>

// Helper function to create tokens for testing
std::vector<Token>
create_tokens(const std::vector<std::tuple<TokenKind, std::string, int, int>>
                  &token_data) {
  std::vector<Token> tokens;
  for (const auto &[kind, lexeme, line, col] : token_data) {
    Location loc(line, col);
    tokens.emplace_back(kind, lexeme, loc);
  }
  return tokens;
}

/*
 * TESTING
 *
 * fun main() -> void {
 *
 * }
 * */

TEST(ParserTest, ParseFunctionDeclaration) {
  std::vector<Token> tokens =
      create_tokens({{TokenKind::IDENT, "fun", 1, 1},
                     {TokenKind::IDENT, "main", 1, 5},
                     {TokenKind::LPAREN, "(", 1, 9},
                     {TokenKind::RPAREN, ")", 1, 10},
                     {TokenKind::THIN_ARROW, "->", 1, 12},
                     {TokenKind::IDENT, "void", 1, 15},
                     {TokenKind::LBRACE, "{", 2, 1},
                     {TokenKind::RBRACE, "}", 3, 1},
                     {TokenKind::EOF_TOKEN, "", 4, 1}});

  Parser parser(tokens);
  auto program = parser.parse();

  ASSERT_EQ(program->nodes.size(), 1);
  auto function = std::dynamic_pointer_cast<Aloha::Function>(program->nodes[0]);
  ASSERT_NE(function, nullptr);
  EXPECT_EQ(function->name->name, "main");
  EXPECT_EQ(function->return_type, AlohaType::Type::VOID);
}

/*
 * TESTING
 *
 * imut foo: void = 3
 * */
TEST(ParserTest, ParseVariableDeclaration) {
  std::vector<Token> tokens = create_tokens({{TokenKind::IDENT, "imut", 1, 1},
                                             {TokenKind::IDENT, "foo", 1, 6},
                                             {TokenKind::COLON, ":", 1, 10},
                                             {TokenKind::IDENT, "void", 1, 12},
                                             {TokenKind::EQUALS, "=", 1, 17},
                                             {TokenKind::INT, "3", 1, 19},
                                             {TokenKind::EOF_TOKEN, "", 2, 1}});

  Parser parser(tokens);
  auto stmt = parser.parse_statement();
  ASSERT_NE(stmt, nullptr);
  auto declaration = std::dynamic_pointer_cast<Aloha::Declaration>(stmt);
  ASSERT_NE(declaration, nullptr);
  EXPECT_EQ(declaration->variable_name, "foo");
  auto literal =
      std::dynamic_pointer_cast<Aloha::Number>(declaration->expression);
  EXPECT_EQ(literal->value, "3");
  EXPECT_FALSE(declaration->is_mutable);
  EXPECT_TRUE(declaration->is_assigned);
}

/*
 * TESTING
 *
 * foo = 7
 * */
TEST(ParserTest, ParseVariableAssignment) {
  std::vector<Token> tokens = create_tokens({{TokenKind::IDENT, "foo", 1, 1},
                                             {TokenKind::EQUALS, "=", 1, 5},
                                             {TokenKind::INT, "7", 1, 7},
                                             {TokenKind::EOF_TOKEN, "", 2, 1}});

  Parser parser(tokens);
  auto stmt = parser.parse_statement();
  ASSERT_NE(stmt, nullptr);
  auto assignment = std::dynamic_pointer_cast<Aloha::Assignment>(stmt);
  ASSERT_NE(assignment, nullptr);
  EXPECT_EQ(assignment->variable_name, "foo");
  auto literal =
      std::dynamic_pointer_cast<Aloha::Number>(assignment->expression);
  EXPECT_EQ(literal->value, "7");
}

/*
 * TESTING FOR
 *
 * if x > 10 {
 *   foo = 1
 * } else {
 *   foo = 2
 * }
 *
 */

TEST(ParserTest, ParseIfElseStatement) {
  std::vector<Token> tokens =
      create_tokens({{TokenKind::IDENT, "if", 1, 1},
                     {TokenKind::IDENT, "x", 1, 4},
                     {TokenKind::GREATERTHAN, ">", 1, 6},
                     {TokenKind::INT, "10", 1, 8},
                     {TokenKind::LBRACE, "{", 1, 11},
                     {TokenKind::IDENT, "foo", 2, 3},
                     {TokenKind::EQUALS, "=", 2, 7},
                     {TokenKind::INT, "1", 2, 9},
                     {TokenKind::RBRACE, "}", 3, 1},
                     {TokenKind::IDENT, "else", 3, 3},
                     {TokenKind::LBRACE, "{", 3, 8},
                     {TokenKind::IDENT, "foo", 4, 3},
                     {TokenKind::EQUALS, "=", 4, 7},
                     {TokenKind::INT, "2", 4, 9},
                     {TokenKind::RBRACE, "}", 5, 1},
                     {TokenKind::EOF_TOKEN, "", 6, 1}});

  Parser parser(tokens);
  auto stmt = parser.parse_statement();

  ASSERT_NE(stmt, nullptr);
  auto if_stmt = std::dynamic_pointer_cast<Aloha::IfStatement>(stmt);
  ASSERT_NE(if_stmt, nullptr);
  auto cond_expr =
      std::dynamic_pointer_cast<Aloha::BinaryExpression>(if_stmt->condition);
  ASSERT_NE(cond_expr, nullptr);
  EXPECT_EQ(cond_expr->op, ">");
  auto then_branch = if_stmt->then_branch;
  ASSERT_EQ(then_branch->statements.size(), 1);
  auto else_branch = if_stmt->else_branch;
  ASSERT_EQ(else_branch->statements.size(), 1);
}

/*
 * TESTING FOR
 *
 * while i < 10 {
 *  i = i + 1
 * }
 * */
TEST(ParserTest, ParseWhileStatement) {
  std::vector<Token> tokens = create_tokens({{TokenKind::IDENT, "while", 1, 1},
                                             {TokenKind::IDENT, "i", 1, 7},
                                             {TokenKind::LESSTHAN, "<", 1, 9},
                                             {TokenKind::INT, "10", 1, 11},
                                             {TokenKind::LBRACE, "{", 1, 14},
                                             {TokenKind::IDENT, "i", 2, 3},
                                             {TokenKind::EQUALS, "=", 2, 5},
                                             {TokenKind::IDENT, "i", 2, 7},
                                             {TokenKind::PLUS, "+", 2, 9},
                                             {TokenKind::INT, "1", 2, 11},
                                             {TokenKind::RBRACE, "}", 3, 1},
                                             {TokenKind::EOF_TOKEN, "", 4, 1}});

  Parser parser(tokens);
  auto stmt = parser.parse_statement();
  ASSERT_NE(stmt, nullptr);

  auto while_stmt = std::dynamic_pointer_cast<Aloha::WhileLoop>(stmt);
  ASSERT_NE(while_stmt, nullptr);
  auto cond_expr =
      std::dynamic_pointer_cast<Aloha::BinaryExpression>(while_stmt->condition);
  ASSERT_NE(cond_expr, nullptr);
  EXPECT_EQ(cond_expr->op, "<");
  auto body = while_stmt->body;
  ASSERT_EQ(body->statements.size(), 1);
}

/*
 * TESTING
 *
 * foo - 10
 * */

TEST(ParserTest, ParseSimpleArithmeticExpression) {
  std::vector<Token> tokens = create_tokens({{TokenKind::IDENT, "foo", 1, 1},
                                             {TokenKind::MINUS, "-", 1, 5},
                                             {TokenKind::INT, "10", 1, 7},
                                             {TokenKind::EOF_TOKEN, "", 2, 1}});

  Parser parser(tokens);
  auto expr = parser.parse_expression(0);

  auto binary_expr = std::dynamic_pointer_cast<Aloha::BinaryExpression>(expr);
  ASSERT_NE(binary_expr, nullptr);
  EXPECT_EQ(binary_expr->op, "-");

  auto left = std::dynamic_pointer_cast<Aloha::Identifier>(binary_expr->left);
  ASSERT_NE(left, nullptr);
  EXPECT_EQ(left->name, "foo");

  auto right = std::dynamic_pointer_cast<Aloha::Number>(binary_expr->right);
  ASSERT_NE(right, nullptr);
  EXPECT_EQ(right->value, "10");
}

/*
 * TESTING
 * 3 + 5 * ( 10 - 2 )
 *
 * Parse Tree
 *
 *      +
 *     / \
 *     3 *
 *       /\
 *      5  -
 *         /\
 *        10 2
 * */

TEST(ParserTest, ParseComplexArithmeticExpression) {
  std::vector<Token> tokens = create_tokens({{TokenKind::INT, "3", 1, 1},
                                             {TokenKind::PLUS, "+", 1, 3},
                                             {TokenKind::INT, "5", 1, 5},
                                             {TokenKind::STAR, "*", 1, 7},
                                             {TokenKind::LPAREN, "(", 1, 9},
                                             {TokenKind::INT, "10", 1, 10},
                                             {TokenKind::MINUS, "-", 1, 13},
                                             {TokenKind::INT, "2", 1, 15},
                                             {TokenKind::RPAREN, ")", 1, 16},
                                             {TokenKind::EOF_TOKEN, "", 2, 1}});

  Parser parser(tokens);
  auto expr = parser.parse_expression(0);

  auto binary_expr = std::dynamic_pointer_cast<Aloha::BinaryExpression>(expr);
  ASSERT_NE(binary_expr, nullptr);
  EXPECT_EQ(binary_expr->op, "+");

  auto left = std::dynamic_pointer_cast<Aloha::Number>(binary_expr->left);
  ASSERT_NE(left, nullptr);
  EXPECT_EQ(left->value, "3");

  auto right =
      std::dynamic_pointer_cast<Aloha::BinaryExpression>(binary_expr->right);
  ASSERT_NE(right, nullptr);
  EXPECT_EQ(right->op, "*");

  auto right_left = std::dynamic_pointer_cast<Aloha::Number>(right->left);
  ASSERT_NE(right_left, nullptr);
  EXPECT_EQ(right_left->value, "5");

  auto right_right =
      std::dynamic_pointer_cast<Aloha::BinaryExpression>(right->right);
  ASSERT_NE(right_right, nullptr);
  EXPECT_EQ(right_right->op, "-");

  auto right_right_left =
      std::dynamic_pointer_cast<Aloha::Number>(right_right->left);
  ASSERT_NE(right_right_left, nullptr);
  EXPECT_EQ(right_right_left->value, "10");

  auto right_right_right =
      std::dynamic_pointer_cast<Aloha::Number>(right_right->right);
  ASSERT_NE(right_right_right, nullptr);
  EXPECT_EQ(right_right_right->value, "2");
}
