#include "quiz_handler.h"
#include "response.h"
#include "res_req_helpers.h"
#include "logger.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Helper function for HTML display
std::string escape_html(const std::string& input) {
    std::ostringstream out;
    for (char c : input) {
        switch (c) {
            case '&': out << "&amp;"; break;
            case '<': out << "&lt;"; break;
            case '>': out << "&gt;"; break;
            case '"': out << "&quot;"; break;
            case '\'': out << "&#39;"; break;
            default: out << c;
        }
    }
    return out.str();
}

// Constructor: stores the path to the directory containing quiz JSON files
QuizHandler::QuizHandler(const std::string& quiz_root) {
    try {
        // Convert to absolute canonical path if it exists
        quiz_root_ = std::filesystem::canonical(quiz_root).string();
        LOG_INFO << "QuizHandler initialized with root: " << quiz_root_;
    } catch (const std::filesystem::filesystem_error& e) {
        quiz_root_ = quiz_root;
        LOG_WARNING << "Could not canonicalize quiz_root.";
    }
}

// Factory method for creating a QuizHandler instance from config args
std::unique_ptr<RequestHandler> QuizHandler::create(const std::unordered_map<std::string, std::string>& args) {
    auto it = args.find("quiz_root");
    if (it != args.end()) {
        return std::make_unique<QuizHandler>(it->second);
    }
    return nullptr;
}

// Handles GET requests to /quiz or /quiz/<id>
std::unique_ptr<response> QuizHandler::handle_request(const request& req) {
    auto res = std::make_unique<response>();
    res->http_version = "HTTP/1.1";

    if (req.uri == "/quiz") {
        std::ostringstream body;
        body << "<html><head><title>BruinFeed Quizzes</title>";
        body << "<link rel=\"stylesheet\" type=\"text/css\" href=\"/static/quizzes/styles.css\">";
        body << "</head><body><div class='container'>";
        body << "<h1>BruinFeed Quizzes</h1>";
        // Create quiz button 
        body << "<div style='margin-bottom: 20px; text-align: left;'>";
        body << "<a href=\"/quiz/create\" class=\"create-quiz-button\">Create Quiz</a>";
        body << "</div>";
        body << "<p>Select a quiz below to get started:</p>";
        body << "<ul>";

        try {
            for (const auto& entry : std::filesystem::directory_iterator(quiz_root_)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    std::string filename = entry.path().stem().string();  // quiz ID
                    std::filesystem::path image_path = std::filesystem::path(quiz_root_) / (filename + ".jpg");
                    body << "<div style='margin-bottom: 30px; text-align: center;'>";
                    if (std::filesystem::exists(image_path)) {
                        body << "<img src=\"/static/quizzes/" << filename << ".jpg\" "
                        << "style=\"max-width: 100%; width: 500px; height: auto; border-radius: 12px; display: block; margin: 0 auto;\" />";
                    }

                    body << "<a href=\"/quiz/" << filename << "\" class='quiz-title'>" << filename << "</a>";
                    body << "</div>";
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            LOG_ERROR << "Failed to read quiz directory: " << e.what();
            res->status_code = 500;
            res->reason_phrase = "Internal Server Error";
            res->headers["Content-Type"] = "text/plain";
            res->body = "Failed to list quizzes.";
            res->headers["Content-Length"] = std::to_string(res->body.size());
            return res;
        }

        body << "</ul></div></body></html>";

        res->status_code = 200;
        res->reason_phrase = "OK";
        res->headers["Content-Type"] = "text/html";
        res->body = body.str();
    }
    // If requesting a specific quiz like /quiz/dining
    else if (req.uri.find("/quiz/") == 0) {
        std::string quiz_id = req.uri.substr(std::string("/quiz/").length());
        std::filesystem::path quiz_file = std::filesystem::path(quiz_root_) / (quiz_id + ".json");
        LOG_INFO << "Looking for quiz at: " << quiz_file;

        // If quiz file doesn't exist, return 404
        if (!std::filesystem::exists(quiz_file)) {
            res->status_code = 404;
            res->reason_phrase = "Not Found";
            res->headers["Content-Type"] = "text/plain";
            res->body = "Quiz not found.";
            res->headers["Content-Length"] = std::to_string(res->body.size());
            return res;
        }

        // Try to parse the quiz JSON file
        std::ifstream file(quiz_file);
        json quiz_json;
        try {
            file >> quiz_json;
        } catch (...) {
            res->status_code = 500;
            res->reason_phrase = "Internal Server Error";
            res->headers["Content-Type"] = "text/plain";
            res->body = "Failed to parse quiz file.";
            res->headers["Content-Length"] = std::to_string(res->body.size());
            return res;
        }

        // Generate HTML form with questions and answer options
        std::ostringstream body;
        body << "<html><head><link rel=\"stylesheet\" href=\"/static/quizzes/styles.css\"></head><body><div class='container'>";
        body << "<h1>" << escape_html(quiz_json["title"]) << "</h1>";
        body << "<form action=\"/quiz/submit\" method=\"POST\">";

        int q_num = 0;
        for (const auto& question : quiz_json["questions"]) {
            if (question.contains("image")) {
                body << "<div style='text-align: center; margin-bottom: 15px;'>";
                body << "<img src=\"/static/quizzes/" << escape_html(question["image"]) 
                    << "\" style=\"max-width: 100%; width: 400px; height: auto; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1);\" />";
                body << "</div>";
            }
            body << "<p>" << escape_html(question["prompt"]) << "</p>";
            body << "<div class='quiz-options'>";
            for (const auto& option : question["options"]) {
                body << "<label class='quiz-option'>";
                body << "<input type=\"radio\" name=\"q" << q_num << "\" value='" << escape_html(option["value"]) << "' />";
                body << "<span>" << escape_html(option["text"]) << "</span>";
                body << "</label>";
            }
            body << "</div>";
            ++q_num;
        }

        // Include quiz ID as hidden input and submit button
        body << "<input type=\"hidden\" name=\"quiz_id\" value=\"" << quiz_id << "\">";
        body << "<input type=\"submit\" value=\"Submit\">";
        body << "</form></div></body></html>";

        // Return the full quiz page as HTML
        res->status_code = 200;
        res->reason_phrase = "OK";
        res->headers["Content-Type"] = "text/html";
        res->body = body.str();
    } 
    // If URI does not match /quiz or /quiz/<id>
    else {
        res->status_code = 404;
        res->reason_phrase = "Not Found";
        res->headers["Content-Type"] = "text/plain";
        res->body = "Page not found.";
    }

    res->headers["Content-Length"] = std::to_string(res->body.size());
    return res;
}
