#include "result_handler.h"
#include "response.h"
#include "res_req_helpers.h"
#include "logger.h"

#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <random>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
std::string escape_html(const std::string& input);
json load_quiz_json(const std::filesystem::path& path);
std::string render_result_html(const json& result_data, const std::string& quiz_id, const std::string& result_key);
std::unique_ptr<response> make_error_response(int status, const std::string& message);

ResultHandler::ResultHandler(const std::string& quiz_root) {
    try {
        // Convert to absolute canonical path if it exists
        quiz_root_ = std::filesystem::canonical(quiz_root).string();
        LOG_INFO << "ResultHandler initialized with root: " << quiz_root_;
    } catch (const std::filesystem::filesystem_error& e) {
        quiz_root_ = quiz_root;
        LOG_WARNING << "Could not canonicalize quiz_root.";
    }
}

// Factory method for creating a ResultHandler instance from config args
std::unique_ptr<RequestHandler> ResultHandler::create(const std::unordered_map<std::string, std::string>& args) {
    auto it = args.find("quiz_root");
    if (it != args.end()) {
        return std::make_unique<ResultHandler>(it->second);
    }
    return nullptr;
}

std::unique_ptr<response> ResultHandler::handle_request(const request& req) {
    LOG_INFO << "Entering handle_request in ResultHandler";

    if (req.method == "POST") {
        return handle_post_result(req);
    } else if (req.method == "GET") {
        return handle_get_shared_result(req);
    } else {
        return make_error_response(405, "Unsupported method.");

    }
}

// Handles POST requests to /quiz/submit
std::unique_ptr<response> ResultHandler::handle_post_result(const request& req) {
    LOG_INFO << "Entering handle_request in ResultHandler";

    // LOG_INFO << "Received POST request with body: " << req.body;    // REMOVE
    // for (const auto& [key, value] : req.headers) {
    //     LOG_INFO << "Header: " << key << " = " << value;
    // }
    
    auto res = std::make_unique<response>();
    res->http_version = "HTTP/1.1";

    // Parse the submitted form body
    auto params = parse_quiz_submission(req.body);

    // Check if quiz_id is missing
    if (params.find("quiz_id") == params.end()) {
        return make_error_response(400, "Uh oh, something went wrong! Please try submitting again.");
    }

    std::string quiz_id = params["quiz_id"];

    std::filesystem::path quiz_path = std::filesystem::path(quiz_root_) / (quiz_id + ".json");

    LOG_INFO << "quiz_path is: " << quiz_path;

    // Load quiz file
    json quiz_json;
    try {
        quiz_json = load_quiz_json(quiz_path);
    } catch (...) {
        return make_error_response(500, "Could not read quiz file.");
    }

    // Determine the result from the answers
    std::string result_key = calculate_result(params);

    if (result_key == "no-result") {
        std::ostringstream body;

        std::string quiz_link = "/quiz";
        if (params.find("quiz_id") != params.end()) {
            quiz_link += "/" + params.at("quiz_id");
        }
        body << "<html><head><link rel=\"stylesheet\" href=\"/static/quizzes/styles.css\"></head><body><div class='container'>";
        body << "<h1>Oops! You didn't answer any questions.</h1>";
        body << "<p>Want to give it another shot?</p>";
        body << "<a href=\"" << quiz_link << "\">Retake the Quiz</a><br>";
        body << "<a href=\"/quiz\">Take another quiz</a>";
        body << "</div></body></html>";

        res->status_code = 200;
        res->reason_phrase = "OK";
        res->body = body.str();
        res->headers["Content-Type"] = "text/html";
        res->headers["Content-Length"] = std::to_string(res->body.size());
        return res;
    }

    const auto& result_data = quiz_json["results"][result_key];

    // Render result HTML
    res->body = render_result_html(result_data, quiz_id, result_key);
    res->status_code = 200;
    res->reason_phrase = "OK";
    res->headers["Content-Type"] = "text/html";
    res->headers["Content-Length"] = std::to_string(res->body.size());

    return res;
}


