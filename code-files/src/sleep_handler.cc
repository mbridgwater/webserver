#include "sleep_handler.h"
#include <chrono>
#include <thread>

std::unique_ptr<RequestHandler> SleepHandler::create(const std::unordered_map<std::string, std::string>&) {
    return std::make_unique<SleepHandler>();
}

std::unique_ptr<response> SleepHandler::handle_request(const request& req) {
    auto res = std::make_unique<response>();
    res->http_version = "HTTP/1.1";
    res->status_code = 200;
    res->reason_phrase = "OK";
    res->headers["Content-Type"] = "text/plain";
    // blocks for a certain amount of time
    std::this_thread::sleep_for(std::chrono::seconds(3));
    res->body = "Slept for 3 seconds";
    res->headers["Content-Length"] = std::to_string(res->body.size());
    return res;
}