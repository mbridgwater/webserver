#ifndef SESSION_H
#define SESSION_H

#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include "echo_handler.h"
#include "static_file_handler.h"
#include "config_interpreter.h"
#include "request_handler_factory.h"
#include <map>
#include "trie.h" 

using boost::asio::ip::tcp;
using namespace boost::placeholders;

class session{
    public:
        // Constructs a session with a socket and handler map.
        // @param io_service: Boost I/O service.
        // @param handlers: URI prefix to handler mapping.
        explicit session(boost::asio::io_service& io_service, TrieNode* trie_root, RequestHandlerFactory& factory);

        
        // Returns a reference to the session's socket.
        // @return: reference to the socket.
        tcp::socket& socket();

        // Begins reading data from the client asynchronously.
        void start();

        // Sets the client's IP address for logging/debugging purposes.
        // @param ip: client's IP address.
        void set_client_ip(const std::string& ip);


    private:
        // Handles async reads from the client socket.
        // Parses a complete HTTP request and dispatches to the correct handler.
        // Serializes the response, and sends it back to the client.
        // On error or disconnect, logs and cleans up the session.
        // @param error: error code from the read operation.
        // @param bytes_transferred: number of bytes read.
        void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
        
        // Handles the completion of the asynchronous write operation to the client
        // by closing socket and deleting session
        // @param error: error code from the write operation.
        void handle_write(const boost::system::error_code& error);

        tcp::socket socket_; // Socket for communicating with the client.
        enum { max_length = 1024 }; // Max size for reading chunks of request data.
        std::string client_ip_; // IP address of the connected client.
        char data_[max_length]; // Buffer for reading incoming data.
        std::string request_buffer_; // Accumulates incoming data to form full HTTP requests.
        TrieNode* trie_root_;
        RequestHandlerFactory& factory_; // Factory for creating request handlers.
};

#endif // SESSION_H