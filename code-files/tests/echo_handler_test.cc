#include <gtest/gtest.h>
#include "echo_handler.h"
#include "request.h"
#include "response.h"

// call echo handler on fake http request to test a certain response output
TEST(EchoHandlerTest, EchoesRequestCorrectly) {
    EchoHandler handler;

    // Create a fake HTTP request
    request req;
    req.method = "GET";
    req.uri = "/echo";
    req.http_version = "HTTP/1.1";
    req.headers["Host"] = "localhost";
    req.body = "";

    // Call the handler
    std::unique_ptr<response> res = handler.handle_request(req);

    // Check the response

// ----------- HAPPY PATH TESTS ----------
    // Expected result: PASS if http version is HTTP/1.1
    EXPECT_EQ(res->http_version, "HTTP/1.1");
    // Expected result: PASS if status code is 200
    EXPECT_EQ(res->status_code, 200);
    // Expected result: PASS if reason phrase is OK
    EXPECT_EQ(res->reason_phrase, "OK");
    // Expected result: PASS if content type is text/plain
    EXPECT_EQ(res->headers.at("Content-Type"), "text/plain");

// ---------- UNHAPPY PATH TESTS ----------
    // Expected result: FAIL if body empty
    EXPECT_FALSE(res->body.empty());
    // Expected result: FAIL if body has GET /echo HTTP/1.1 or Host: localhost in it
    EXPECT_NE(res->body.find("GET /echo HTTP/1.1"), std::string::npos);
    EXPECT_NE(res->body.find("Host: localhost"), std::string::npos);
}