std::unique_ptr<response> ResultHandler::handle_get_shared_result(const request& req) {
    auto res = std::make_unique<response>();
    res->http_version = "HTTP/1.1";

    // Parse query parameters from the URI (e.g., /quiz/submit?quiz_id=dining-hall&result=computer-science)
    auto query_pos = req.uri.find('?');
    if (query_pos == std::string::npos || query_pos + 1 >= req.uri.size()) {
        return make_error_response(400, "Missing query parameters.");
    }

    auto query = req.uri.substr(query_pos + 1);
    std::unordered_map<std::string, std::string> params = parse_quiz_submission(query);  // reuse form parser

    if (params.find("quiz_id") == params.end() || params.find("result") == params.end()) {
        return make_error_response(400, "Missing quiz_id or result in query.");

    }

    std::string quiz_id = params["quiz_id"];
    std::string result_key = params["result"];
    std::filesystem::path quiz_path = std::filesystem::path(quiz_root_) / (quiz_id + ".json");

    json quiz_json;
    try {
        quiz_json = load_quiz_json(quiz_path);
    } catch (...) {
        return make_error_response(500, "Could not read quiz file.");

    }

    if (!quiz_json["results"].contains(result_key)) {
        return make_error_response(404, "Result not found in quiz.");

    }

    const auto& result_data = quiz_json["results"][result_key];

    res->body = render_result_html(result_data, quiz_id, result_key);
    res->status_code = 200;
    res->reason_phrase = "OK";
    res->headers["Content-Type"] = "text/html";
    res->headers["Content-Length"] = std::to_string(res->body.size());
    return res;
}

// Helper functions

// Helper to load and parse a quiz JSON file from disk
json load_quiz_json(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f.is_open()) throw std::runtime_error("Quiz file not found.");

    json quiz_json;
    f >> quiz_json;
    return quiz_json;
}

// Helper to generates HTML for displaying quiz results, including shareable link
std::string render_result_html(const json& result_data, const std::string& quiz_id, const std::string& result_key) {
    std::ostringstream body;
    body << "<html><head><link rel=\"stylesheet\" href=\"/static/quizzes/styles.css\"></head><body><div class='container'>";
    body << "<h1>" << escape_html(result_data["title"]) << "</h1>";
    body << "<p>" << escape_html(result_data["description"]) << "</p>";
    if (result_data.contains("image")) {
                body << "<div style='text-align: center; margin-bottom: 15px;'>";
                body << "<img src=\"/static/quizzes/" << escape_html(result_data["image"]) 
                    << "\" style=\"max-width: 100%; width: 400px; height: auto; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1);\" />";
                body << "</div>";
    }
    body << "<br><a href=\"/quiz\">Take another quiz</a>";
    std::string share_link = "/quiz/submit?quiz_id=" + quiz_id + "&result=" + result_key;

    body << "<div class='share-section'>";
    body << "<p>Want to share your result?</p>";
    body << "<input type=\"text\" value=\"" << share_link << "\" id=\"shareLink\" readonly>";
    body << "<br>";
    body << "<button onclick=\"navigator.clipboard.writeText(document.getElementById('shareLink').value)\">Copy Link</button>";
    body << "</div></body></html>";
    return body.str();
}

// Helper that constructs a standardized plain-text error response with the given HTTP status and message.
std::unique_ptr<response> make_error_response(int status, const std::string& message) {
    auto res = std::make_unique<response>();
    res->http_version = "HTTP/1.1";
    res->status_code = status;

    if (status == 400)
        res->reason_phrase = "Bad Request";
    else if (status == 404)
        res->reason_phrase = "Not Found";
    else if (status == 405)
        res->reason_phrase = "Method Not Allowed";
    else
        res->reason_phrase = "Internal Server Error";

    res->body = message;
    res->headers["Content-Type"] = "text/plain";
    res->headers["Content-Length"] = std::to_string(res->body.size());
    return res;
}

// Helper to parse x-www-form-urlencoded (simple, not robust)
std::unordered_map<std::string, std::string> parse_quiz_submission(const std::string& body) {
    std::unordered_map<std::string, std::string> params;
    std::istringstream ss(body); // split on '&'
    std::string token;

    while (std::getline(ss, token, '&')) {
        auto pos = token.find('=');
        if (pos != std::string::npos) {
            std::string key = token.substr(0, pos);      // e.g. "quiz_id"
            std::string val = token.substr(pos + 1);     // e.g. "dining-hall"

            // If value starts and ends with '%22' (encoded quotes), strip them (e.g. "%22epicuria%22" → "epicuria")
            if (val.size() >= 4 && val.substr(0, 3) == "%22" && val.substr(val.size() - 3) == "%22")
                val = val.substr(3, val.size() - 6);
            params[key] = val;
        }
    }
    return params;
}

std::string calculate_result(const std::unordered_map<std::string, std::string>& params) {
    std::map<std::string, int> counts;

    for (const auto& [key, val] : params) {
        if (key.rfind("q", 0) == 0 && key != "quiz_id") { // starts with 'q' and isn't the quiz id
            counts[val]++;
        }
    }

    // Find the most frequent value
    int max_count = 0;
    std::vector<std::string> top;   // top is a vector with the values with the top counts
    for (const auto& [val, count] : counts) {
        if (count > max_count) {
            max_count = count;
            top = {val};
        } else if (count == max_count) {
            top.push_back(val);
        }
    }

    return top.empty() ? "no-result" : top[0];  // Just pick first (arbitrary tie-breaker)
}
