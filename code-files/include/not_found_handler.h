#ifndef NOT_FOUND_HANDLER_H
#define NOT_FOUND_HANDLER_H

#include "request_handler.h"
#include <memory>
#include <unordered_map>

class NotFoundHandler : public RequestHandler {
public:
    // args: a map of configuration arguments (not used for NotFoundHandler, but included for API consistency)
    // returns a unique pointer to the created NotFoundHandler instance
    static std::unique_ptr<RequestHandler> create(const std::unordered_map<std::string, std::string>& args);
    // req: parsed HTTP request object
    // returns a response object with 404 status code and a plain text body explaining the error
    std::unique_ptr<response> handle_request(const request& req) override;
};

#endif // NOT_FOUND_HANDLER_H