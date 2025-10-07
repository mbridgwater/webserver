#include "res_req_helpers.h"
#include <sstream>  

// HTTP request parser 
request parse_request(const std::string& raw_request)
{
    request req;

    // Find the separator between headers and body
    size_t header_end = raw_request.find("\r\n\r\n");

    if (header_end == std::string::npos) {
        // Malformed HTTP request (no header-body separator)
        return req;
    }

    // Split header and body parts
    std::string header_part = raw_request.substr(0, header_end);
    req.body = raw_request.substr(header_end + 4);  // skip over \r\n\r\n

    std::istringstream header_stream(header_part);
    std::string line;

    // First line: METHOD URI HTTP_VERSION
    if (std::getline(header_stream, line)) {
        if (!line.empty() && line.back() == '\r') {  // <-- strip \r
            line.pop_back();
        }
        std::istringstream line_stream(line);
        if (!(line_stream >> req.method >> req.uri >> req.http_version)) {
            return {};  // Malformed: can't parse 3 parts
        }
    }

    // Validation
    if (req.method.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos) {
        return {};
    }
    if (req.uri.empty() || req.uri[0] != '/') {
        return {};
    }
    if (req.http_version != "HTTP/1.0" && req.http_version != "HTTP/1.1") {
        return {};
    }
    if (req.http_version.find_first_not_of("HTTP/./0123456789") != std::string::npos) {
        return {};
    }

    // Headers
    while (std::getline(header_stream, line)) {
        if (!line.empty() && line.back() == '\r') {  // <-- strip \r
            line.pop_back();
        }
        if (line.empty()) {
            break;  // safety, although we already split on \r\n\r\n
        }

        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            // Remove any leading spaces
            if (!value.empty() && value[0] == ' ') {
                value = value.substr(1);
            }
            req.headers[key] = value;
        }
    }

    return req;
}

// HTTP response serializer 
std::string serialize_response(const response& res)
{
    std::ostringstream response_stream;
    response_stream << res.http_version << " " << res.status_code << " " << res.reason_phrase << "\r\n";
    for (const auto& header : res.headers) {
        response_stream << header.first << ": " << header.second << "\r\n";
    }
    response_stream << "\r\n";
    response_stream << res.body;
    return response_stream.str();
}