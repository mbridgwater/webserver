#ifndef REQUEST_HANDLER_FACTORY_H
#define REQUEST_HANDLER_FACTORY_H

#include "request_handler.h"

class RequestHandlerFactory {
public:
    // Register a factory function for a specific handler type
    void register_factory(const std::string& handler_name, RequestHandler::Factory factory);

    // Create a handler instance based on the handler type and arguments
    std::unique_ptr<RequestHandler> create_handler(const std::string& handler_name, 
                                                   const std::unordered_map<std::string, std::string>& args);

private:
    std::unordered_map<std::string, RequestHandler::Factory> factory_map_;
};

#endif // REQUEST_HANDLER_FACTORY_H