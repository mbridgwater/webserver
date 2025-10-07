#ifndef NGINX_CONFIG_H
#define NGINX_CONFIG_H

#include <memory>
#include <string>
#include <vector>

class NginxConfig;

// The parsed representation of a single statement in the config file.
// Each statement has tokens and may contain a nested block.
class NginxConfigStatement {
 public:
  // Converts the statement and optional nested block into a formatted string.
  // @param depth: indentation level.
  // @return: string representation of the statement.
  std::string ToString(int depth = 0) const;

  std::vector<std::string> tokens_;
  std::unique_ptr<NginxConfig> child_block_;
};

// The parsed representation of the entire config.
// Represents a block of statements in the config file.
// Top-level config and nested blocks are instances of this.
class NginxConfig {
 public:
  // Serializes all config statements in the entire config block 
  // (including nested statements) recursively.
  // @param depth: indentation level.
  // @return: string representation of the config block.
  std::string ToString(int depth = 0) const;

  std::vector<std::unique_ptr<NginxConfigStatement>> statements_;
};

#endif // NGINX_CONFIG_H