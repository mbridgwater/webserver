#ifndef NGINX_CONFIG_PARSER_H
#define NGINX_CONFIG_PARSER_H

#include <string>
#include <istream>
#include "nginx_config.h"

// Responsible for parsing an input stream into an NginxConfig AST.
class NginxConfigParser {
 public:
  NginxConfigParser() = default;

  // Parses a config from an input stream into an NginxConfig AST.
  // @return: true if valid config syntax, false otherwise.
  bool Parse(std::istream* input, NginxConfig* config);

  // Opens and parses a config file from the given path.
  // @return: true if the file is valid and successfully parsed.
  bool Parse(const char* file_name, NginxConfig* config);

 private:
  enum TokenType {
    TOKEN_TYPE_START = 0,
    TOKEN_TYPE_NORMAL = 1,
    TOKEN_TYPE_START_BLOCK = 2,
    TOKEN_TYPE_END_BLOCK = 3,
    TOKEN_TYPE_COMMENT = 4,
    TOKEN_TYPE_STATEMENT_END = 5,
    TOKEN_TYPE_EOF = 6,
    TOKEN_TYPE_ERROR = 7,
    TOKEN_TYPE_QUOTED_STRING = 8
  };

  enum TokenParserState {
    TOKEN_STATE_INITIAL_WHITESPACE = 0,
    TOKEN_STATE_SINGLE_QUOTE = 1,
    TOKEN_STATE_DOUBLE_QUOTE = 2,
    TOKEN_STATE_TOKEN_TYPE_COMMENT = 3,
    TOKEN_STATE_TOKEN_TYPE_NORMAL = 4
  };

  // Extracts the next token and its type from the input stream.
  // @param value: string to populate with the token.
  // @return: the detected token type.
  TokenType ParseToken(std::istream* input, std::string* value);

  // Converts a TokenType enum to its string representation.
  // @return: const char* label for the token type.
  const char* TokenTypeAsString(TokenType type);
};

#endif // NGINX_CONFIG_PARSER_H