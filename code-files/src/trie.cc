// trie.cc
#include "trie.h"
#include <sstream>

// Adds a URI path and its associated config to the trie
// Example: "/static/hello" will be split into "static" and "hello" and stored in the trie
void TrieNode::insert(const std::string& path, ConfigStruct* config_ptr) {
    std::istringstream iss(path);
    std::string token;
    TrieNode* node = this;

    while (getline(iss, token, '/')) {
        // Skip empty tokens (from leading '/')
        if (token.empty()) {
            continue;
        }
        // Create new child if it doesn't exist
        if (!node->children[token]) {
            node->children[token] = std::make_shared<TrieNode>();
        }
        // Move down the trie
        node = node->children[token].get();
    }
    // Store the config at the final node
    node->config = config_ptr;
}

// Finds the best matching (longest prefix) config for a given URI.
// Example: "/static/hello/file.txt" would match "/static/hello" if it's in the trie.
ConfigStruct* TrieNode::find(const std::string& uri) {
    // Strip query string if present
    std::string path = uri;
    size_t query_pos = path.find('?');
    if (query_pos != std::string::npos) {
        path = path.substr(0, query_pos);  // Only keep /quiz/submit
    }

    std::istringstream iss(path);
    std::string token;
    TrieNode* node = this;
    ConfigStruct* last_config = nullptr;

    while (getline(iss, token, '/')) {
        if (token.empty()) {
            continue;
        }
        // Stop if the path doesn't exist in the trie
        if (node->children.find(token) == node->children.end()) {
            break;
        }
        node = node->children[token].get();
        // Keep track of the last valid config we saw
        if (node->config) {
            last_config = node->config;
        }
    }

    return last_config;
}