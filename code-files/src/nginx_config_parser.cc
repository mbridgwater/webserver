#include "nginx_config_parser.h"
#include "logger.h"
#include <stack>
#include <fstream>

const char* NginxConfigParser::TokenTypeAsString(TokenType type) {
  switch (type) {
    case TOKEN_TYPE_START: return "TOKEN_TYPE_START";
    case TOKEN_TYPE_NORMAL: return "TOKEN_TYPE_NORMAL";
    case TOKEN_TYPE_START_BLOCK: return "TOKEN_TYPE_START_BLOCK";
    case TOKEN_TYPE_END_BLOCK: return "TOKEN_TYPE_END_BLOCK";
    case TOKEN_TYPE_COMMENT: return "TOKEN_TYPE_COMMENT";
    case TOKEN_TYPE_STATEMENT_END: return "TOKEN_TYPE_STATEMENT_END";
    case TOKEN_TYPE_EOF: return "TOKEN_TYPE_EOF";
    case TOKEN_TYPE_ERROR: return "TOKEN_TYPE_ERROR";
    case TOKEN_TYPE_QUOTED_STRING: return "TOKEN_TYPE_QUOTED_STRING";
    default: return "UNKNOWN";
  }
}

NginxConfigParser::TokenType NginxConfigParser::ParseToken(std::istream* input, std::string* value) {
  TokenParserState state = TOKEN_STATE_INITIAL_WHITESPACE;
  while (input->good()) {
    const char c = input->get();
    if (!input->good()) break;

    switch (state) {
      case TOKEN_STATE_INITIAL_WHITESPACE:
        switch (c) {
          case '{': *value = c; return TOKEN_TYPE_START_BLOCK;
          case '}': *value = c; return TOKEN_TYPE_END_BLOCK;
          case '#': *value = c; state = TOKEN_STATE_TOKEN_TYPE_COMMENT; continue;
          case '"': *value = c; state = TOKEN_STATE_DOUBLE_QUOTE; continue;
          case '\'': *value = c; state = TOKEN_STATE_SINGLE_QUOTE; continue;
          case ';': *value = c; return TOKEN_TYPE_STATEMENT_END;
          case ' ': case '\t': case '\n': case '\r': continue;
          default: *value += c; state = TOKEN_STATE_TOKEN_TYPE_NORMAL; continue;
        }
      case TOKEN_STATE_SINGLE_QUOTE:
      case TOKEN_STATE_DOUBLE_QUOTE:
        *value += c;
        if (c == '\\') {
        *value += c;  // Keep the backslash
        char escaped_char = input->get();
        if (input->good()) {
            *value += escaped_char;
        }
        continue;
        }
        // Return quoted string token when encountering a non-escaped closing quote.
        if (((state == TOKEN_STATE_SINGLE_QUOTE && c == '\'') ||
             (state == TOKEN_STATE_DOUBLE_QUOTE && c == '"')) &&
            (value->length() < 2 || value->at(value->length() - 2) != '\\')) {
          return TOKEN_TYPE_QUOTED_STRING;
        }
        continue;
      case TOKEN_STATE_TOKEN_TYPE_COMMENT:
        if (c == '\n' || c == '\r') return TOKEN_TYPE_COMMENT;
        *value += c; 
        continue;
      case TOKEN_STATE_TOKEN_TYPE_NORMAL:
        if (c == ' ' || c == '\t' || c == '\n' || c == ';' || c == '{' || c == '}') {
          input->unget(); 
          return TOKEN_TYPE_NORMAL;
        }
        *value += c; 
        continue;
    }
  }

  // If we get here, we reached the end of the file.
   if (state == TOKEN_STATE_SINGLE_QUOTE || state == TOKEN_STATE_DOUBLE_QUOTE){
    return TOKEN_TYPE_ERROR;
   }

  return TOKEN_TYPE_EOF;
}

