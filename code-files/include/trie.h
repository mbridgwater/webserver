// trie.h
#ifndef TRIE_H
#define TRIE_H

#include <map>
#include <string>
#include <memory>
#include "config_interpreter.h"

class TrieNode {
public:
    std::map<std::string, std::shared_ptr<TrieNode>> children;
    ConfigStruct* config = nullptr;

    void insert(const std::string& path, ConfigStruct* config_ptr);
    ConfigStruct* find(const std::string& uri);
};

#endif // TRIE_H