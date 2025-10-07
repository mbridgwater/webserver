#ifndef ECHO_HANDLER_H
#define ECHO_HANDLER_H

#include "request_handler.h"

class EchoHandler : public RequestHandler {
public:
    // Generates a plain-text response that echoes the full HTTP request.
    // @param req: the incoming HTTP request.
    // @return: unqiue pointer to HTTP 200 OK response with the request echoed in the body.
    virtual std::unique_ptr<response> handle_request(const request& req) override;

    // Factory method 
    // @param args: a map of configuration arguments (EchoHandler does not use any specific arguments)
    // @return: a unique pointer to a newly created EchoHandler instance
    static std::unique_ptr<RequestHandler> create(const std::unordered_map<std::string, std::string>& args);
};

#endif // ECHO_HANDLER_H