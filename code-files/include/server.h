#ifndef SERVER_H
#define SERVER_H

#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include "session.h"
#include "config_interpreter.h"
#include "request_handler_factory.h"
#include <map>
#include "trie.h" 

using boost::asio::ip::tcp;
using namespace boost::placeholders;

class server{
    public:
    // Initializes the server, binds to the given port, and starts accepting connections.
    // @param io_service: Boost I/O service for asynchronous ops.
    // @param port: port to listen on.
    // @param handlers: map of URI prefixes to request handlers.
    server(boost::asio::io_service& io_service, short port, TrieNode* trie_root, RequestHandlerFactory& factory);

    private:

    // Begins asynchronously accepting a new incoming client connection and starts a session.
    void start_accept();

    // Handles the result of an asynchronous accept operation. Deletes session upon failure.
    // @param new_session: pointer to the accepted session.
    // @param error: error code indicating success or failure.
    void handle_accept(session* new_session, const boost::system::error_code& error);

    boost::asio::io_service& io_service_; // Reference to the Boost I/O service for managing async operations.
    tcp::acceptor acceptor_; // Accepts incoming TCP connections on the bound port.

    TrieNode* trie_root_; // Maps URI prefixes to corresponding request handler configs.
    RequestHandlerFactory& factory_; // Factory for creating request handlers.
};

#endif // SERVER_H