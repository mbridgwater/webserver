#ifndef CRUD_HANDLER_H
#define CRUD_HANDLER_H

#include "request_handler.h"
#include "logger.h" 
#include "file_system.h"
#include "file_system_interface.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class CrudHandler : public RequestHandler {
public:
    CrudHandler(std::shared_ptr<FileSystemInterface> file_system);

    static std::unique_ptr<RequestHandler> create(const std::unordered_map<std::string, std::string>& args); 

    virtual std::unique_ptr<response> handle_request(const request& req) override;
private:
    // Holds the root directory for where CRUD handler stores files
    std::shared_ptr<FileSystemInterface> file_system_; 

    // Request Actions
    std::unique_ptr<response> post(const request& req, const std::string& name);
    std::unique_ptr<response> get(const std::string& name, const std::string& id);
    std::unique_ptr<response> put(const request& req, const std::string& name, const std::string& id);
    std::unique_ptr<response> delete_req(const std::string& name, const std::string& id);
};

#endif
