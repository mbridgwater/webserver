#ifndef REQUEST_H
#define REQUEST_H

#include <string>
#include <map>

struct request {
    std::string method;           // e.g., "GET"
    std::string uri;              // e.g., "/static/index.html"
    std::string http_version;     // e.g., "HTTP/1.1"
    std::map<std::string, std::string> headers; // header key-value pairs
    std::string body;             // only used for POST, usually empty for GET
};

#endif // REQUEST_H