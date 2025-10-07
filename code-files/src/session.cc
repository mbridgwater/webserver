#include "session.h"
#include "logger.h"
#include "request.h"
#include "trie.h" 
#include "response.h"
#include "res_req_helpers.h"
#include "config_interpreter.h"
#include <iostream>
#include <set>


session::session(boost::asio::io_service& io_service, TrieNode* trie_root, RequestHandlerFactory& factory)
: socket_(io_service), trie_root_(trie_root), factory_(factory)
{}

tcp::socket& session::socket()
{
    return socket_;
}

void session::set_client_ip(const std::string& ip) {
    client_ip_ = ip;  // Set the client IP
}

void session::start()
{
    LOG_DEBUG << "Starting to read data from client: " << client_ip_;
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void session::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
    if (!error)
    {
        LOG_DEBUG << "Read " << bytes_transferred << " bytes from client: " << client_ip_;
        request_buffer_.append(data_, bytes_transferred);

        // Check for full HTTP request (basic, ends with \r\n\r\n)
        if (request_buffer_.find("\r\n\r\n") != std::string::npos) {

            LOG_DEBUG << "HTTP header received. Building response.";

            // Parse request
            request req = parse_request(request_buffer_);

            // If parse_request() returns an empty request, it's malformed
            if (req.method.empty()) {
                LOG_WARNING << "Malformed request from " << client_ip_ << " — parse failed.";

                response res;
                res.http_version = "HTTP/1.1";
                res.status_code = 400;
                res.reason_phrase = "Bad Request";
                res.headers["Content-Type"] = "text/plain";
                res.body = "Bad Request";
                res.headers["Content-Length"] = std::to_string(res.body.size());
                res.headers["Connection"] = "close";

                auto out = serialize_response(res);
                boost::asio::async_write(socket_,
                    boost::asio::buffer(out),
                    boost::bind(&session::handle_write, this,
                                boost::asio::placeholders::error));
                return;
            }
            
            // Log method, path, and client IP
            LOG_INFO << "Received " << req.method << " request for path: " << req.uri << " from " << client_ip_;

            // Trie lookup
            std::shared_ptr<RequestHandler> handler = nullptr;
            const ConfigStruct* handler_config = trie_root_->find(req.uri);

            std::unique_ptr<response> res;
            std::string handler_name;
            if (handler_config != nullptr) {
                try {
                    LOG_INFO << "Matched handler for URI prefix: " << handler_config->uri;
                    handler = factory_.create_handler(handler_config->handler, handler_config->args);
                    handler_name = handler_config->handler;
                    res = handler->handle_request(req);
                } catch (const std::exception& e) {
                    LOG_WARNING << "Failed to create handler - " << e.what();
                }
            } else {
                LOG_INFO << "No matching handler found for URI: " << req.uri << " — using NotFoundHandler";
                // Fallback to 404 NotFoundHandler
                handler = factory_.create_handler("NotFoundHandler", {});
                handler_name = "NotFoundHandler";
                res = handler->handle_request(req);
            }

            // Generate response 
            request_buffer_.clear();
            res->headers["Connection"] = "close";
            std::string out = serialize_response(*res);

            boost::asio::async_write(socket_,
                boost::asio::buffer(out),
                boost::bind(&session::handle_write, this,
                boost::asio::placeholders::error));
            
            // Log ResponseMetric
                LOG_INFO << "[ResponseMetrics] code=" << res->status_code
                << " path=" << req.uri
                << " ip=" << client_ip_
                << " handler=" << handler_name;
        }
        else {
            LOG_DEBUG << "HTTP request not complete, awaiting more data.";
            socket_.async_read_some(boost::asio::buffer(data_, max_length),
                boost::bind(&session::handle_read, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
        }
    }
    else
    {
        if (error == boost::asio::error::eof) {
            LOG_INFO << "Client " << client_ip_ << " closed the connection (EOF)";
        } else {
            LOG_WARNING << "Error reading from client: " << client_ip_ << " - Error: " << error.message();
        }
        delete this;
    }
}

void session::handle_write(const boost::system::error_code& error)
{
    if (socket_.is_open()) {
        boost::system::error_code ignored;
        socket_.shutdown(tcp::socket::shutdown_both, ignored);
        socket_.close(ignored);
    }

    LOG_INFO << "Session closed for client " << client_ip_;
    delete this;
}
