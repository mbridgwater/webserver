#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>
#include <map>

struct response {
    std::string http_version;     // e.g., "HTTP/1.1"
    int status_code;              // e.g., 200, 404
    std::string reason_phrase;    // e.g., "OK", "Not Found"
    std::map<std::string, std::string> headers; // header key-value pairs
    std::string body;             // the response body (html file, image bytes, echo text, etc.)
};

#endif // RESPONSE_H