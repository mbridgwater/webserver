#include <gtest/gtest.h>
#include "res_req_helpers.h"
#include "request.h"
#include "response.h"

// parse a request and create a correct request packet
TEST(SessionTestFixture, ParseRequestBasicGet) {
    std::string raw_request =
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: TestAgent\r\n"
        "\r\n"
        "body content";

    request req = parse_request(raw_request);
    // Expected Result: PASS if request method is GET
    EXPECT_EQ(req.method, "GET");
    // Expected Result: PASS if request uri is /index.html
    EXPECT_EQ(req.uri, "/index.html");
    // Expected Result: PASS if request http version is HTTP/1.1
    EXPECT_EQ(req.http_version, "HTTP/1.1");
    // Expected Result: PASS if request headers size is 2
    ASSERT_EQ(req.headers.size(), 2);
    // Expected Result: PASS if request headers Host is localhost
    EXPECT_EQ(req.headers["Host"], "localhost");
    // Expected Result: PASS if request headers User-Agent is TestAgent
    EXPECT_EQ(req.headers["User-Agent"], "TestAgent");
    // Expected Result: PASS if request body is body content
    EXPECT_EQ(req.body, "body content");
}

// // serialize an incoming response and create a correct response packet
TEST(SessionTestFixture, SerializeResponseBasic) {
    response res;
    res.http_version = "HTTP/1.1";
    res.status_code = 200;
    res.reason_phrase = "OK";
    res.headers["Content-Type"] = "text/html";
    res.headers["Content-Length"] = "13";
    res.body = "<h1>Hello</h1>";

    // Note: Headers must be included in alphabetical order for the sake of this test because they are stored in a map
    // Order does not matter in actual specification 
    std::string expected_response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "<h1>Hello</h1>";

    // Expected Result: PASS if calling the serialize response function from res_req_helpers.h is the same as the format shown right above
    EXPECT_EQ(serialize_response(res), expected_response);
}