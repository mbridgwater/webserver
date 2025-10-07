#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <thread>
#include <string>
#include "session.h"
#include "request_handler_factory.h"
#include "trie.h"
#include "not_found_handler.h"
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/core.hpp>

using boost::asio::ip::tcp;

class SessionTestFixture : public ::testing::Test {
protected:
  short test_port;
  std::thread server_thread;
  boost::asio::io_context io_context;
  tcp::socket socket;

  SessionTestFixture() : socket(io_context) {}

  void SetUp() override {
    // Assign a unique-ish port (test isolation)
    static short base_port = 12350;
    test_port = base_port++;

    // Launch server in background 
    // This does NOT use server.cc 
    server_thread = std::thread([this]() {
      boost::asio::io_context server_io;
      tcp::acceptor acceptor(server_io, tcp::endpoint(tcp::v4(), test_port));

      TrieNode* trie_root = new TrieNode();

      ConfigStruct* echoConfig = new ConfigStruct;
      echoConfig->uri = "/echo";
      echoConfig->handler = "EchoHandler";
      trie_root->insert(echoConfig->uri, echoConfig);

      ConfigStruct* staticConfig = new ConfigStruct;
      staticConfig->uri = "/static";
      staticConfig->handler = "StaticFileHandler";
      staticConfig->args["mount_point"] = "/static/";
      staticConfig->args["doc_root"] = "../src/app";
      trie_root->insert(staticConfig->uri, staticConfig);

      ConfigStruct* static1Config = new ConfigStruct;
      static1Config->uri = "/static1";
      static1Config->handler = "StaticFileHandler";
      static1Config->args["mount_point"] = "/static1/";
      static1Config->args["doc_root"] = "../src/app1";
      trie_root->insert(static1Config->uri, static1Config);

      ConfigStruct* staticLongConfig = new ConfigStruct;
      staticLongConfig->uri = "/static-long";
      staticLongConfig->handler = "StaticFileHandler";
      staticLongConfig->args["mount_point"] = "/static-long/";
      staticLongConfig->args["doc_root"] = "../tests/app_long";
      trie_root->insert(staticLongConfig->uri, staticLongConfig);

      ConfigStruct* staticLongerConfig = new ConfigStruct;
      staticLongerConfig->uri = "/static-longer";
      staticLongerConfig->handler = "StaticFileHandler";
      staticLongerConfig->args["mount_point"] = "/static-longer/";
      staticLongerConfig->args["doc_root"] = "../tests/app_long/app_longer";
      trie_root->insert(staticLongerConfig->uri, staticLongerConfig);



      RequestHandlerFactory factory;
      factory.register_factory("EchoHandler", &EchoHandler::create);
      factory.register_factory("StaticFileHandler", &StaticFileHandler::create);
      factory.register_factory("NotFoundHandler", &NotFoundHandler::create);

      // Move factory and trie_root into lambda capture
      acceptor.async_accept([&server_io, trie_root, &factory](const boost::system::error_code& error, tcp::socket peer_socket) {
          if (!error) {
              auto new_session = new session(server_io, trie_root, factory);
              new_session->socket() = std::move(peer_socket);
              new_session->start();
          }
      });

      server_io.run();
    });

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Connect client
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("127.0.0.1", std::to_string(test_port));
    boost::asio::connect(socket, endpoints);
  }

  void TearDown() override {
    socket.close();
    if (server_thread.joinable()) {
      server_thread.join();
    }
  }

  // Sends a request string and returns the entire response string (headers + body)
  std::string sendRequestAndGetResponse(const std::string& request) {
    boost::asio::write(socket, boost::asio::buffer(request));

    boost::asio::streambuf response_buf;
    boost::asio::read_until(socket, response_buf, "\r\n\r\n"); 

    std::istream response_stream(&response_buf);
    std::string response;
    std::getline(response_stream, response); // status line

    // Read headers
    std::string header;
    while (std::getline(response_stream, header) && header != "\r") {
      response += "\n" + header;
    }

    // Read body if available
    std::string body;
    if (std::getline(response_stream, body)) {
      response += "\n\n" + body;
    }

    return response;
  }
};

// --------- Happy path tests ---------

// Responds to correct HTTP request with correct response 
// Expected result: PASS
TEST_F(SessionTestFixture, EchoesHttpRequest) {
  const std::string full_request = "GET /echo HTTP/1.1\r\nHost: localhost\r\n\r\n";
  std::string response = sendRequestAndGetResponse(full_request);
  std::cout << "Response:\n" << response << std::endl;


  EXPECT_TRUE(response.find("200 OK") != std::string::npos);
  EXPECT_TRUE(response.find("Content-Type: text/plain") != std::string::npos);
  EXPECT_TRUE(response.find("GET /echo HTTP/1.1") != std::string::npos);
  // EXPECT_TRUE(response.find("Host: localhost") != std::string::npos);
}

