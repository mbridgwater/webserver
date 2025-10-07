#ifndef QUIZ_HANDLER_H
#define QUIZ_HANDLER_H

#include "request_handler.h"
#include <string>
#include <unordered_map>

class QuizHandler : public RequestHandler {
public:

    // Constructs a QuizHandler with a directory containing quiz JSON files.
    explicit QuizHandler(const std::string& quiz_root);

    // Factory method
    static std::unique_ptr<RequestHandler> create(const std::unordered_map<std::string, std::string>& args);

    // Handles GET requests to /quiz or /quiz/<id>.
    virtual std::unique_ptr<response> handle_request(const request& req) override;

private:
    std::string quiz_root_; 
};

#endif // QUIZ_HANDLER_H
