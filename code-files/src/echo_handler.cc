#include "echo_handler.h"
#include <string>
#include "request.h"
#include "response.h"

std::unique_ptr<RequestHandler> EchoHandler::create(const std::unordered_map<std::string, std::string>& args) {
        return std::make_unique<EchoHandler>();
}

std::unique_ptr<response> EchoHandler::handle_request(const request& req) {
    auto res = std::make_unique<response>();
    res->http_version = "HTTP/1.1";
    res->status_code = 200;
    res->reason_phrase = "OK";
    res->headers["Content-Type"] = "text/plain";

    // Echo back the full request
    std::string body = req.method + " " + req.uri + " " + req.http_version + "\r\n";

    for (const auto& header : req.headers) {
        body += header.first + ": " + header.second + "\r\n";
    }
    body += "\r\n" + req.body;

    res->body = body;
    res->headers["Content-Length"] = std::to_string(res->body.size());

    return res;
}
