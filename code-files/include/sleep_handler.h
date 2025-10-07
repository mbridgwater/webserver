#pragma once
#include "request_handler.h"

class SleepHandler : public RequestHandler {
public:
    static std::unique_ptr<RequestHandler> create(const std::unordered_map<std::string, std::string>& config);
    std::unique_ptr<response> handle_request(const request& req) override;
};
