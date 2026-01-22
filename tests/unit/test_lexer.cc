#include "../src/lexer.h"
#include "../src/reader.h"
#include <gtest/gtest.h>
#include <iostream>
#include <vector>

class LexerTest : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(LexerTest, HandlesBasicTokens) {
  std::string input = "( ) { } , = + - * / % ; : -> == != < > <= >=";
  auto source = Aloha::StringReader(input).as_string();
  auto source_sv = std::string_view(source);
  Lexer lexer(source_sv);
  lexer.lex();

  std::vector<Token> tokens = lexer.get_tokens();

  ASSERT_EQ(tokens.size(), 21); // +1 for EOF
  EXPECT_EQ(tokens[0].kind, TokenKind::LEFT_PAREN);
  EXPECT_EQ(tokens[1].kind, TokenKind::RIGHT_PAREN);
  EXPECT_EQ(tokens[2].kind, TokenKind::LEFT_BRACE);
  EXPECT_EQ(tokens[3].kind, TokenKind::RIGHT_BRACE);
  EXPECT_EQ(tokens[4].kind, TokenKind::COMMA);
  EXPECT_EQ(tokens[5].kind, TokenKind::EQUAL);
  EXPECT_EQ(tokens[6].kind, TokenKind::PLUS);
  EXPECT_EQ(tokens[7].kind, TokenKind::MINUS);
  EXPECT_EQ(tokens[8].kind, TokenKind::STAR);
  EXPECT_EQ(tokens[9].kind, TokenKind::SLASH);
  EXPECT_EQ(tokens[10].kind, TokenKind::PERCENT);
  EXPECT_EQ(tokens[11].kind, TokenKind::SEMICOLON);
  EXPECT_EQ(tokens[12].kind, TokenKind::COLON);
  EXPECT_EQ(tokens[13].kind, TokenKind::THIN_ARROW);
  EXPECT_EQ(tokens[14].kind, TokenKind::EQUAL_EQUAL);
  EXPECT_EQ(tokens[15].kind, TokenKind::NOT_EQUAL);
  EXPECT_EQ(tokens[16].kind, TokenKind::LESS_THAN);
  EXPECT_EQ(tokens[17].kind, TokenKind::GREATER_THAN);
  EXPECT_EQ(tokens[18].kind, TokenKind::LESS_EQUAL);
  EXPECT_EQ(tokens[19].kind, TokenKind::GREATER_EQUAL);
}

TEST_F(LexerTest, HandlesIdentifiers) {
  std::string input = "variable _underscoreStart under_score123";
  auto source = Aloha::StringReader(input).as_string();
  auto source_sv = std::string_view(source);
  Lexer lexer(source_sv);
  lexer.lex();
  std::vector<Token> tokens = lexer.get_tokens();

  ASSERT_EQ(tokens.size(), 4);
  EXPECT_EQ(tokens[0].kind, TokenKind::IDENT);
  EXPECT_EQ(tokens[0].get_lexeme(), "variable");
  EXPECT_EQ(tokens[1].kind, TokenKind::IDENT);
  EXPECT_EQ(tokens[1].get_lexeme(), "_underscoreStart");
  EXPECT_EQ(tokens[2].kind, TokenKind::IDENT);
  EXPECT_EQ(tokens[2].get_lexeme(), "under_score123");
}

TEST_F(LexerTest, HandlesNumbers) {
  std::string input = "123 45.67";
  auto source = Aloha::StringReader(input).as_string();
  auto source_sv = Aloha::StringReader(input).as_string();
  Lexer lexer(source_sv);
  lexer.lex();

  std::vector<Token> tokens = lexer.get_tokens();

  ASSERT_EQ(tokens.size(), 3);
  EXPECT_EQ(tokens[0].kind, TokenKind::INT);
  EXPECT_EQ(tokens[0].get_lexeme(), "123");
  EXPECT_EQ(tokens[1].kind, TokenKind::FLOAT);
  EXPECT_EQ(tokens[1].get_lexeme(), "45.67");
}

