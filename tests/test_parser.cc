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

  ASSERT_EQ(program->m_nodes.size(), 1);
  auto *function = dynamic_cast<aloha::Function *>(program->m_nodes[0].get());
  ASSERT_NE(function, nullptr);
  EXPECT_EQ(function->m_name->m_name, "main");
  EXPECT_EQ(function->m_return_type, AlohaType::Type::VOID);
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
  auto *declaration = dynamic_cast<aloha::Declaration *>(stmt.get());
  ASSERT_NE(declaration, nullptr);
  EXPECT_EQ(declaration->m_variable_name, "foo");
  auto literal = dynamic_cast<aloha::Number *>(declaration->m_expression.get());
  EXPECT_EQ(literal->m_value, "3");
  EXPECT_FALSE(declaration->m_is_mutable);
  EXPECT_TRUE(declaration->m_is_assigned);
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
  auto *assignment = dynamic_cast<aloha::Assignment *>(stmt.get());
  ASSERT_NE(assignment, nullptr);
  EXPECT_EQ(assignment->m_variable_name, "foo");
  auto *literal = dynamic_cast<aloha::Number *>(assignment->m_expression.get());
  EXPECT_EQ(literal->m_value, "7");
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
  auto *if_stmt = dynamic_cast<aloha::IfStatement *>(stmt.get());
  ASSERT_NE(if_stmt, nullptr);
  auto *cond_expr =
      dynamic_cast<aloha::BinaryExpression *>(if_stmt->m_condition.get());
  ASSERT_NE(cond_expr, nullptr);
  EXPECT_EQ(cond_expr->m_op, ">");
  ASSERT_EQ(if_stmt->m_then_branch->m_statements.size(), 1);
  ASSERT_NE(if_stmt->m_else_branch, nullptr);
  ASSERT_EQ(if_stmt->m_else_branch->m_statements.size(), 1);
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

  auto *while_stmt = dynamic_cast<aloha::WhileLoop *>(stmt.get());
  ASSERT_NE(while_stmt, nullptr);
  auto *cond_expr =
      dynamic_cast<aloha::BinaryExpression *>(while_stmt->m_condition.get());
  ASSERT_NE(cond_expr, nullptr);
  EXPECT_EQ(cond_expr->m_op, "<");
  ASSERT_EQ(while_stmt->m_body->m_statements.size(), 1);
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

  auto *binary_expr = dynamic_cast<aloha::BinaryExpression *>(expr.get());
  ASSERT_NE(binary_expr, nullptr);
  EXPECT_EQ(binary_expr->m_op, "-");

  auto *left = dynamic_cast<aloha::Identifier *>(binary_expr->m_left.get());
  ASSERT_NE(left, nullptr);
  EXPECT_EQ(left->m_name, "foo");

  auto *right = dynamic_cast<aloha::Number *>(binary_expr->m_right.get());
  ASSERT_NE(right, nullptr);
  EXPECT_EQ(right->m_value, "10");
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

  auto *binary_expr = dynamic_cast<aloha::BinaryExpression *>(expr.get());
  ASSERT_NE(binary_expr, nullptr);
  EXPECT_EQ(binary_expr->m_op, "+");

  auto *left = dynamic_cast<aloha::Number *>(binary_expr->m_left.get());
  ASSERT_NE(left, nullptr);
  EXPECT_EQ(left->m_value, "3");

  auto *right =
      dynamic_cast<aloha::BinaryExpression *>(binary_expr->m_right.get());
  ASSERT_NE(right, nullptr);
  EXPECT_EQ(right->m_op, "*");

  auto *right_left = dynamic_cast<aloha::Number *>(right->m_left.get());
  ASSERT_NE(right_left, nullptr);
  EXPECT_EQ(right_left->m_value, "5");

  auto *right_right =
      dynamic_cast<aloha::BinaryExpression *>(right->m_right.get());
  ASSERT_NE(right_right, nullptr);
  EXPECT_EQ(right_right->m_op, "-");

  auto *right_right_left =
      dynamic_cast<aloha::Number *>(right_right->m_left.get());
  ASSERT_NE(right_right_left, nullptr);
  EXPECT_EQ(right_right_left->m_value, "10");

  auto *right_right_right =
      dynamic_cast<aloha::Number *>(right_right->m_right.get());
  ASSERT_NE(right_right_right, nullptr);
  EXPECT_EQ(right_right_right->m_value, "2");
}
