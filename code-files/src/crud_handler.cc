#include "crud_handler.h"
#include <filesystem>

namespace pt = boost::property_tree;
namespace fs = std::filesystem;

CrudHandler::CrudHandler(std::shared_ptr<FileSystemInterface> file_system) 
    : file_system_(file_system) {}
    
std::unique_ptr<RequestHandler> CrudHandler::create(const std::unordered_map<std::string, std::string>& args) {
    auto it = args.find("data_path");
    if (it != args.end()) {
        std::filesystem::create_directories(it->second);
        LOG_INFO << "Using data_path: " << it->second;
        return std::make_unique<CrudHandler>(std::make_shared<FileSystem>(it->second));
    }
    return nullptr;
}

std::unique_ptr<response> CrudHandler::handle_request(const request& req) {
    auto resp = std::make_unique<response>();
    resp->http_version = "HTTP/1.1";
    resp->headers["Content-Type"] = "application/json";

    // Expected URI format: /api/EntityType[/ID]
    const std::string api_prefix = "/api/";
    if (req.uri.compare(0, api_prefix.size(), api_prefix) != 0) {
        LOG_DEBUG << "Invalid API prefix in URI: " << req.uri;
        resp->status_code = 404;
        resp->reason_phrase = "Not Found";
        resp->body = "Invalid API prefix";
        return resp;
    }

    // Strip "/api/" prefix
    std::string path = req.uri.substr(api_prefix.size());

    // Find first '/' after entity name
    size_t slash_pos = path.find('/');
    std::string entity = (slash_pos == std::string::npos) ? path : path.substr(0, slash_pos);
    std::string id = (slash_pos == std::string::npos) ? "" : path.substr(slash_pos + 1);

    if (entity.empty()) {
        LOG_DEBUG << "Missing entity type in URI: " << req.uri;
        resp->status_code = 400;
        resp->reason_phrase = "Bad Request";
        resp->body = "Missing entity type.";
        return resp;
    }

    if (req.method == "POST") {
        return post(req, entity);
    } else if (req.method == "GET") {
        return get(entity, id);
    } else if (req.method == "PUT") {
        return put(req, entity, id);
    } else if (req.method == "DELETE") {
        return delete_req(entity, id);
    } else {
        resp->status_code = 405;
        resp->reason_phrase = "Method Not Allowed";
        resp->body = "Unsupported operation for given URI.";
        return resp;
    }
}

std::unique_ptr<response> CrudHandler::post(const request& req, const std::string& name) {
    auto resp = std::make_unique<response>();
    resp->http_version = "HTTP/1.1";
    resp->headers["Content-Type"] = "application/json";
    LOG_INFO << "Handling POST request for entity: " << name;

    try {
        std::stringstream ss;
        ss << req.body;

        // Check if the body is empty or contains only whitespace
        std::string body = ss.str();
        if (body.empty() || std::all_of(body.begin(), body.end(), ::isspace)) {
            resp->status_code = 400;
            resp->reason_phrase = "Bad Request";
            resp->body = "Empty or whitespace-only body\n";
            LOG_ERROR << "Received an empty or whitespace-only body for POST request to " << name;
            return resp;
        }

        // Validate JSON
        pt::ptree pt;
        pt::read_json(ss, pt);

        // Create the entity and get the id
        auto [success, id] = file_system_->create_entity(name);

        if (success) {
            // Write the entire JSON body to the entity
            if (file_system_->write_entity(name, id, req.body)) {
                resp->status_code = 201;
                resp->reason_phrase = "Created";
                resp->body = "{\"id\": \"" + id + "\"}\n";
                LOG_INFO << "Entity created successfully with ID: " << id;
            } else {
                resp->status_code = 500;
                resp->reason_phrase = "Internal Server Error";
                resp->body = "Failed to write entity data\n";
                LOG_ERROR << "Failed to write entity data for " << name << " with ID: " << id;
            }
        } else {
            resp->status_code = 500;
            resp->reason_phrase = "Internal Server Error";
            resp->body = "Failed to create entity\n";
            LOG_ERROR << "Failed to create entity for " << name;
        }
    } catch (const pt::json_parser_error& e) {
        resp->status_code = 400;
        resp->reason_phrase = "Bad Format";
        resp->body = "Invalid JSON format\n";
        LOG_ERROR << "Invalid JSON format: " << e.what();
    } catch (const std::exception& e) {
        resp->status_code = 500;
        resp->reason_phrase = "Internal Server Error";
        resp->body = "An unexpected error occurred\n";
        LOG_ERROR << "An unexpected error occurred: " << e.what();
    }

    return resp;
}