// Continues waiting for an incomplete request 
// Expected result: PASS
TEST_F(SessionTestFixture, HandlesIncompleteHeaderGracefully) {
  const std::string partial_request = "GET /echo HTTP/1.1\r\nHost: localhost\r\n";
  boost::asio::write(socket, boost::asio::buffer(partial_request));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  boost::asio::socket_base::bytes_readable command(true);
  socket.io_control(command);

  EXPECT_EQ(command.get(), 0); // no response yet
}

// Test that /static/hello_world.html returns correct response
// Expected result: PASS
TEST_F(SessionTestFixture, ServesStaticHtmlFile) {
  const std::string request =
      "GET /static/hello_world.html HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "\r\n";
  std::string response = sendRequestAndGetResponse(request);
  std::cout << "Response:\n" << response << std::endl;

  EXPECT_TRUE(response.find("200 OK") != std::string::npos);
  EXPECT_TRUE(response.find("Content-Type: text/html") != std::string::npos);
  EXPECT_TRUE(response.find("<!DOCTYPE html>") != std::string::npos);
}

// Test that /static1/index.txt returns correct response
// Expected result: PASS
TEST_F(SessionTestFixture, ServesStaticTxtFromAlternateMount) {
  const std::string request =
      "GET /static1/index.txt HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "\r\n";
  std::string response = sendRequestAndGetResponse(request);
  std::cout << "Response:\n" << response << std::endl;

  EXPECT_TRUE(response.find("200 OK") != std::string::npos);
  EXPECT_TRUE(response.find("Content-Type: text/plain") != std::string::npos);
  EXPECT_TRUE(response.find("Hi! This is Natalie. ") != std::string::npos);
}

// Responds with 404 Not Found for an unmatched URI
// Expected result: PASS
TEST_F(SessionTestFixture, RespondsWith404ForUnknownRoute) {
  const std::string request =
      "GET /nonexistent/path HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "\r\n";
  std::string response = sendRequestAndGetResponse(request);
  std::cout << "Response:\n" << response << std::endl;

  EXPECT_TRUE(response.find("404 Not Found") != std::string::npos);
}

// Matches longest prefix when two similar paths exist
// Expected result: /static-longer is chosen over /static-long
TEST_F(SessionTestFixture, LongestPrefixWins) {
  const std::string request =
      "GET /static-longer/file.txt HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "\r\n";
  std::string response = sendRequestAndGetResponse(request);
  std::cout << "Response:\n" << response << std::endl;

  EXPECT_TRUE(response.find("200 OK") != std::string::npos);
  EXPECT_TRUE(response.find("This is from static-longer") != std::string::npos);
}

// Logs ResponseMetrics Line
// Expected result: PASS. Correct information is given in log. 
TEST_F(SessionTestFixture, LogsResponseMetricsLine) {
  using namespace boost::log;

  // Create an ostringstream to capture output
  auto string_stream = std::make_shared<std::ostringstream>();

  // Create sink to write to the ostringstream
  using text_sink = sinks::synchronous_sink<sinks::text_ostream_backend>;
  boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();

  // Wrap the ostringstream in a std::ostream (not make_shared!)
  sink->locked_backend()->add_stream(boost::shared_ptr<std::ostream>(string_stream.get(), [](std::ostream*) {}));

  // Set the formatter to output just the message
  sink->set_formatter(expressions::stream << expressions::smessage);

  // Add and activate the sink
  core::get()->add_sink(sink);

  // Send request to trigger logging
  const std::string full_request = "GET /echo HTTP/1.1\r\nHost: localhost\r\n\r\n";
  std::string response = sendRequestAndGetResponse(full_request);

  // Flush and remove the sink
  sink->flush();
  core::get()->remove_sink(sink);

  // Inspect the captured logs
  std::string logs = string_stream->str();
  std::cout << "Captured logs:\n" << logs << std::endl;

  EXPECT_NE(logs.find("[ResponseMetrics]"), std::string::npos);
  EXPECT_NE(logs.find("code=200"), std::string::npos);
  EXPECT_NE(logs.find("path=/echo"), std::string::npos);
  EXPECT_NE(logs.find("handler=EchoHandler"), std::string::npos);
}