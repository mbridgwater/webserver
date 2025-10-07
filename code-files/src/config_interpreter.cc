#include "config_interpreter.h"
#include "nginx_config_parser.h"
#include <stdexcept>
#include <iostream>
#include "logger.h"

void process_config_file(std::ifstream& config_file, NginxConfig& config) {
  NginxConfigParser parser;
  // Make sure config can be opened 
  if (!config_file.is_open()) {
    throw std::runtime_error("Failed to open config file.");
  }
  // Make sure config can be parsed 
  if (!parser.Parse(&config_file, &config)) {
    throw std::runtime_error("Failed to parse config file.");
  }
}

std::string find_value_for_key(const NginxConfig* config_block, const std::string& key) {
  for (const auto& statement : config_block->statements_) {
    // Check if the statement is: key <value>;
    if (statement->tokens_.size() == 2 && statement->tokens_[0] == key) {
      return statement->tokens_[1];
    }
    // Recurse into nested blocks
    if (statement->child_block_) {
      try {
        return find_value_for_key(statement->child_block_.get(), key);
      } catch (const std::runtime_error&) {
        // Keep searching other branches
        continue;
      }
    }
  }
  // If not found in this block or any children
  throw std::runtime_error("No valid " + key + " directive found in config.");
}

short find_listen_port(const NginxConfig* config_block) {
  std::string port_str = find_value_for_key(config_block, "listen");
  try {
    return static_cast<short>(std::stoi(port_str));
  } catch (...) {
    throw std::runtime_error("Invalid port number format in 'listen' directive.");
  }
}

std::vector<ConfigStruct> extract_handler_configs(const NginxConfig* config_block){
  std::vector<ConfigStruct> handler_configs;

  for (const auto& statement : config_block->statements_) {

        // Print out statement size and value to logger 
        LOG_DEBUG << "Statement has " << statement->tokens_.size() << " tokens: ";
        for (const auto& token : statement->tokens_) {
            LOG_DEBUG << token << " ";
        }

        // Check if the statement is: location <uri> <handler> 
        if (statement->tokens_.size() >= 2 && statement->tokens_[0] == "location") {
            ConfigStruct config;

            // Check if URI has trailing slash and remove if it does 
            std::string uri = statement->tokens_[1];
            if (!uri.empty() && uri.back() == '/') {
                throw std::runtime_error("Trailing slash found in URI: " + uri + ". Removing trailing slash.");
            }
            config.uri = uri;
            
            config.handler = statement->tokens_[2];

            // Parse out unique arguments needed for each handler constructor 
            if (config.handler == "StaticFileHandler" )
            {
              config.args["mount_point"] = config.uri;
              if (statement->child_block_) {
                config.args["doc_root"] = find_value_for_key(statement->child_block_.get(), "root");
              }
              else{
                throw std::runtime_error("StaticFileHandler is incorrectly configured");
              }
              handler_configs.push_back(config); 
            }
            else if (config.handler == "EchoHandler")
            {
              handler_configs.push_back(config); 
            }
            else if (config.handler == "CrudHandler") 
            {
              if (statement->child_block_) {
                config.args["data_path"] = find_value_for_key(statement->child_block_.get(), "data_path");
              } else {
                throw std::runtime_error("CrudHandler requires a child block with a 'data_path' directive.");
              }
              handler_configs.push_back(config);
            }
            else if (config.handler == "HealthHandler")
            {
              // No special args needed
                handler_configs.push_back(config);  
            }
            else if (config.handler == "SleepHandler")
            {
              // No special args needed
              handler_configs.push_back(config);  
            }
            else if (config.handler == "QuizHandler") {
              config.args["quiz_root"] = find_value_for_key(statement->child_block_.get(), "quiz_root");
              handler_configs.push_back(config);
            }
            else if (config.handler == "ResultHandler") {
              config.args["quiz_root"] = find_value_for_key(statement->child_block_.get(), "quiz_root");
              handler_configs.push_back(config);
            }
            else if (config.handler == "CreateQuizHandler") 
            {
              if (statement->child_block_) {
                config.args["quiz_root"] = find_value_for_key(statement->child_block_.get(), "quiz_root");
              } else {
                throw std::runtime_error("CreateQuizHandler requires a child block with a 'quiz_root' directive.");
              }
              handler_configs.push_back(config);
            }
            else {
              throw std::runtime_error("Handler Type " + config.handler + " not supported");
            }
        }
        
  }
   return handler_configs;
}

std::map<std::string, ConfigStruct> create_uri_to_config_map(const std::vector<ConfigStruct>& handler_configs) {
    std::map<std::string, ConfigStruct> uri_to_config_map;

    for (const auto& config : handler_configs) {
      if (uri_to_config_map.find(config.uri) != uri_to_config_map.end()) {
        throw std::runtime_error("Server failed to start. Duplicate locations defined in config.");
      }
      uri_to_config_map[config.uri] = config;
      LOG_INFO << "Adding config";
      LOG_INFO << "URI: " << config.uri;
      LOG_INFO << "Handler: " << config.handler;
    }

    return uri_to_config_map;
}