std::unique_ptr<response> CrudHandler::get(const std::string& name, const std::string& id) {
    auto resp = std::make_unique<response>();
    resp->http_version = "HTTP/1.1";
    resp->headers["Content-Type"] = "application/json";
    LOG_INFO << "Handling GET request for entity: " << name << " with id: " << id;

    // if the ID is not specified, list all ID's associated with the entity
    if (id.empty()) {
        auto [success, ids] = file_system_->list_entities(name);
        if (!success) {
            resp->status_code = 404;
            resp->reason_phrase = "Not Found";
            resp->body = "Entity type does not exist\n";
            return resp;
        }

        // Build a json array of ID's tied with the entity to be returned
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < ids.size(); ++i) {
            oss << "\"" << ids[i] << "\"";
            if (i != ids.size() - 1) oss << ", ";
        }
        oss << "]\n";

        resp->status_code = 200;
        resp->reason_phrase = "OK";
        resp->body = oss.str();
        LOG_INFO << "Successfully retrieved ids for entity: " << name;
        return resp;
    }

    auto [success, data] = file_system_->read_entity(name, id);
    if (success) {
        resp->status_code = 200;
        resp->reason_phrase = "OK";
        resp->body = data;
        LOG_INFO << "Successfully retrieved entity: " << name << " with id: " << id;
    } else {
        resp->status_code = 404;
        resp->reason_phrase = "Not found";
        resp->body = "Entity not found\n";
        LOG_INFO << "Couldn't retrieve entity: " << name << " with id: " << id;
    }

    return resp;
}

// Implementation of PUT method for updating entities
std::unique_ptr<response> CrudHandler::put(const request& req, const std::string& name, const std::string& id) {
    auto resp = std::make_unique<response>();
    resp->http_version = "HTTP/1.1";
    resp->headers["Content-Type"] = "application/json";
    LOG_INFO << "Handling PUT request for entity: " << name << " with id: " << id;

    // Check if id is provided
    if (id.empty()) {
        resp->status_code = 400;
        resp->reason_phrase = "Bad Request";
        resp->body = "ID must be specified for PUT operation\n";
        LOG_ERROR << "Missing ID in PUT request for entity type: " << name;
        return resp;
    }

    try {
        std::stringstream ss;
        ss << req.body;

        // Check if the body is empty or contains only whitespace
        std::string body = ss.str();
        if (body.empty() || std::all_of(body.begin(), body.end(), ::isspace)) {
            resp->status_code = 400;
            resp->reason_phrase = "Bad Request";
            resp->body = "Empty or whitespace-only body\n";
            LOG_ERROR << "Received an empty or whitespace-only body for PUT request to " << name << "/" << id;
            return resp;
        }

        // Validate JSON
        pt::ptree pt;
        pt::read_json(ss, pt);
        
        // Check if the entity directory exists, create if not
        fs::path directory = fs::path(file_system_->get_data_path()) / name;
        if (!fs::exists(directory)) {
            fs::create_directories(directory);
        }
        
        // Check if entity already exists
        bool entity_existed = file_system_->exists(name, id);

        // Write using interface
        bool write_success = file_system_->write_entity(name, id, body);
        if (!write_success) {
            resp->status_code = 500;
            resp->reason_phrase = "Internal Server Error";
            resp->body = "Failed to write entity data\n";
            LOG_ERROR << "Failed to write entity data for " << name << " with ID: " << id;
            return resp;
        }
        
        resp->status_code = entity_existed ? 200 : 201;
        resp->reason_phrase = entity_existed ? "OK" : "Created";
        resp->body = "{\"id\": \"" + id + "\"}\n";
        LOG_INFO << "Entity " << (entity_existed ? "updated" : "created") << " successfully with ID: " << id;
    } catch (const pt::json_parser_error& e) {
        resp->status_code = 400;
        resp->reason_phrase = "Bad Format";
        resp->body = "Invalid JSON format\n";
        LOG_ERROR << "Invalid JSON format: " << e.what();
    } catch (const std::exception& e) {
        resp->status_code = 500;
        resp->reason_phrase = "Internal Server Error";
        resp->body = "An unexpected error occurred\n";
        LOG_ERROR << "An unexpected error occurred: " << e.what();
    }

    return resp;
}

// Implementation of DELETE method for removing entities
std::unique_ptr<response> CrudHandler::delete_req(const std::string& name, const std::string& id) {
    auto resp = std::make_unique<response>();
    resp->http_version = "HTTP/1.1";
    resp->headers["Content-Type"] = "application/json";
    LOG_INFO << "Handling DELETE request for entity: " << name << " with id: " << id;

    // Check if id is provided
    if (id.empty()) {
        resp->status_code = 400;
        resp->reason_phrase = "Bad Request";
        resp->body = "ID must be specified for DELETE operation\n";
        LOG_ERROR << "Missing ID in DELETE request for entity type: " << name;
        return resp;
    }

    if (file_system_->delete_entity(name, id)) {
        resp->status_code = 200;
        resp->reason_phrase = "OK";
        resp->body = "{\"id\": \"" + id + "\", \"deleted\": true}\n";
        LOG_INFO << "Successfully deleted entity: " << name << " with id: " << id;
    } else {
        resp->status_code = 404;
        resp->reason_phrase = "Not Found";
        resp->body = "Entity not found\n";
        LOG_INFO << "Couldn't delete entity: " << name << " with id: " << id << " (not found)";
    }

    return resp;
}