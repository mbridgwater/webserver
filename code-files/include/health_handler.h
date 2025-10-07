#ifndef HEALTH_HANDLER_H
#define HEALTH_HANDLER_H

#include "request_handler.h"

class HealthHandler : public RequestHandler {
public:
    // Default 
    HealthHandler() = default;

    // Factory method
    static std::unique_ptr<RequestHandler> create(const std::unordered_map<std::string, std::string>& args);

    // Always returns 200 OK with "OK" body
    virtual std::unique_ptr<response> handle_request(const request& req) override;
};

#endif // HEALTH_HANDLER_H
