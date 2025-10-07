#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "request.h"
#include "response.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <memory>

// Abstract base class for request handlers
class RequestHandler { 
public:
    // Creates a callable function that takes in an unordered_map<std::string, std::string> as its parameter and returns a std::unique_ptr<RequestHandler>
    using Factory = std::function<std::unique_ptr<RequestHandler>(const std::unordered_map<std::string, std::string>&)>; 
    virtual ~RequestHandler() {}

    // Handles an HTTP request and returns a response.
    // @param req: the incoming HTTP request.
    // @return: a unique pointer to the response object to be sent back to the client.
    virtual std::unique_ptr<response> handle_request(const request& req) = 0;
};

#endif