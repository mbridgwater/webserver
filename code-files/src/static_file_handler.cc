#include "static_file_handler.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include "logger.h" 
namespace fs = std::filesystem;

// Initialize the static MIME map
const std::unordered_map<std::string, std::string> StaticFileHandler::kMimeTypes = {
    {".html", "text/html"},
    {".css",  "text/css"},
    {".js",   "application/javascript"},
    {".json", "application/json"},
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".webp", "image/webp"},
    {".txt",  "text/plain"},
    {".zip",  "application/zip"}
};

StaticFileHandler::StaticFileHandler(const std::string& mount_point,
                                     const std::string& doc_root)
    : mount_point_(mount_point), doc_root_(doc_root) {
    // Ensure mount_point_ ends with '/'
    if (!mount_point_.empty() && mount_point_.back() != '/') {
        mount_point_ += '/';
    }
    // Remove trailing slash from doc_root_
    if (!doc_root_.empty() && doc_root_.back() == '/') {
        doc_root_.pop_back();
    }
}

std::unique_ptr<RequestHandler> StaticFileHandler::create(const std::unordered_map<std::string, std::string>& args) {
        auto it_mount = args.find("mount_point");
        auto it_root = args.find("doc_root");
        if (it_mount != args.end() && it_root != args.end()) {
            return std::make_unique<StaticFileHandler>(it_mount->second, it_root->second);
        }
        return nullptr;
}

std::unique_ptr<response> StaticFileHandler::handle_request(const request& req) {
    auto resp = std::make_unique<response>();

    // Mirror HTTP version
    resp->http_version = req.http_version;

    // Must begin with our mount_point_
    if (req.uri.rfind(mount_point_, 0) != 0) {
        LOG_DEBUG << "MOUNT_POINT DOESN'T BEGIN WITH /static/";
        // Not for us: 404
        resp->status_code = 404;
        resp->reason_phrase = "Not Found";
        resp->body = "404 Not Found";
        return resp;
    }

    // Strip prefix
    std::string rel_path = req.uri.substr(mount_point_.size());
    if (rel_path.empty() || rel_path == "/") {
        // assuming that if a file name isn't given --> 404 not found
        LOG_DEBUG << "NO FILE NAME GIVEN";
        resp->status_code = 404;
        resp->reason_phrase = "Not Found";
        resp->body = "404 Not Found";
        return resp;
    }

    // Prevent directory traversal
    fs::path safe;
    for (auto& part : fs::path(rel_path)) {
        if (part == "..") {
            LOG_DEBUG << "TRYING TO ACCESS A PARENT DIRECTORY";
            resp->status_code = 404;
            resp->reason_phrase = "Not Found";
            resp->body = "404 Not Found";
            return resp;
        }
        safe /= part;
    }

    fs::path full = fs::path(doc_root_) / safe;
    if (!fs::exists(full) || !fs::is_regular_file(full)) {
        LOG_DEBUG << "FILE DOESNT EXIST" << full;
        resp->status_code = 404;
        resp->reason_phrase = "Not Found";
        resp->body = "404 Not Found";
        return resp;
    }

    // Read file
    std::ifstream in(full, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string data = ss.str();

    // Determine MIME
    std::string ext = full.extension().string();
    auto it = kMimeTypes.find(ext);
    std::string mime = (it != kMimeTypes.end() ? it->second : "application/octet-stream");

    // Build response
    resp->status_code = 200;
    resp->reason_phrase = "OK";
    resp->headers["Content-Type"] = mime;
    resp->headers["Content-Length"] = std::to_string(data.size());
    resp->body = std::move(data);
    LOG_DEBUG << "SUCCESSFUL: RESPONSE SENDING";
    return resp;
}
