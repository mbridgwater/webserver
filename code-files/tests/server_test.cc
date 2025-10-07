#include "gtest/gtest.h"
#include "server.h"
#include "trie.h"
#include "echo_handler.h"
#include "request_handler_factory.h"
#include <thread>
#include <boost/asio.hpp>
#include <chrono>

// --------- Happy path tests ---------

// Server binds to a valid port
TEST(ServerFeatureTest, BindsToValidPort) {
    boost::asio::io_service io_service;

    // Set up trie and dummy config
    TrieNode* trie_root = new TrieNode();
    ConfigStruct config;
    config.uri = "/test";
    config.handler = "EchoHandler";
    trie_root->insert(config.uri, &config);

    RequestHandlerFactory factory;
    factory.register_factory("EchoHandler", &EchoHandler::create);

    EXPECT_NO_THROW({
        server s(io_service, 8080, trie_root, factory);
    });
}

// Server accepts a connection from a client
TEST(ServerFeatureTest, AcceptsClientConnection) {
    boost::asio::io_service io_service;

    TrieNode* trie_root = new TrieNode();
    ConfigStruct config;
    config.uri = "/test";
    config.handler = "EchoHandler";
    trie_root->insert(config.uri, &config);

    RequestHandlerFactory factory;
    factory.register_factory("EchoHandler", &EchoHandler::create);

    server s(io_service, 9090, trie_root, factory);

    // Start server loop in a separate thread
    std::thread server_thread([&io_service]() {
        io_service.run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Connect a client
    boost::asio::io_service client_io_service;
    boost::asio::ip::tcp::socket socket(client_io_service);
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::address::from_string("127.0.0.1"), 9090);
    socket.connect(endpoint);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    io_service.stop();
    server_thread.join();
}
