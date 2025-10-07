#include "server.h"
#include "logger.h" 
#include "trie.h"

server::server(boost::asio::io_service& io_service, short port, TrieNode* trie_root, RequestHandlerFactory& factory)
: io_service_(io_service),
  acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
  trie_root_(trie_root),
  factory_(factory)
{
    LOG_INFO << "Server starting on port " << port;
    start_accept();
}


void server::start_accept()
{
    session* new_session = new session(io_service_, trie_root_, factory_);
    LOG_DEBUG << "Waiting for incoming connections...";
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&server::handle_accept, this, new_session,
            boost::asio::placeholders::error));
}

void server::handle_accept(session* new_session,
    const boost::system::error_code& error)
{
    if (!error)
    {
        // Capture the client IP address after accept
        std::string client_ip = new_session->socket().remote_endpoint().address().to_string();
        LOG_INFO << "Accepted new connection from: " << client_ip;

        // Pass the captured client IP to the session
        new_session->set_client_ip(client_ip);
        // each session runs in its own thread, allows for concurrency
        std::thread([new_session]() {
            new_session->start();
        }).detach();
    }
    else
    {
        LOG_WARNING << "Failed to accept connection: " << error.message();
        delete new_session;
    }

    start_accept();
}

