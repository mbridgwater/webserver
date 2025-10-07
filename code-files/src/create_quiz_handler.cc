#include "create_quiz_handler.h"
#include "request.h"
#include "response.h"
#include "file_system_interface.h"
#include "logger.h"

#include <memory>
#include <sstream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <regex>
#include <unordered_set>
#include <codecvt>
#include <locale>

// Helpers 
std::string url_decode(const std::string& str) {
    std::string decoded;
    char hex[3] = {0};
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '+') {
            decoded += ' ';
        } else if (str[i] == '%' && i + 2 < str.size()) {
            hex[0] = str[i + 1];
            hex[1] = str[i + 2];
            decoded += static_cast<char>(std::strtol(hex, nullptr, 16));
            i += 2;
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}

std::unordered_map<std::string, std::string> parse_urlencoded(const std::string& body) {
    std::unordered_map<std::string, std::string> result;
    std::istringstream ss(body);
    std::string pair;
    while (std::getline(ss, pair, '&')) {
        auto eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = url_decode(pair.substr(0, eq));
            std::string val = url_decode(pair.substr(eq + 1));
            result[key] = val;
        }
    }
    return result;
}

bool is_valid_utf8(const std::string& str) {
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        conv.from_bytes(str);  // throws if invalid UTF-8
        return true;
    } catch (const std::range_error&) {
        return false;
    }
}

namespace fs = std::filesystem;

// Constructor
CreateQuizHandler::CreateQuizHandler(std::shared_ptr<FileSystemInterface> file_system)
    : file_system_(file_system) {}

// Factory method
std::unique_ptr<RequestHandler> CreateQuizHandler::create(const std::unordered_map<std::string, std::string>& args) {
    auto it = args.find("quiz_root");
    if (it != args.end()) {
        return std::make_unique<CreateQuizHandler>(std::make_shared<FileSystem>(it->second));
    }
    return nullptr;
}

