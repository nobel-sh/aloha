#include "../src/parser.h"

#include "../src/lexer.h"

#include "../src/token.h"

#include <gtest/gtest.h>

#include <vector>

/*

 * TESTING// Helper function to create a parser from source code

// *Parser create_parser(const std::string &source) {

 * fun main() -> void {}  Lexer *lexer = new Lexer(source);

 * */
return Parser(*lexer);
}

TEST(ParserTest, ParseFunctionDeclaration)
{

  std::string source = "fun main() -> void {}"; /*

   Lexer lexer(source); * TESTING

   Parser parser(lexer); *

   auto program = parser.parse(); * fun main() -> void {

  *

   ASSERT_EQ(program->m_nodes.size(), 1); * }

   auto *function = dynamic_cast<aloha::Function *>(program->m_nodes[0].get()); * */

  ASSERT_NE(function, nullptr);

  EXPECT_EQ(function->m_name->m_name, "main");
  TEST(ParserTest, ParseFunctionDeclaration)
  {

    EXPECT_EQ(function->m_return_type, AlohaType::Type::VOID);
    std::string source = "fun main() -> void {}";
  }
  Lexer lexer(source);

  Parser parser(lexer);

  /*  auto program = parser.parse();

   * TESTING

   *  ASSERT_EQ(program->m_nodes.size(), 1);

   * imut foo: void = 3  auto *function = dynamic_cast<aloha::Function *>(program->m_nodes[0].get());

   * */
  ASSERT_NE(function, nullptr);

  EXPECT_EQ(function->m_name->m_name, "main");

  TEST(ParserTest, ParseVariableDeclaration)
  {
    EXPECT_EQ(function->m_return_type, AlohaType::Type::VOID);

    std::string source = "imut foo: void = 3";
  }

  Lexer lexer(source);

  Parser parser(lexer); /*

   auto stmt = parser.parse_statement(); * TESTING

   ASSERT_NE(stmt, nullptr); *

   auto *declaration = dynamic_cast<aloha::Declaration *>(stmt.get()); * imut foo: void = 3

   ASSERT_NE(declaration, nullptr); * */

  EXPECT_EQ(declaration->m_variable_name, "foo");
  TEST(ParserTest, ParseVariableDeclaration)
  {

    auto literal = dynamic_cast<aloha::Number *>(declaration->m_expression.get());  std::vector<Token> tokens = create_tokens({
      {TokenKind::IDENT, "imut", 1, 1},

          EXPECT_EQ(literal->m_value, "3");
      {TokenKind::IDENT, "foo", 1, 6},

          EXPECT_FALSE(declaration->m_is_mutable);
      {TokenKind::COLON, ":", 1, 10},

          EXPECT_TRUE(declaration->m_is_assigned);
      {TokenKind::IDENT, "void", 1, 12},

}                                             {TokenKind::EQUAL, "=", 1, 17},

                                             {TokenKind::INT, "3", 1, 19},

/*                                             {TokenKind::EOF_TOKEN, "", 2, 1}});

 * TESTING

 *  Parser parser(tokens);

 * foo = 7  auto stmt = parser.parse_statement();

 * */  ASSERT_NE(stmt, nullptr);

TEST(ParserTest, ParseVariableAssignment) {
      auto *declaration = dynamic_cast<aloha::Declaration *>(stmt.get());

      std::string source = "foo = 7";
      ASSERT_NE(declaration, nullptr);

      Lexer lexer(source);
      EXPECT_EQ(declaration->m_variable_name, "foo");

      Parser parser(lexer);
      auto literal = dynamic_cast<aloha::Number *>(declaration->m_expression.get());

      auto stmt = parser.parse_statement();
      EXPECT_EQ(literal->m_value, "3");

      ASSERT_NE(stmt, nullptr);
      EXPECT_FALSE(declaration->m_is_mutable);

      auto *assignment = dynamic_cast<aloha::Assignment *>(stmt.get());
      EXPECT_TRUE(declaration->m_is_assigned);

      ASSERT_NE(assignment, nullptr);}

  EXPECT_EQ(assignment->m_variable_name, "foo");

  auto *literal = dynamic_cast<aloha::Number *>(assignment->m_expression.get());/*

  EXPECT_EQ(literal->m_value, "7"); * TESTING

} *

 * foo = 7

/* * */

 * TESTINGTEST(ParserTest, ParseVariableAssignment) {
 *  std::vector<Token> tokens = create_tokens({
        {TokenKind::IDENT, "foo", 1, 1},

            *if x > 10
        {
          {TokenKind::EQUAL, "=", 1, 5},

              *foo = 1 {TokenKind::INT, "7", 1, 7},

              *
        } else {                                             {TokenKind::EOF_TOKEN, "", 2, 1}});

        *foo = 2

            * }  Parser parser(tokens);

 */  auto stmt = parser.parse_statement();

  ASSERT_NE(stmt, nullptr);

TEST(ParserTest, ParseIfElseStatement) {
        auto *assignment = dynamic_cast<aloha::Assignment *>(stmt.get());

        std::string source = R"(  ASSERT_NE(assignment, nullptr);

    if x > 10 {  EXPECT_EQ(assignment->m_variable_name, "foo");

      foo = 1  auto *literal = dynamic_cast<aloha::Number *>(assignment->m_expression.get());

    } else {  EXPECT_EQ(literal->m_value, "7");

      foo = 2}

    }

  )"; /*

   Lexer lexer(source); * TESTING FOR

   Parser parser(lexer); *

   auto stmt = parser.parse_statement(); * if x > 10 {

  *   foo = 1

   ASSERT_NE(stmt, nullptr); * } else {

   auto *if_stmt = dynamic_cast<aloha::IfStatement *>(stmt.get()); *   foo = 2

   ASSERT_NE(if_stmt, nullptr); * }

   auto *cond_expr = *

       dynamic_cast<aloha::BinaryExpression *>(if_stmt->m_condition.get()); */

        ASSERT_NE(cond_expr, nullptr);

        EXPECT_EQ(cond_expr->m_op, ">");
        TEST(ParserTest, ParseIfElseStatement)
        {

          ASSERT_EQ(if_stmt->m_then_branch->m_statements.size(), 1);
          std::vector<Token> tokens =

              ASSERT_NE(if_stmt->m_else_branch, nullptr);      create_tokens({
            {TokenKind::IDENT, "if", 1, 1},

                ASSERT_EQ(if_stmt->m_else_branch->m_statements.size(), 1);
            {TokenKind::IDENT, "x", 1, 4},

}                     {TokenKind::GREATER_THAN, ">", 1, 6},

                     {TokenKind::INT, "10", 1, 8},

/*                     {TokenKind::LEFT_BRACE, "{", 1, 11},

 * TESTING                     {TokenKind::IDENT, "foo", 2, 3},

 *                     {TokenKind::EQUAL, "=", 2, 7},

 * while i < 10 {                     {TokenKind::INT, "1", 2, 9},

 *  i = i + 1                     {TokenKind::RIGHT_BRACE, "}", 3, 1},

 * }                     {TokenKind::IDENT, "else", 3, 3},

 */                     {TokenKind::LEFT_BRACE, "{", 3, 8},

TEST(ParserTest, ParseWhileStatement) {
            {TokenKind::IDENT, "foo", 4, 3},

                std::string source = R"(                     {TokenKind::EQUAL, "=", 4, 7},

    while i < 10 {                     {TokenKind::INT, "2", 4, 9},

      i = i + 1                     {TokenKind::RIGHT_BRACE, "}", 5, 1},

    }                     {TokenKind::EOF_TOKEN, "", 6, 1}});

  )";

            Lexer lexer(source);
            Parser parser(tokens);

            Parser parser(lexer);
            auto stmt = parser.parse_statement();

            auto stmt = parser.parse_statement();

            ASSERT_NE(stmt, nullptr);
            ASSERT_NE(stmt, nullptr);

            auto *if_stmt = dynamic_cast<aloha::IfStatement *>(stmt.get());

            auto *while_stmt = dynamic_cast<aloha::WhileLoop *>(stmt.get());
            ASSERT_NE(if_stmt, nullptr);

            ASSERT_NE(while_stmt, nullptr);
            auto *cond_expr =

                auto *cond_expr = dynamic_cast<aloha::BinaryExpression *>(if_stmt->m_condition.get());

            dynamic_cast<aloha::BinaryExpression *>(while_stmt->m_condition.get());
            ASSERT_NE(cond_expr, nullptr);

            ASSERT_NE(cond_expr, nullptr);
            EXPECT_EQ(cond_expr->m_op, ">");

            EXPECT_EQ(cond_expr->m_op, "<");
            ASSERT_EQ(if_stmt->m_then_branch->m_statements.size(), 1);

            ASSERT_EQ(while_stmt->m_body->m_statements.size(), 1);
            ASSERT_NE(if_stmt->m_else_branch, nullptr);

}  ASSERT_EQ(if_stmt->m_else_branch->m_statements.size(), 1);
        }

        /*

         * TESTING/*

         * * TESTING FOR

         * foo - 10 *

         */ *while i < 10
        {

          *i = i + 1

               TEST(ParserTest, ParseSimpleArithmeticExpression){*}

               std::string source = "foo - 10";
          **/

              Lexer lexer(source);
          TEST(ParserTest, ParseWhileStatement)
          {

            Parser parser(lexer);
            std::vector<Token> tokens =

                auto expr = parser.parse_expression(0);
            create_tokens({{TokenKind::IDENT, "while", 1, 1},

                     {TokenKind::IDENT, "i", 1, 7},

  auto *binary_expr = dynamic_cast<aloha::BinaryExpression *>(expr.get());                     {TokenKind::LESS_THAN, "<", 1, 9},

  ASSERT_NE(binary_expr, nullptr);                     {TokenKind::INT, "10", 1, 11},

  EXPECT_EQ(binary_expr->m_op, "-");                     {TokenKind::LEFT_BRACE, "{", 1, 14},

                     {TokenKind::IDENT, "i", 2, 3},

  auto *left = dynamic_cast<aloha::Identifier *>(binary_expr->m_left.get());                     {TokenKind::EQUAL, "=", 2, 5},

  ASSERT_NE(left, nullptr);                     {TokenKind::IDENT, "i", 2, 7},

  EXPECT_EQ(left->m_name, "foo");                     {TokenKind::PLUS, "+", 2, 9},

                     {TokenKind::INT, "1", 2, 11},

  auto *right = dynamic_cast<aloha::Number *>(binary_expr->m_right.get());                     {TokenKind::RIGHT_BRACE, "}", 3, 1},

  ASSERT_NE(right, nullptr);                     {TokenKind::EOF_TOKEN, "", 4, 1} });

            EXPECT_EQ(right->m_value, "10");
          }
          Parser parser(tokens);

          auto stmt = parser.parse_statement();

          /*  ASSERT_NE(stmt, nullptr);

           * TESTING

           * 3 + 5 * ( 10 - 2 )  auto *while_stmt = dynamic_cast<aloha::WhileLoop *>(stmt.get());

           *  ASSERT_NE(while_stmt, nullptr);

           * Parse Tree  auto *cond_expr =

           *      dynamic_cast<aloha::BinaryExpression *>(while_stmt->m_condition.get());

           *      +  ASSERT_NE(cond_expr, nullptr);

           *     / \  EXPECT_EQ(cond_expr->m_op, "<");

           *     3 *  ASSERT_EQ(while_stmt->m_body->m_statements.size(), 1);

           *       /\}

           *      5  -

           *         /\/*

           *        10 2 * TESTING

           */ *

              *foo -
              10

              TEST(ParserTest, ParseComplexArithmeticExpression)
          {
            **/

                std::string source = "3 + 5 * (10 - 2)";

            Lexer lexer(source);
            TEST(ParserTest, ParseSimpleArithmeticExpression)
            {

              Parser parser(lexer);
              std::vector<Token> tokens = create_tokens({{TokenKind::IDENT, "foo", 1, 1},

  auto expr = parser.parse_expression(0);                                             {TokenKind::MINUS, "-", 1, 5},

                                             {TokenKind::INT, "10", 1, 7},

  auto *binary_expr = dynamic_cast<aloha::BinaryExpression *>(expr.get());                                             {TokenKind::EOF_TOKEN, "", 2, 1} });

              ASSERT_NE(binary_expr, nullptr);

              EXPECT_EQ(binary_expr->m_op, "+");
              Parser parser(tokens);

              auto expr = parser.parse_expression(0);

              auto *left = dynamic_cast<aloha::Number *>(binary_expr->m_left.get());

              ASSERT_NE(left, nullptr);
              auto *binary_expr = dynamic_cast<aloha::BinaryExpression *>(expr.get());

              EXPECT_EQ(left->m_value, "3");
              ASSERT_NE(binary_expr, nullptr);

              EXPECT_EQ(binary_expr->m_op, "-");

              auto *right =

                  dynamic_cast<aloha::BinaryExpression *>(binary_expr->m_right.get());
              auto *left = dynamic_cast<aloha::Identifier *>(binary_expr->m_left.get());

              ASSERT_NE(right, nullptr);
              ASSERT_NE(left, nullptr);

              EXPECT_EQ(right->m_op, "*");
              EXPECT_EQ(left->m_name, "foo");

              auto *right_left = dynamic_cast<aloha::Number *>(right->m_left.get());
              auto *right = dynamic_cast<aloha::Number *>(binary_expr->m_right.get());

              ASSERT_NE(right_left, nullptr);
              ASSERT_NE(right, nullptr);

              EXPECT_EQ(right_left->m_value, "5");
              EXPECT_EQ(right->m_value, "10");
            }

            auto *right_right =

                dynamic_cast<aloha::BinaryExpression *>(right->m_right.get()); /*

             ASSERT_NE(right_right, nullptr); * TESTING

             EXPECT_EQ(right_right->m_op, "-"); * 3 + 5 * ( 10 - 2 )

            *

             auto *right_right_left = * Parse Tree

                 dynamic_cast<aloha::Number *>(right_right->m_left.get()); *

             ASSERT_NE(right_right_left, nullptr); *      +

             EXPECT_EQ(right_right_left->m_value, "10"); *     / \

            *     3 *

             auto *right_right_right = *       /\

                 dynamic_cast<aloha::Number *>(right_right->m_right.get()); *      5  -

             ASSERT_NE(right_right_right, nullptr); *         /\

             EXPECT_EQ(right_right_right->m_value, "2"); *        10 2

           } * */

            TEST(ParserTest, ParseComplexArithmeticExpression)
            {
              std::vector<Token> tokens =
                  create_tokens({{TokenKind::INT, "3", 1, 1},
                                 {TokenKind::PLUS, "+", 1, 3},
                                 {TokenKind::INT, "5", 1, 5},
                                 {TokenKind::STAR, "*", 1, 7},
                                 {TokenKind::LEFT_PAREN, "(", 1, 9},
                                 {TokenKind::INT, "10", 1, 10},
                                 {TokenKind::MINUS, "-", 1, 13},
                                 {TokenKind::INT, "2", 1, 15},
                                 {TokenKind::RIGHT_PAREN, ")", 1, 16},
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
