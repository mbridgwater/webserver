#include <gtest/gtest.h>
#include "static_file_handler.h"
#include "request.h"
#include "response.h"

// Fixture for StaticFileHandler tests
class StaticFileHandlerTest : public ::testing::Test {
protected:
  StaticFileHandler handler;  

  StaticFileHandlerTest()
   : handler("/static/", "../tests/app"){}
};

// checks if response outputs hello_world.html
// Expected result: PASS
TEST_F(StaticFileHandlerTest, ServesHtmlFile) {
    // Arrange
    request req;
    req.method = "GET";
    req.uri = "/static/hello_world.html";
    req.http_version = "HTTP/1.1";

    // Act
    std::unique_ptr<response> res = handler.handle_request(req);

    // Assert
    EXPECT_EQ(res->http_version, "HTTP/1.1");
    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_EQ(res->headers.at("Content-Type"), "text/html");
    EXPECT_FALSE(res->body.empty());
    // Check that it starts with the expected DOCTYPE or HTML tag
    EXPECT_NE(res->body.find("<!DOCTYPE html>"), std::string::npos);
}

// checks if response outputs flower.jpeg
// Expected result: PASS
TEST_F(StaticFileHandlerTest, ServesJpegFile) {
    request req;
    req.method = "GET";
    req.uri = "/static/flower.jpeg";
    req.http_version = "HTTP/1.1";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_EQ(res->headers.at("Content-Type"), "image/jpeg");
    EXPECT_EQ(res->headers.at("Content-Length"), std::to_string(res->body.size()));
    // JPEG magic bytes: 0xFF 0xD8
    ASSERT_GE(res->body.size(), 2u);
    EXPECT_EQ(static_cast<unsigned char>(res->body[0]), 0xFF);
    EXPECT_EQ(static_cast<unsigned char>(res->body[1]), 0xD8);
}

// checks if response outputs index.txt
// Expected result: PASS
TEST_F(StaticFileHandlerTest, ServesTxtFile) {
    request req;
    req.method = "GET";
    req.uri = "/static/index.txt";
    req.http_version = "HTTP/1.1";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_EQ(res->headers.at("Content-Type"), "text/plain");
    EXPECT_FALSE(res->body.empty());
    // Optionally check specific content
    EXPECT_NE(res->body.find(""), std::string::npos);
}

// checks if response outputs images.zip
// Expected result: PASS
TEST_F(StaticFileHandlerTest, ServesZipFile) {
    request req;
    req.method = "GET";
    req.uri = "/static/images.zip";
    req.http_version = "HTTP/1.1";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_EQ(res->headers.at("Content-Type"), "application/zip");
    EXPECT_EQ(res->headers.at("Content-Length"), std::to_string(res->body.size()));
    // ZIP magic bytes: 'P' 'K' 0x03 0x04
    ASSERT_GE(res->body.size(), 4u);
    EXPECT_EQ(static_cast<unsigned char>(res->body[0]), 0x50);
    EXPECT_EQ(static_cast<unsigned char>(res->body[1]), 0x4B);
    EXPECT_EQ(static_cast<unsigned char>(res->body[2]), 0x03);
    EXPECT_EQ(static_cast<unsigned char>(res->body[3]), 0x04);
}

// checks if response outputs 404 Not Found error to the browser when attempting to access a missing file
// Expected result: PASS
TEST_F(StaticFileHandlerTest, Returns404ForMissingFile) {
    request req;
    req.method = "GET";
    req.uri = "/static/nonexistent.file";
    req.http_version = "HTTP/1.1";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->http_version, "HTTP/1.1");
    EXPECT_EQ(res->status_code, 404);
    EXPECT_EQ(res->reason_phrase, "Not Found");
    EXPECT_EQ(res->body, "404 Not Found");
}

// checks if response outputs 404 Not Found error to the browser when attempting to access a missing file
// Expected result: PASS
TEST_F(StaticFileHandlerTest, PreventsParentDirectoryAccess) {
    // Attempt to traverse up from the mount point
    request req;
    req.method = "GET";
    req.uri = "/static/../secret.txt";  // tries to access parent directory
    req.http_version = "HTTP/1.1";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->http_version, "HTTP/1.1");
    EXPECT_EQ(res->status_code, 404);
    EXPECT_EQ(res->reason_phrase, "Not Found");
    EXPECT_EQ(res->body, "404 Not Found");
}
// checks if response outputs style.css
// Expected result: PASS
TEST_F(StaticFileHandlerTest, ServesCssFile) {
    request req;
    req.method = "GET";
    req.uri = "/static/style.css";
    req.http_version = "HTTP/1.1";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_EQ(res->headers.at("Content-Type"), "text/css");
    EXPECT_FALSE(res->body.empty());
    // Optionally: make sure it contains at least one CSS rule character
    EXPECT_NE(res->body.find("{"), std::string::npos);
}
// checks if response outputs script.js
// Expected result: PASS
TEST_F(StaticFileHandlerTest, ServesJsFile) {
    request req;
    req.method = "GET";
    req.uri = "/static/script.js";
    req.http_version = "HTTP/1.1";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_EQ(res->headers.at("Content-Type"), "application/javascript");
    EXPECT_FALSE(res->body.empty());
    // Optionally: look for a common JS token
    EXPECT_NE(res->body.find("function"), std::string::npos);
}
// checks if response outputs user.json
// Expected result: PASS
TEST_F(StaticFileHandlerTest, ServesJsonFile) {
    request req;
    req.method = "GET";
    req.uri = "/static/user.json";
    req.http_version = "HTTP/1.1";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_EQ(res->headers.at("Content-Type"), "application/json");
    EXPECT_FALSE(res->body.empty());
    // Should start with a JSON object or array
    EXPECT_TRUE(res->body.front() == '{' || res->body.front() == '[');
}
// checks if response outputs ucla.png
// Expected result: PASS
TEST_F(StaticFileHandlerTest, ServesPngFile) {
    request req;
    req.method = "GET";
    req.uri = "/static/ucla.png";
    req.http_version = "HTTP/1.1";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_EQ(res->headers.at("Content-Type"), "image/png");
    EXPECT_EQ(res->headers.at("Content-Length"), std::to_string(res->body.size()));
    ASSERT_GE(res->body.size(), 8u);
    // PNG magic bytes: 0x89 'P' 'N' 'G' 0x0D 0x0A 0x1A 0x0A
    unsigned char const* data = reinterpret_cast<unsigned char const*>(res->body.data());
    EXPECT_EQ(data[0], 0x89);
    EXPECT_EQ(data[1], 'P');
    EXPECT_EQ(data[2], 'N');
    EXPECT_EQ(data[3], 'G');
}
// checks if response outputs sunshine.jpg
// Expected result: PASS
TEST_F(StaticFileHandlerTest, ServesJpgFile) {
    request req;
    req.method = "GET";
    req.uri = "/static/sunshine.jpg";
    req.http_version = "HTTP/1.1";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_EQ(res->headers.at("Content-Type"), "image/jpeg");
    EXPECT_EQ(res->headers.at("Content-Length"), std::to_string(res->body.size()));
    ASSERT_GE(res->body.size(), 2u);
    unsigned char const* data = reinterpret_cast<unsigned char const*>(res->body.data());
    EXPECT_EQ(data[0], 0xFF);
    EXPECT_EQ(data[1], 0xD8);
}
