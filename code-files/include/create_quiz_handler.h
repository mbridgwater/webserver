#ifndef CREATE_QUIZ_HANDLER_H
#define CREATE_QUIZ_HANDLER_H

#include "request_handler.h"
#include "file_system.h"
#include "file_system_interface.h"
#include <string>
#include <unordered_map>

class CreateQuizHandler : public RequestHandler {
public:

    // Constructs a CreateQuizHandler
    CreateQuizHandler(std::shared_ptr<FileSystemInterface> file_system);

    // Factory method
    static std::unique_ptr<RequestHandler> create(const std::unordered_map<std::string, std::string>& args);

    // Handles requests 
    virtual std::unique_ptr<response> handle_request(const request& req) override;

private:
    // Holds the root directory for where create quiz handler stores files
    std::shared_ptr<FileSystemInterface> file_system_; 
};

#endif // QUIZ_HANDLER_H