// Handle POST request to create a new quiz
std::unique_ptr<response> CreateQuizHandler::handle_request(const request& req) {
    auto res = std::make_unique<response>();
    res->http_version = "HTTP/1.1";

    if (req.uri == "/quiz/create") {
        if (req.method == "GET") {
            std::ostringstream body;
            body << "<html><head><title>BruinFeed Quizzes</title>";
            body << "<link rel=\"stylesheet\" type=\"text/css\" href=\"/static/quizzes/styles.css\">";
            body << "<script>"
                    "let questionCount = 1;"
                    "function addQuestion() {"
                    "  const container = document.getElementById('questions');"
                    "  const qIndex = questionCount++;"
                    "  const fieldset = document.createElement('fieldset');"
                    "  fieldset.innerHTML = `"
                    "<legend>Question ${qIndex + 1}</legend>"
                    "<label>Prompt: <input type='text' name='q${qIndex}_prompt' required></label><br>"
                    "Option 1: <input type='text' name='q${qIndex}_opt0_text' placeholder='Option Text' required> "
                    "<input type='text' name='q${qIndex}_opt0_val' placeholder='Result Value' required><br>"
                    "Option 2: <input type='text' name='q${qIndex}_opt1_text' placeholder='Option Text' required> "
                    "<input type='text' name='q${qIndex}_opt1_val' placeholder='Result Value' required><br>"
                    "Option 3: <input type='text' name='q${qIndex}_opt2_text' placeholder='Option Text' required> "
                    "<input type='text' name='q${qIndex}_opt2_val' placeholder='Result Value' required><br>"
                    "Option 4: <input type='text' name='q${qIndex}_opt3_text' placeholder='Option Text' required> "
                    "<input type='text' name='q${qIndex}_opt3_val' placeholder='Result Value' required><br>"
                    "`;"
                    "  container.appendChild(fieldset);"
                    "}"
                    "</script>";
            body << "</head><body><div class='container'>";
            body << "<h1>Create Quiz</h1>";
            body << "<p>Create your own UCLA inspired quiz below:</p>";

            body << "<form action=\"/quiz/create\" method=\"POST\">";

            // Quiz ID and title
            body << "<label>Unique Quiz ID: <input type=\"text\" name=\"quiz_id\" required></label><br><br>";
            body << "<label>Quiz Title: <input type=\"text\" name=\"title\" required></label><br><br>";

            // One default question
            body << "<div id='questions'>";
            body << "<fieldset><legend>Question 1</legend>";
            body << "<label>Prompt: <input type=\"text\" name=\"q0_prompt\" required></label><br>";
            for (int j = 0; j < 4; ++j) {
                body << "Option " << (j + 1) << ": ";
                body << "<input type=\"text\" name=\"q0_opt" << j << "_text\" placeholder=\"Option Text\" required> ";
                body << "<input type=\"text\" name=\"q0_opt" << j << "_val\" placeholder=\"Result Value\" required><br>";
            }
            body << "</fieldset>";
            body << "</div><br>";

            body << "<button type=\"button\" onclick=\"addQuestion()\">Add Question</button><br><br>";

            // Results section
            body << "<h3>Define Result Categories</h3>";
            body << "<div id='results'>";
            body << "<p>The results will be matched by value in the options above.</p>";

            for (int i = 0; i < 4; ++i) {
                body << "<fieldset><legend>Result " << (i + 1) << "</legend>";
                body << "<label>Result Value Key: <input type=\"text\" name=\"result_" << i << "_key\" required></label><br>";
                body << "<label>Title: <input type=\"text\" name=\"result_" << i << "_title\" required></label><br>";
                body << "<label>Description:<br><textarea name=\"result_" << i << "_desc\" rows=\"4\" cols=\"50\" required></textarea></label><br>";
                body << "</fieldset><br>";
            }
            body << "</div>";

            body << "<input type=\"submit\" value=\"Create Quiz\">";
            body << "</form>";
            body << "<br><a href=\"/quiz\">Back to BruinFeed Quizzes homepage</a>";
            body << "</div></body></html>";


            res->status_code = 200;
            res->reason_phrase = "OK";
            res->headers["Content-Type"] = "text/html";
            res->body = body.str();
        } else if (req.method == "POST") {
            // Parse form-encoded body into key-value pairs
            LOG_INFO << "Raw POST body: " << req.body << std::endl;
            std::unordered_map<std::string, std::string> params = parse_urlencoded(req.body);

            for (const auto& [k, v] : params) {
                LOG_INFO << "Parsed param: " << k << " = " << v << std::endl;
            }
            LOG_INFO << "quiz_id = " << params["quiz_id"] << ", title = " << params["title"] << std::endl;

            // Check for invalid UTF-8 in all submitted values
            for (const auto& [key, value] : params) {
                if (!is_valid_utf8(value)) {
                    res->status_code = 400;
                    res->reason_phrase = "Bad Request";
                    res->body = "Submission contains invalid characters in field: " + key;
                    res->headers["Content-Type"] = "text/plain";
                    res->headers["Content-Length"] = std::to_string(res->body.size());
                    return res;
                }
            }

            // Check for consistent result values
            std::unordered_set<std::string> used_result_keys;

            std::string quiz_id = params["quiz_id"];
            std::string quiz_title = params["title"];

            if (quiz_id.empty() || quiz_title.empty()) {
                res->status_code = 400;
                res->reason_phrase = "Bad Request";
                res->body = "Uh-oh, something went wrong! Please try submitting again.";
                res->headers["Content-Type"] = "text/plain";
                res->headers["Content-Length"] = std::to_string(res->body.size());
                return res;
            }

            // Check filename is valid
            std::regex valid_id("^[a-zA-Z0-9._-]+$"); // allow letters, numbers, dot, underscore, dash

            if (!std::regex_match(quiz_id, valid_id)) {
                res->status_code = 400;
                res->reason_phrase = "Bad Request";
                res->body = "Quiz ID must only contain letters, numbers, dots, dashes, or underscores.";
                res->headers["Content-Type"] = "text/plain";
                res->headers["Content-Length"] = std::to_string(res->body.size());
                return res;
            }

            // Construct quiz JSON
            nlohmann::json quiz_json;
            quiz_json["title"] = quiz_title;
            quiz_json["questions"] = nlohmann::json::array();

            // Collect questions
            int q_index = 0;
            while (true) {
                std::string prompt_key = "q" + std::to_string(q_index) + "_prompt";
                if (params.find(prompt_key) == params.end()) break;

                std::string prompt = params[prompt_key];
                if (prompt.empty()) {
                    ++q_index;
                    continue; // skip if prompt is blank
                }

                nlohmann::json question;
                question["prompt"] = prompt;
                question["options"] = nlohmann::json::array();

                bool has_valid_option = false;
                for (int j = 0; j < 4; ++j) {
                    std::string text_key = "q" + std::to_string(q_index) + "_opt" + std::to_string(j) + "_text";
                    std::string val_key =  "q" + std::to_string(q_index) + "_opt" + std::to_string(j) + "_val";

                    if (params.find(text_key) != params.end() && params.find(val_key) != params.end()) {
                        std::string text = params[text_key];
                        std::string val = params[val_key];
                        if (!text.empty() && !val.empty()) {
                            has_valid_option = true;
                            used_result_keys.insert(val);  // track result value
                            question["options"].push_back({
                                {"text", text},
                                {"value", val}
                            });
                        }
                    }
                }

                if (has_valid_option) {
                    quiz_json["questions"].push_back(question);
                }

                ++q_index;
            }

            // Collect results
            quiz_json["results"] = nlohmann::json::object();
            int r_index = 0;
            while (true) {
                std::string key_field = "result_" + std::to_string(r_index) + "_key";
                if (params.find(key_field) == params.end()) break;

                std::string key = params[key_field];
                std::string title = params["result_" + std::to_string(r_index) + "_title"];
                std::string desc = params["result_" + std::to_string(r_index) + "_desc"];

                if (!key.empty() && !title.empty() && !desc.empty()) {
                    quiz_json["results"][key] = {
                        {"title", title},
                        {"description", desc}
                    };
                }

                ++r_index;
            }
            // Check if every used result value is defined
            for (const auto& val : used_result_keys) {
                if (quiz_json["results"].find(val) == quiz_json["results"].end()) {
                    res->status_code = 400;
                    res->reason_phrase = "Bad Request";
                    res->body = "Each option result value must have a corresponding result definition. Missing: " + val;
                    res->headers["Content-Type"] = "text/plain";
                    res->headers["Content-Length"] = std::to_string(res->body.size());
                    return res;
                }
            }
            
            // Ensure only 4 results are given 
            if (quiz_json["results"].size() != 4) {
                res->status_code = 400;
                res->reason_phrase = "Bad Request";
                res->body = "Exactly 4 result categories must be defined.";
                res->headers["Content-Type"] = "text/plain";
                res->headers["Content-Length"] = std::to_string(res->body.size());
                return res;
            }

            // Serialize to JSON and store via file_system_
            std::ostringstream json_out;
            json_out << quiz_json.dump(4);  // pretty-print

            fs::path quiz_dir = fs::path(file_system_->get_data_path()); 
            fs::create_directories(quiz_dir);  // ensure the path exists

            fs::path file_path = quiz_dir / (quiz_id + ".json");

            // create the file so that write_entity doesn't fail
            if (!fs::exists(file_path)) {
                std::ofstream(file_path).close();
            }

            bool success = file_system_->write_entity("", quiz_id + ".json", json_out.str());

            if (!success) {
                res->status_code = 500;
                res->reason_phrase = "Internal Server Error";
                res->headers["Content-Type"] = "text/html";
                res->body = "<html><body><h1>Failed to save quiz</h1></body></html>";
            } else {
                res->status_code = 200;
                res->reason_phrase = "OK";
                res->headers["Content-Type"] = "text/html";
                std::ostringstream html;
                html << "<html><head><title>Quiz Created</title>";
                html << "<link rel=\"stylesheet\" type=\"text/css\" href=\"/static/quizzes/styles.css\">";
                html << "</head><body><div class='container'>";
                html << "<h1>Quiz Created Successfully</h1>";
                html << "<p>Your quiz has been saved as <strong>" << quiz_id << ".json</strong>.</p>";
                html << "<a href=\"/quiz\">Return to BruinFeed Quizzes Homepage</a>";
                html << "</div></body></html>";
                res->body = html.str();
            }
        } else {
            res->status_code = 405;
            res->reason_phrase = "Method Not Allowed";
            res->headers["Content-Type"] = "text/plain";
            res->body = "Unsupported method.";
        }
    } else {
        res->status_code = 404;
        res->reason_phrase = "Not Found";
        res->headers["Content-Type"] = "text/plain";
        res->body = "Page not found.";
    }

    res->headers["Content-Length"] = std::to_string(res->body.size());
    return res;
}
