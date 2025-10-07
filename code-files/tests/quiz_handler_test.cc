#include "gtest/gtest.h"
#include "quiz_handler.h"
#include "request.h"
#include "response.h"

#include <fstream>
#include <filesystem>
#include <string>

// Helper to create a minimal valid quiz JSON
void write_quiz_json(const std::filesystem::path& path, const std::string& title = "Sample Quiz") {
    std::ofstream file(path);
    file << R"({
        "title": ")" << title << R"(",
        "questions": [
            {
                "prompt": "What is your favorite color?",
                "options": [
                    { "text": "Blue", "value": "blue" },
                    { "text": "Yellow", "value": "yellow" }
                ]
            }
        ]
    })";
}

// Helper to build a request
request make_get_request(const std::string& uri) {
    request req;
    req.method = "GET";
    req.uri = uri;
    req.http_version = "HTTP/1.1";
    return req;
}

class QuizHandlerTest : public ::testing::Test {
protected:
    std::filesystem::path temp_dir;

    void SetUp() override {
        temp_dir = std::filesystem::temp_directory_path() / "quiz_handler_test";
        std::filesystem::create_directories(temp_dir);
    }

    void TearDown() override {
        std::filesystem::remove_all(temp_dir);
    }
};

// Test that /quiz returns a 200 OK and lists all quizzes as clickable links 
TEST_F(QuizHandlerTest, QuizListReturnsHTMLWithLinks) {
    write_quiz_json(temp_dir / "dining.json");
    write_quiz_json(temp_dir / "personality.json");

    QuizHandler handler(temp_dir.string());
    auto res = handler.handle_request(make_get_request("/quiz"));

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->headers["Content-Type"], "text/html");
    EXPECT_NE(res->body.find("BruinFeed Quizzes"), std::string::npos);
    EXPECT_NE(res->body.find("/quiz/dining"), std::string::npos);
    EXPECT_NE(res->body.find("/quiz/personality"), std::string::npos);
}

// Test that /quiz/<id> returns a 200 OK and renders a quiz form when the JSON is valid
TEST_F(QuizHandlerTest, ValidQuizReturnsHtmlForm) {
    write_quiz_json(temp_dir / "dining.json", "Dining Quiz");

    QuizHandler handler(temp_dir.string());
    auto res = handler.handle_request(make_get_request("/quiz/dining"));

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->headers["Content-Type"], "text/html");
    EXPECT_NE(res->body.find("Dining Quiz"), std::string::npos);
    EXPECT_NE(res->body.find("form action=\"/quiz/submit\""), std::string::npos);
}

// Test that requesting a nonexistent quiz returns a 404 Not Found with appropriate message
TEST_F(QuizHandlerTest, NonexistentQuizReturns404) {
    QuizHandler handler(temp_dir.string());
    auto res = handler.handle_request(make_get_request("/quiz/unknown"));

    EXPECT_EQ(res->status_code, 404);
    EXPECT_EQ(res->reason_phrase, "Not Found");
    EXPECT_EQ(res->body, "Quiz not found.");
}

// Test that invalid JSON in a quiz file results in a 500 Internal Server Error
TEST_F(QuizHandlerTest, MalformedQuizJsonReturns500) {
    std::ofstream file(temp_dir / "broken.json");
    file << "{ invalid json ";

    QuizHandler handler(temp_dir.string());
    auto res = handler.handle_request(make_get_request("/quiz/broken"));

    EXPECT_EQ(res->status_code, 500);
    EXPECT_EQ(res->body, "Failed to parse quiz file.");
}