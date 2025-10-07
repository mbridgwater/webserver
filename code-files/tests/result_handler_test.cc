#include "gtest/gtest.h"
#include "result_handler.h"
#include "request.h"
#include "response.h"

#include <fstream>
#include <filesystem>
#include <string>

void write_ucla_dining_quiz_json(const std::filesystem::path& path) {
    std::ofstream file(path);
    file << R"({
        "questions": [
            {
                "prompt": "Pick one",
                "options": [
                    { "text": "BPlate", "value": "bplate" },
                    { "text": "Epicuria", "value": "epicuria" }
                ]
            }
        ],
        "results": {
            "bplate": {
                "title": "You're B-Plate!",
                "description": "You’re on top of everything. You protect your peace...",
                "image": "/images/bplate.jpg"
            },
            "rende-west": {
                "title": "You're Rende West!",
                "description": "You are super chill and go with the flow...",
                "image": "/images/rende-west.jpg"
            },
            "epicuria": {
                "title": "You're Epicuria!",
                "description": "You like to romanticize your life...",
                "image": "/images/epicuria.jpg"
            },
            "de-neve": {
                "title": "You're De Neve!",
                "description": "Picky AF. Chaotic neutral...",
                "image": "/images/de-neve.jpg"
            }
        }
    })";
}

request make_post_request(const std::string& body) {
    request req;
    req.method = "POST";
    req.uri = "/quiz/submit";
    req.http_version = "HTTP/1.1";
    req.body = body;
    return req;
}

class ResultHandlerTest : public ::testing::Test {
protected:
    std::filesystem::path temp_dir;

    void SetUp() override {
        temp_dir = std::filesystem::temp_directory_path() / "result_handler_test";
        std::filesystem::create_directories(temp_dir);
    }

    void TearDown() override {
        std::filesystem::remove_all(temp_dir);
    }
};

// Test that selecting mostly "epicuria" answers returns "You're Epicuria!"
TEST_F(ResultHandlerTest, EpicuriaDominantAnswersReturnsEpicuriaResult) {
    write_ucla_dining_quiz_json(temp_dir / "dining.json");

    ResultHandler handler(temp_dir.string());
    auto res = handler.handle_request(make_post_request("quiz_id=dining&q1=epicuria&q2=epicuria&q3=epicuria"));

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->headers["Content-Type"], "text/html");
    // NOTE: &#39; is ' 
    EXPECT_NE(res->body.find("You&#39;re Epicuria!"), std::string::npos);
    EXPECT_NE(res->body.find("/images/epicuria.jpg"), std::string::npos);
}

// Test that selecting mostly "bplate" answers returns "You're B-Plate!"
TEST_F(ResultHandlerTest, BPlateDominantAnswersReturnsBPlateResult) {
    write_ucla_dining_quiz_json(temp_dir / "dining.json");

    ResultHandler handler(temp_dir.string());
    auto res = handler.handle_request(make_post_request("quiz_id=dining&q1=bplate&q2=bplate&q3=bplate"));

    EXPECT_EQ(res->status_code, 200);
    // NOTE: &#39; is ' 
    EXPECT_NE(res->body.find("You&#39;re B-Plate!"), std::string::npos);
    EXPECT_NE(res->body.find("/images/bplate.jpg"), std::string::npos);
}

// Test that tied answers still return a valid result (tie-breaker)
TEST_F(ResultHandlerTest, TieBreakerReturnsAnyTopResult) {
    write_ucla_dining_quiz_json(temp_dir / "dining.json");

    ResultHandler handler(temp_dir.string());
    auto res = handler.handle_request(make_post_request("quiz_id=dining&q1=bplate&q2=epicuria&q3=bplate&q4=epicuria"));

    EXPECT_EQ(res->status_code, 200);
    EXPECT_TRUE(
        // NOTE: &#39; is ' 
        res->body.find("You&#39;re B-Plate!") != std::string::npos ||
        res->body.find("You&#39;re Epicuria!") != std::string::npos
    );
}

// Test that submitting no answers returns a friendly message and retake link
TEST_F(ResultHandlerTest, NoAnswersReturnsFriendlyMessage) {
    write_ucla_dining_quiz_json(temp_dir / "dining.json");

    ResultHandler handler(temp_dir.string());
    auto res = handler.handle_request(make_post_request("quiz_id=dining"));

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->headers["Content-Type"], "text/html");

    EXPECT_NE(res->body.find("Oops! You didn't answer any questions."), std::string::npos);
    EXPECT_NE(res->body.find("Retake the Quiz"), std::string::npos);
    EXPECT_NE(res->body.find("/quiz/dining"), std::string::npos);
}

