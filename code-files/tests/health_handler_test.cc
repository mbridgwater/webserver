#include <gtest/gtest.h>
#include "health_handler.h"
#include "request.h"
#include "response.h"

// Unit tests for HealthHandler
// Expected behavior: Always returns 200 OK with plain text body "OK"
class HealthHandlerTest : public ::testing::Test {
protected:
    HealthHandler handler;  
};

// Checks if the response returns 200 OK and body = "OK" when /health is requested
// Expected result: PASS
TEST_F(HealthHandlerTest, ReturnsOKStatusAndBody) {
    request req;
    req.method = "GET";
    req.uri = "/health";
    req.http_version = "HTTP/1.1";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->http_version, "HTTP/1.1");
    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_EQ(res->headers.at("Content-Type"), "text/plain");
    EXPECT_EQ(res->headers.at("Content-Length"), std::to_string(res->body.size()));
    EXPECT_EQ(res->body, "OK");
}

// Checks if handler still returns 200 OK for a non-standard method 
// Expected result: PASS
TEST_F(HealthHandlerTest, AcceptsNonGETMethod) {
    request req;
    req.method = "POST";
    req.uri = "/health";
    req.http_version = "HTTP/1.0";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->http_version, "HTTP/1.0");
    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_EQ(res->headers.at("Content-Type"), "text/plain");
    EXPECT_EQ(res->body, "OK");
}

// Checks if handler works when called with a different but valid HTTP version
// Expected result: PASS
TEST_F(HealthHandlerTest, SupportsHttp10) {
    request req;
    req.method = "GET";
    req.uri = "/health";
    req.http_version = "HTTP/1.0";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->http_version, "HTTP/1.0");
    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->body, "OK");
}