TEST_F(LexerTest, HandlesStrings) {
  std::string input = "\"Hello, World!\" \"Another string\"";
  auto source = Aloha::StringReader(input).as_string();
  auto source_sv = Aloha::StringReader(input).as_string();
  Lexer lexer(source_sv);
  lexer.lex();

  std::vector<Token> tokens = lexer.get_tokens();

  ASSERT_EQ(tokens.size(), 3);
  EXPECT_EQ(tokens[0].kind, TokenKind::STRING);
  EXPECT_EQ(tokens[0].get_lexeme(), "Hello, World!");
  EXPECT_EQ(tokens[1].kind, TokenKind::STRING);
  EXPECT_EQ(tokens[1].get_lexeme(), "Another string");
}

TEST_F(LexerTest, HandlesUntermainatedString) {
  std::string input = "\"Hello, World!";
  auto source = Aloha::StringReader(input).as_string();
  auto source_sv = Aloha::StringReader(input).as_string();
  Lexer lexer(source_sv);
  lexer.lex();
  std::vector<Token> tokens = lexer.get_tokens();

  EXPECT_TRUE(lexer.has_error());
  ASSERT_EQ(lexer.get_errors().size(), 1);
  auto errors = lexer.get_errors();
  EXPECT_TRUE(errors[0].find("Unterminated string") != std::string::npos);
}

TEST_F(LexerTest, HandlesComplexExpression) {
  std::string input =
      "if (x <= 10) { print(\"x is less than or equal to 10\"); }";
  auto source = Aloha::StringReader(input).as_string();
  auto source_sv = Aloha::StringReader(input).as_string();
  Lexer lexer(source_sv);
  lexer.lex();
  std::vector<Token> tokens = lexer.get_tokens();

  ASSERT_GT(tokens.size(), 1);
  EXPECT_EQ(tokens[0].kind, TokenKind::IDENT);
  EXPECT_EQ(tokens[0].get_lexeme(), "if");
  EXPECT_EQ(tokens[1].kind, TokenKind::LEFT_PAREN);
  EXPECT_EQ(tokens[2].kind, TokenKind::IDENT);
  EXPECT_EQ(tokens[2].get_lexeme(), "x");
  EXPECT_EQ(tokens[3].kind, TokenKind::LESS_EQUAL);
  EXPECT_EQ(tokens[4].kind, TokenKind::INT);
  EXPECT_EQ(tokens[4].get_lexeme(), "10");
  EXPECT_EQ(tokens[5].kind, TokenKind::RIGHT_PAREN);
  EXPECT_EQ(tokens[6].kind, TokenKind::LEFT_BRACE);
  EXPECT_EQ(tokens[7].kind, TokenKind::IDENT);
  EXPECT_EQ(tokens[7].get_lexeme(), "print");
  EXPECT_EQ(tokens[8].kind, TokenKind::LEFT_PAREN);
  EXPECT_EQ(tokens[9].kind, TokenKind::STRING);
  EXPECT_EQ(tokens[9].get_lexeme(), "x is less than or equal to 10");
  EXPECT_EQ(tokens[10].kind, TokenKind::RIGHT_PAREN);
  EXPECT_EQ(tokens[11].kind, TokenKind::SEMICOLON);
  EXPECT_EQ(tokens[12].kind, TokenKind::RIGHT_BRACE);
}

TEST_F(LexerTest, HandlesEmptyInput) {
  std::string input = "";
  auto source = Aloha::StringReader(input).as_string();
  auto source_sv = Aloha::StringReader(input).as_string();
  Lexer lexer(source_sv);
  lexer.lex();
  std::vector<Token> tokens = lexer.get_tokens();

  ASSERT_EQ(tokens.size(), 1); // Just EOF
  EXPECT_EQ(tokens[0].kind, TokenKind::EOF_TOKEN);
}
