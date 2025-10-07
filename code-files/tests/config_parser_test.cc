#include "gtest/gtest.h"
#include "nginx_config_parser.h"
#include <fstream>
#include <sstream>

class NginxConfigParserTest : public ::testing::Test {
  protected:
    NginxConfigParser parser;
    NginxConfig out_config;
};

// --------- Happy path tests ---------

// Simple flat statement to test ToString 
// Expected result: PASS 
TEST_F(NginxConfigParserTest, SimpleStatement) {
  bool success = parser.Parse("test_configs/parser_configs/simple_statement", &out_config);
  EXPECT_TRUE(success);

  EXPECT_EQ(out_config.ToString(0),  "listen 80;\n");
}

// Nested statements to test ToString 
// Expected result: PASS 
TEST_F(NginxConfigParserTest, NestedStatement) {
  bool success = parser.Parse("test_configs/parser_configs/nested_config", &out_config);
  EXPECT_TRUE(success);

  EXPECT_EQ(out_config.ToString(0),  "stream {\n"
  "  server {\n"
  "    listen 80;\n"
  "  }\n"
  "}\n");
}

// Sanity test 
// Expected result: PASS
TEST_F(NginxConfigParserTest, SimpleConfig) {
  bool success = parser.Parse("test_configs/parser_configs/example_config", &out_config);
  EXPECT_TRUE(success);
}

// Multiple statements
// Expected result: PASS
TEST_F(NginxConfigParserTest, MultipleStatementsConfig) {
  bool success = parser.Parse("test_configs/parser_configs/multiple_statements_config", &out_config);
  EXPECT_TRUE(success);
}

// Multiple blocks 
// Expected result: PASS 
TEST_F(NginxConfigParserTest, MultipleBlocksConfig) {
  bool success = parser.Parse("test_configs/parser_configs/multiple_blocks_config", &out_config);
  EXPECT_TRUE(success);
}

// Config with comments 
// Expected result: PASS 
TEST_F(NginxConfigParserTest, CommentConfig) {
  bool success = parser.Parse("test_configs/parser_configs/comment_config", &out_config);
  EXPECT_TRUE(success);
}

// Nested config 
// Expected result: PASS 
TEST_F(NginxConfigParserTest, NestedConfig) {
  bool success = parser.Parse("test_configs/parser_configs/nested_config", &out_config);
  EXPECT_TRUE(success);
}

// Single quoted config
// Expected result: PASS 
TEST_F(NginxConfigParserTest, SingleQuoteConfig) {
  bool success = parser.Parse("test_configs/parser_configs/single_quoted_config", &out_config);
  EXPECT_TRUE(success);
}

// Double quoted config
// Expected result: PASS 
TEST_F(NginxConfigParserTest, DoubleQuoteConfig) {
  bool success = parser.Parse("test_configs/parser_configs/double_quoted_config", &out_config);
  EXPECT_TRUE(success);
}

// Escaped quoted config
// Expected result: PASS 
TEST_F(NginxConfigParserTest, EscapeQuoteConfig) {
  bool success = parser.Parse("test_configs/parser_configs/escaped_quote_config", &out_config);
  EXPECT_TRUE(success);
}

// --------- Unhappy path tests ---------

// Empty input 
// Expected result: FAIL
TEST_F(NginxConfigParserTest, EmptyConfig) {
  bool success = parser.Parse("test_configs/parser_configs/empty_config", &out_config);
  EXPECT_FALSE(success);
}

// Config with unclosed braces 
// Expected result: FAIL 
TEST_F(NginxConfigParserTest, UnclosedBraceConfig) {
  bool success = parser.Parse("test_configs/parser_configs/unclosed_brace_config", &out_config);
  EXPECT_FALSE(success);
}

// Config with extra braces 
// Expected result: FAIL 
TEST_F(NginxConfigParserTest, ExtraBraceConfig) {
  bool success = parser.Parse("test_configs/parser_configs/extra_brace_config", &out_config);
  EXPECT_FALSE(success);
}

// Unexpected token  
// Expected result: FAIL 
TEST_F(NginxConfigParserTest, UnexpectedTokenConfig) {
  bool success = parser.Parse("test_configs/parser_configs/unexpected_token_config", &out_config);
  EXPECT_FALSE(success);
}

// Missing semicolon 
// Expected result: FAIL 
TEST_F(NginxConfigParserTest, MissingSemicolonConfig) {
  bool success = parser.Parse("test_configs/parser_configs/missing_semicolon_config", &out_config);
  EXPECT_FALSE(success);
}

// Config with missing start brace
// Expected result: FAIL
TEST_F(NginxConfigParserTest, MissingStartBraceConfig) {
  bool success = parser.Parse("test_configs/parser_configs/missing_start_brace_config", &out_config);
  EXPECT_FALSE(success);
}

// Unclosed quoted config
// Expected result: FAIL
TEST_F(NginxConfigParserTest, UnclosedQuoteConfig) {
  bool success = parser.Parse("test_configs/parser_configs/unclosed_quote_config", &out_config);
  EXPECT_FALSE(success);
}

// Invalid quoted config
// Expected result: FAIL
TEST_F(NginxConfigParserTest, InvalidQuoteConfig) {
  bool success = parser.Parse("test_configs/parser_configs/invalid_quote_config", &out_config);
  EXPECT_FALSE(success);
}

// Unexpected tokens following each other 
// Expected result: FAIL 
TEST_F(NginxConfigParserTest, UnexpectedTokenOrderConfig) {
  bool success = parser.Parse("test_configs/parser_configs/unexpected_token_order_config", &out_config);
  EXPECT_FALSE(success);
}

// Block opened without a valid statement name 
// Expected result: FAIL 
TEST_F(NginxConfigParserTest, InvalidStatmentNameConfig) {
  bool success = parser.Parse("test_configs/parser_configs/invalid_statement_name_config", &out_config);
  EXPECT_FALSE(success);
}

// Semicolon found without valid statement 
// Expected result: FAIL 
TEST_F(NginxConfigParserTest, SemicolonNoStatementConfig) { 
  bool success = parser.Parse("test_configs/parser_configs/semicolon_no_statement_config", &out_config);
  EXPECT_FALSE(success);
}

// Nonexistent config 
// Expected result: FAIL 
TEST_F(NginxConfigParserTest, FileParseFailsForMissingFile) {
  EXPECT_FALSE(parser.Parse("non_existent", &out_config));
}

// Invalid character following quote
// Expected result: FAIL
TEST_F(NginxConfigParserTest, InvalidCharacterFollowingQuoteConfig) {
  EXPECT_FALSE(parser.Parse("test_configs/parser_configs/invalid_character_following_quote_config", &out_config));
}