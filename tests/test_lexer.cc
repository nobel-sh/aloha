#include "../src/lexer.h"
#include "../src/reader.h"
#include <gtest/gtest.h>
#include <vector>

class LexerTest : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}
};

std::vector<char> get_bytes(const std::string &input) {
  return Aloha::StringReader(input).as_bytes();
}

TEST_F(LexerTest, HandlesBasicTokens) {
  std::string input = "( ) { } , = + - * / % ; : -> == != < > <= >=";
  Lexer lexer(get_bytes(input));
  lexer.lex();

  ASSERT_EQ(lexer.tokens.size(), 21); // +1 for EOF
  EXPECT_EQ(lexer.tokens[0].kind, TokenKind::LPAREN);
  EXPECT_EQ(lexer.tokens[1].kind, TokenKind::RPAREN);
  EXPECT_EQ(lexer.tokens[2].kind, TokenKind::LBRACE);
  EXPECT_EQ(lexer.tokens[3].kind, TokenKind::RBRACE);
  EXPECT_EQ(lexer.tokens[4].kind, TokenKind::COMMA);
  EXPECT_EQ(lexer.tokens[5].kind, TokenKind::EQUALS);
  EXPECT_EQ(lexer.tokens[6].kind, TokenKind::PLUS);
  EXPECT_EQ(lexer.tokens[7].kind, TokenKind::MINUS);
  EXPECT_EQ(lexer.tokens[8].kind, TokenKind::STAR);
  EXPECT_EQ(lexer.tokens[9].kind, TokenKind::SLASH);
  EXPECT_EQ(lexer.tokens[10].kind, TokenKind::PERCENT);
  EXPECT_EQ(lexer.tokens[11].kind, TokenKind::SEMICOLON);
  EXPECT_EQ(lexer.tokens[12].kind, TokenKind::COLON);
  EXPECT_EQ(lexer.tokens[13].kind, TokenKind::THIN_ARROW);
  EXPECT_EQ(lexer.tokens[14].kind, TokenKind::EQUALEQUAL);
  EXPECT_EQ(lexer.tokens[15].kind, TokenKind::NOTEQUAL);
  EXPECT_EQ(lexer.tokens[16].kind, TokenKind::LESSTHAN);
  EXPECT_EQ(lexer.tokens[17].kind, TokenKind::GREATERTHAN);
  EXPECT_EQ(lexer.tokens[18].kind, TokenKind::LESSTHANEQUAL);
  EXPECT_EQ(lexer.tokens[19].kind, TokenKind::GREATERTHANEQUAL);
}

TEST_F(LexerTest, HandlesIdentifiers) {
  std::string input = "variable _underscoreStart under_score123";
  Lexer lexer(get_bytes(input));
  lexer.lex();

  ASSERT_EQ(lexer.tokens.size(), 4);
  EXPECT_EQ(lexer.tokens[0].kind, TokenKind::IDENT);
  EXPECT_EQ(lexer.tokens[0].lexeme, "variable");
  EXPECT_EQ(lexer.tokens[1].kind, TokenKind::IDENT);
  EXPECT_EQ(lexer.tokens[1].lexeme, "_underscoreStart");
  EXPECT_EQ(lexer.tokens[2].kind, TokenKind::IDENT);
  EXPECT_EQ(lexer.tokens[2].lexeme, "under_score123");
}

TEST_F(LexerTest, HandlesNumbers) {
  std::string input = "123 45.67";
  Lexer lexer(get_bytes(input));
  lexer.lex();

  ASSERT_EQ(lexer.tokens.size(), 3);
  EXPECT_EQ(lexer.tokens[0].kind, TokenKind::INT);
  EXPECT_EQ(lexer.tokens[0].lexeme, "123");
  EXPECT_EQ(lexer.tokens[1].kind, TokenKind::FLOAT);
  EXPECT_EQ(lexer.tokens[1].lexeme, "45.67");
}

TEST_F(LexerTest, HandlesStrings) {
  std::string input = "\"Hello, World!\" \"Another string\"";
  Lexer lexer(get_bytes(input));
  lexer.lex();

  ASSERT_EQ(lexer.tokens.size(), 3);
  EXPECT_EQ(lexer.tokens[0].kind, TokenKind::STRING);
  EXPECT_EQ(lexer.tokens[0].lexeme, "Hello, World!");
  EXPECT_EQ(lexer.tokens[1].kind, TokenKind::STRING);
  EXPECT_EQ(lexer.tokens[1].lexeme, "Another string");
}

TEST_F(LexerTest, HandlesUntermainatedString) {
  std::string input = "\"Hello, World!";
  Lexer lexer(get_bytes(input));
  lexer.lex();

  EXPECT_TRUE(lexer.has_error);
  ASSERT_EQ(lexer.errors.size(), 1);
  EXPECT_TRUE(lexer.errors[0].find("Non terminated String") !=
              std::string::npos);
}

TEST_F(LexerTest, HandlesComplexExpression) {
  std::string input =
      "if (x <= 10) { print(\"x is less than or equal to 10\"); }";
  Lexer lexer(get_bytes(input));
  lexer.lex();

  ASSERT_GT(lexer.tokens.size(), 1);
  EXPECT_EQ(lexer.tokens[0].kind, TokenKind::IDENT);
  EXPECT_EQ(lexer.tokens[0].lexeme, "if");
  EXPECT_EQ(lexer.tokens[1].kind, TokenKind::LPAREN);
  EXPECT_EQ(lexer.tokens[2].kind, TokenKind::IDENT);
  EXPECT_EQ(lexer.tokens[2].lexeme, "x");
  EXPECT_EQ(lexer.tokens[3].kind, TokenKind::LESSTHANEQUAL);
  EXPECT_EQ(lexer.tokens[4].kind, TokenKind::INT);
  EXPECT_EQ(lexer.tokens[4].lexeme, "10");
  EXPECT_EQ(lexer.tokens[5].kind, TokenKind::RPAREN);
  EXPECT_EQ(lexer.tokens[6].kind, TokenKind::LBRACE);
  EXPECT_EQ(lexer.tokens[7].kind, TokenKind::IDENT);
  EXPECT_EQ(lexer.tokens[7].lexeme, "print");
  EXPECT_EQ(lexer.tokens[8].kind, TokenKind::LPAREN);
  EXPECT_EQ(lexer.tokens[9].kind, TokenKind::STRING);
  EXPECT_EQ(lexer.tokens[9].lexeme, "x is less than or equal to 10");
  EXPECT_EQ(lexer.tokens[10].kind, TokenKind::RPAREN);
  EXPECT_EQ(lexer.tokens[11].kind, TokenKind::SEMICOLON);
  EXPECT_EQ(lexer.tokens[12].kind, TokenKind::RBRACE);
}

TEST_F(LexerTest, HandlesEmptyInput) {
  std::string input = "";
  Lexer lexer(get_bytes(input));
  lexer.lex();

  ASSERT_EQ(lexer.tokens.size(), 1); // Just EOF
  EXPECT_EQ(lexer.tokens[0].kind, TokenKind::EOF_TOKEN);
}
