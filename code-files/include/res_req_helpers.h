#ifndef RES_REQ_HELPERS_H
#define RES_REQ_HELPERS

#include <string>
#include "request.h"
#include "response.h"

// Parses a raw HTTP request string into a request struct.
// @param raw_request: full HTTP request as a string.
// @return: populated request object.
request parse_request(const std::string& raw_request);

// Converts a response struct into a raw HTTP response string.
// @param res: response object to serialize.
// @return: HTTP response as a string.
std::string serialize_response(const response& res);

#endif // RES_REQ_HELPERS