#include "not_found_handler.h"

// Returns a unique_ptr to a new NotFoundHandler instance
// Args are unused, but included for API consistency
std::unique_ptr<RequestHandler> NotFoundHandler::create(const std::unordered_map<std::string, std::string>& args) {
    return std::make_unique<NotFoundHandler>();
}

// Handles an incoming request and returns a 404 Not Found response.
std::unique_ptr<response> NotFoundHandler::handle_request(const request& req) {
    auto resp = std::make_unique<response>();
    resp->http_version = "HTTP/1.1";
    resp->status_code = 404;
    resp->reason_phrase = "Not Found"; 
    resp->body = "404 Not Found";
    resp->headers["Content-Length"] = std::to_string(resp->body.size());
    resp->headers["Content-Type"] = "text/plain";
    return resp;
}