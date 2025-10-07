#ifndef RESULT_HANDLER_H
#define RESULT_HANDLER_H

#include "request_handler.h"
#include <string>
#include <unordered_map>


class ResultHandler : public RequestHandler {

public:
    ResultHandler(const std::string& quiz_root);
    static std::unique_ptr<RequestHandler> create(const std::unordered_map<std::string, std::string>& args);
    
    // Handles both POST (submission) and GET (shared result) requests.
    std::unique_ptr<response> handle_request(const request& req) override;

private:
    // Private functions

    // Handles POST requests to /quiz/submit.
    // Parses quiz answers and renders a result page with a shareable link.
    std::unique_ptr<response> handle_post_result(const request& req);

    // Handles GET requests to /quiz/submit?quiz_id=...&result=...
    // Renders a result page using query parameters only.
    std::unique_ptr<response> handle_get_shared_result(const request& req);

    // Private members
    std::string quiz_root_;
};

// Parses a URL-encoded quiz submission body into key-value pairs and strips surrounding "%22" from values if present.
// @param body POST body string (e.g., "q0=%22bplate%22&quiz_id=dining-hall")
// @return Map of form field names to values (e.g., "q0" → "bplate")
std::unordered_map<std::string, std::string> parse_quiz_submission(const std::string& body);

// Determines the quiz result based on the most frequently selected answer value.
// Breaks ties by returning the first value encountered with the highest count.
// @param params Map of form field names to values (e.g., "q0" → "bplate")
// @return The result key (e.g., "bplate") to look up in the quiz JSON
std::string calculate_result(const std::unordered_map<std::string, std::string>& params);


#endif // RESULT_HANDLER_H