#include "health_handler.h"
#include "response.h"

std::unique_ptr<RequestHandler> HealthHandler::create(const std::unordered_map<std::string, std::string>&) {
    return std::make_unique<HealthHandler>();
}

// Always returns 200 OK with "OK" body
std::unique_ptr<response> HealthHandler::handle_request(const request& req) {
    auto resp = std::make_unique<response>();
    resp->http_version = req.http_version;
    resp->status_code = 200;
    resp->reason_phrase = "OK";
    resp->headers["Content-Type"] = "text/plain";
    resp->body = "OK";
    resp->headers["Content-Length"] = std::to_string(resp->body.size());
    return resp;
}
