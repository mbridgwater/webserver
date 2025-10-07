//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <csignal>
#include "server.h"
#include "nginx_config_parser.h"
#include "config_interpreter.h"
#include "logger.h"
#include "request_handler_factory.h"
#include "trie.h"
#include "not_found_handler.h"
#include "crud_handler.h"
#include "health_handler.h"
#include "sleep_handler.h"
#include "quiz_handler.h"
#include "result_handler.h"
#include "create_quiz_handler.h"


using boost::asio::ip::tcp;
using namespace boost::placeholders;

// Signal handler for graceful shutdown
void signal_handler(int signal) {
  if (signal == SIGINT) {
    LOG_INFO << "Server terminated by SIGINT (Ctrl+C)";
  } else {
    LOG_INFO << "Server terminated by signal: " << signal;
  }
  std::exit(0);
}

int main(int argc, char* argv[])
{
  Logger::init();
  try
  {
    if (argc != 2)
    {
      LOG_WARNING << "Usage: ./server_main <config_file>";
      return 1;
    }

    // Process config file
    NginxConfig config;
    try{
      std::ifstream config_file(argv[1]);
      process_config_file(config_file, config); 
      LOG_DEBUG << "Successfully opened and parsed config file"; 
    } catch (const std::exception& e) {
      LOG_WARNING << "Config error during file parsing: " << e.what();
      return 1;
    }
    
    // Register factories 
    RequestHandlerFactory factory;

    factory.register_factory("EchoHandler", &EchoHandler::create);
    factory.register_factory("StaticFileHandler", &StaticFileHandler::create);
    factory.register_factory("HealthHandler", HealthHandler::create);
    

    // Register 404 fallback
    factory.register_factory("NotFoundHandler", &NotFoundHandler::create);
    
    // CRUD
    factory.register_factory("CrudHandler", &CrudHandler::create);

    // Quizzes
    factory.register_factory("QuizHandler", &QuizHandler::create);
    factory.register_factory("ResultHandler", &ResultHandler::create);
    factory.register_factory("CreateQuizHandler", &CreateQuizHandler::create);

    // Multithreading
    factory.register_factory("SleepHandler", &SleepHandler::create);

    // Extract required config values
    std::vector<ConfigStruct> handler_configs;
    short port;
    try {
      // Extract port number from config
      port = find_listen_port(&config);
      LOG_DEBUG << "Port extracted from config: " << port;

      // Extract handler config structs 
      handler_configs = extract_handler_configs(&config);

    } catch (const std::exception& e) {
      LOG_WARNING << "Config extraction error: " << e.what();
      return 1;
    }

    // Build TRIE from handler_configs
    TrieNode* trie_root = new TrieNode();
    for (auto& config : handler_configs) {
      trie_root->insert(config.uri, &config);
      LOG_INFO << "Adding config";
      LOG_INFO << "URI: " << config.uri;
      LOG_INFO << "Handler: " << config.handler;
    }


    // Register signal handlers for graceful termination
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Start server 
    boost::asio::io_service io_service;

    LOG_DEBUG << "Creating server on port " << port;

    server s(io_service, port, trie_root, factory);

    LOG_INFO << "Server started, listening on port " << port;

    io_service.run();
    LOG_INFO << "io_service run loop exited. Server shutting down.";
  }
  catch (std::exception& e)
  {
    LOG_WARNING << "Unhandled exception in main: " << e.what();
  }

  return 0;
}