bool NginxConfigParser::Parse(std::istream* input, NginxConfig* config) {
  LOG_INFO << "Starting config file parsing...";
  
  std::stack<NginxConfig*> config_stack;
  config_stack.push(config);
  TokenType last_token_type = TOKEN_TYPE_START;
  TokenType token_type;
  int open_blocks = 0;

  while (true) {
    std::string token;
    token_type = ParseToken(input, &token);

    if (token_type == TOKEN_TYPE_ERROR) {
      LOG_WARNING << "Invalid token encountered: " << token;
      break;
    }

    if (token_type == TOKEN_TYPE_COMMENT) {
      continue;
    }

    if (token_type == TOKEN_TYPE_START) {
      LOG_WARNING << "Unexpected start of token stream";
      break;
    } else if (token_type == TOKEN_TYPE_NORMAL || token_type == TOKEN_TYPE_QUOTED_STRING) {
      if (last_token_type == TOKEN_TYPE_START ||
          last_token_type == TOKEN_TYPE_STATEMENT_END ||
          last_token_type == TOKEN_TYPE_START_BLOCK ||
          last_token_type == TOKEN_TYPE_END_BLOCK ||
          last_token_type == TOKEN_TYPE_NORMAL ||
          last_token_type == TOKEN_TYPE_QUOTED_STRING) {
        if (last_token_type != TOKEN_TYPE_NORMAL && last_token_type != TOKEN_TYPE_QUOTED_STRING) {
          config_stack.top()->statements_.emplace_back(new NginxConfigStatement);
        }
        config_stack.top()->statements_.back()->tokens_.push_back(token);
      } else {
        LOG_WARNING << "Unexpected token '" << token << "' after " << TokenTypeAsString(last_token_type);
        break;
      }
    } else if (token_type == TOKEN_TYPE_STATEMENT_END) {
      if (last_token_type != TOKEN_TYPE_NORMAL && last_token_type != TOKEN_TYPE_QUOTED_STRING) {
        LOG_WARNING << "Semicolon found without a valid statement";
        break;
      }
    } else if (token_type == TOKEN_TYPE_START_BLOCK) {
      if (last_token_type != TOKEN_TYPE_NORMAL && last_token_type != TOKEN_TYPE_QUOTED_STRING) {
        LOG_WARNING << "Block opened without a valid statement name";
        break;
      }
      NginxConfig* const new_config = new NginxConfig;
      config_stack.top()->statements_.back()->child_block_.reset(new_config);
      config_stack.push(new_config);
      open_blocks++;
    } else if (token_type == TOKEN_TYPE_END_BLOCK) {
      if (last_token_type != TOKEN_TYPE_STATEMENT_END &&
        last_token_type != TOKEN_TYPE_END_BLOCK &&
        last_token_type != TOKEN_TYPE_COMMENT &&
        last_token_type != TOKEN_TYPE_START_BLOCK) {
        LOG_WARNING << "Error: Closing block '}' without a valid statement or matching '{'";
        break;
      }
      config_stack.pop();
      open_blocks--;
    } else if (token_type == TOKEN_TYPE_EOF) {
      if (open_blocks != 0) {
        LOG_WARNING << "Mismatched braces. Some blocks were not closed.";
        break;
      }
      if (last_token_type != TOKEN_TYPE_STATEMENT_END && last_token_type != TOKEN_TYPE_END_BLOCK) {
        LOG_WARNING << "File ended unexpectedly after " << TokenTypeAsString(last_token_type);
        break;
      }
      LOG_INFO << "Configuration file parsed successfully.";
      return true;
    } else {
      LOG_WARNING << "Unknown token encountered: " << token;
      break;
    }

    last_token_type = token_type;
  }

  LOG_WARNING << "Bad transition from " << TokenTypeAsString(last_token_type)
               << " to " << TokenTypeAsString(token_type);
  return false;
}


bool NginxConfigParser::Parse(const char* file_name, NginxConfig* config) {
  std::ifstream file(file_name);
  if (!file.good()) { 
    LOG_WARNING << "Failed to open config file: " << file_name;
    return false;
  }
  bool success = Parse(&file, config);
  file.close();
  return success;
}
