#ifndef STATIC_FILE_HANDLER_H
#define STATIC_FILE_HANDLER_H

#include "request_handler.h"
#include <string>
#include <unordered_map>

class StaticFileHandler : public RequestHandler {
public:

    // Constructs a StaticFileHandler to serve files from a directory.
    // @param mount_point: URL prefix to match (e.g., "/static/").
    // @param doc_root: root directory on disk (local filesystem directory) containing static files (e.g. "/usr/src/project/static").
    StaticFileHandler(const std::string& mount_point,
                      const std::string& doc_root);

    // Factory method 
    // @param args: dictionary of argument names to values 
    static std::unique_ptr<RequestHandler> create(const std::unordered_map<std::string, std::string>& args); 

    // Serves a static file under mount_point_ based on the request URI.
    // @return: Unique pointer to HTTP response with 200 OK and file content, or 404 Not Found on error.
    virtual std::unique_ptr<response> handle_request(const request& req) override;

private:
    std::string mount_point_; // URI prefix this handler responds to.
    std::string doc_root_; // Filesystem directory containing static content.

    static const std::unordered_map<std::string, std::string> kMimeTypes; // Maps file extensions to MIME types (e.g., ".html" → "text/html").
};

#endif // STATIC_FILE_HANDLER_H