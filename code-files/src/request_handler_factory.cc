#include "request_handler_factory.h"
#include <unordered_map>
#include <string>
#include <memory>

void RequestHandlerFactory::register_factory(const std::string& handler_name, RequestHandler::Factory factory) {
        factory_map_[handler_name] = factory;
}

// Create a handler instance based on the handler type and arguments
std::unique_ptr<RequestHandler> RequestHandlerFactory::create_handler(const std::string& handler_name, 
                                                const std::unordered_map<std::string, std::string>& args) {
    auto it = factory_map_.find(handler_name);
    if (it != factory_map_.end()) {
        return it->second(args);
    }
    return nullptr;
}