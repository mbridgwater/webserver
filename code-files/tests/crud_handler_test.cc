#include "crud_handler.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "request.h"
#include "response.h"
#include <filesystem>

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

// Mock implementation
class MockFileSystem : public FileSystemInterface {
public:
    // CRUD methods
    MOCK_METHOD((std::pair<bool, std::string>), create_entity,
                (const std::string &), (override));
    MOCK_METHOD((std::pair<bool, std::string>), read_entity,
                (const std::string &, const std::string &), (const, override));
    MOCK_METHOD(bool, write_entity,
                (const std::string &, const std::string &, const std::string &),
                (override));
    MOCK_METHOD(bool, delete_entity,
                (const std::string &, const std::string &), (override));
    MOCK_METHOD((std::pair<bool, std::vector<std::string>>), list_entities,
                (const std::string &), (const, override));
    MOCK_METHOD(const std::string &, get_data_path, (), (const, override));
    MOCK_METHOD(bool, exists, (const std::string&, const std::string&), (const, override));
};

// Test fixture using dependency injection
class CrudHandlerTest : public ::testing::Test {
protected:
    // We give PUT tests a real, writable temp dir because CrudHandler::put
    // still writes directly using std::filesystem (beyond the interface).
    const std::string tmp_dir = "/tmp/crud_test_di/";
    std::shared_ptr<NiceMock<MockFileSystem>> mock_fs;
    CrudHandler handler;

    CrudHandlerTest()
        : mock_fs(std::make_shared<NiceMock<MockFileSystem>>()),
          handler(mock_fs) {}

    void SetUp() override {
        std::filesystem::remove_all(tmp_dir);
        std::filesystem::create_directory(tmp_dir);
        // Provide the path whenever the handler asks for it.
        ON_CALL(*mock_fs, get_data_path()).WillByDefault(ReturnRef(tmp_dir));
    }

    void TearDown() override {
        std::filesystem::remove_all(tmp_dir);
    }
};

// --------------------------- Invalid Request Tests ---------------------------

// Checks if the handler returns 404 Not Found for an invalid API prefix
// Expected result: PASS
TEST_F(CrudHandlerTest, InvalidPrefixReturns404) {
    request req;
    req.method = "GET";
    req.uri    = "/invalidprefix/Shoes/1";

    auto res = handler.handle_request(req);
    EXPECT_EQ(res->status_code, 404);
    EXPECT_EQ(res->reason_phrase, "Not Found");
}

// Checks if the handler returns 400 Bad Request when the entity type is missing
// Expected result: PASS
TEST_F(CrudHandlerTest, MissingEntityReturns400) {
    request req;
    req.method = "GET";
    req.uri    = "/api/";

    auto res = handler.handle_request(req);
    EXPECT_EQ(res->status_code, 400);
    EXPECT_EQ(res->reason_phrase, "Bad Request");
}

// Checks if the handler returns 405 Method Not Allowed for unsupported methods
// Expected result: PASS
TEST_F(CrudHandlerTest, UnsupportedMethodReturns405) {
    request req;
    req.method = "PATCH";               // not supported
    req.uri    = "/api/Shoes/1";

    auto res = handler.handle_request(req);
    EXPECT_EQ(res->status_code, 405);
    EXPECT_EQ(res->reason_phrase, "Method Not Allowed");
}


// --------------------------- POST Tests ---------------------------

// Checks if a POST request creates an entity and returns a 201 Created status
// Expected result: PASS
TEST_F(CrudHandlerTest, PostRequestCreatesEntity) {
    const std::string body = "{\"name\": \"Air Max\", \"size\": 10}";

    // Expect sequence: create -> write
    EXPECT_CALL(*mock_fs, create_entity("Shoes"))
        .WillOnce(Return(std::make_pair(true, "gen‑id‑123")));
    EXPECT_CALL(*mock_fs,
                write_entity("Shoes", "gen‑id‑123", body))
        .WillOnce(Return(true));

    request req;
    req.method = "POST";
    req.uri    = "/api/Shoes";
    req.body   = body;

    auto res = handler.handle_request(req);
    EXPECT_EQ(res->status_code, 201);
    EXPECT_EQ(res->reason_phrase, "Created");
    EXPECT_NE(res->body.find("\"id\": \"gen‑id‑123\""), std::string::npos);
}

// Checks if a POST request with invalid JSON returns a 400 Bad Request
// Expected result: PASS
TEST_F(CrudHandlerTest, PostRequestWithInvalidJsonReturns400) {
    request req;
    req.method = "POST";
    req.uri    = "/api/Shoes";
    req.body   = "{\"name\": \"Air Max\", \"size\": 10"; // missing }

    // No FileSystem calls expected because JSON validation fails first.
    EXPECT_CALL(*mock_fs, create_entity(_)).Times(0);

    auto res = handler.handle_request(req);
    EXPECT_EQ(res->status_code, 400);
    EXPECT_EQ(res->reason_phrase, "Bad Format");
    EXPECT_EQ(res->body, "Invalid JSON format\n");
}

// --------------------------- GET Tests ---------------------------

// Checks if a GET request for a nonexistent ID returns 404
// Expected result: PASS
TEST_F(CrudHandlerTest, GetRequestWithInvalidIdReturns404) {
    EXPECT_CALL(*mock_fs,
                read_entity("Shoes", "bad‑id"))
        .WillOnce(Return(std::make_pair(false, "")));

    request req;
    req.method = "GET";
    req.uri    = "/api/Shoes/bad‑id";
    req.body   = "";
    auto res = handler.handle_request(req);
    EXPECT_EQ(res->status_code, 404);
    EXPECT_EQ(res->reason_phrase, "Not found");
    EXPECT_EQ(res->body, "Entity not found\n");
}

// Checks if a valid GET request returns the expected entity
TEST_F(CrudHandlerTest, GetRequestWithValidIdReturnsEntity) {
    const std::string entity_type = "Shoes";
    const std::string entity_id = "123";
    const std::string expected_data = R"({"name": "Air Max", "size": 10})";

    EXPECT_CALL(*mock_fs, read_entity(entity_type, entity_id))
        .WillOnce(Return(std::make_pair(true, expected_data)));

    request req;
    req.method = "GET";
    req.uri    = "/api/Shoes/123";
    req.body   = "";
    auto res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_EQ(res->body, expected_data);
}


// Checks if a GET request on an entity type that doesn't exist returns 404
// Expected result: PASS
TEST_F(CrudHandlerTest, GetRequestForUnknownEntityTypeReturns404) {
    EXPECT_CALL(*mock_fs, list_entities("UnknownEntity"))
        .WillOnce(Return(std::make_pair(false,
                                        std::vector<std::string>{})));

    request req;
    req.method = "GET";
    req.uri    = "/api/UnknownEntity";
    req.body   = "";
    auto res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 404);
    EXPECT_EQ(res->reason_phrase, "Not Found");
    EXPECT_EQ(res->body, "Entity type does not exist\n");
}


// Checks if a GET request to list all entities returns an array of IDs
// Expected result: PASS
TEST_F(CrudHandlerTest, GetRequestWithoutIdReturnsListOfEntities) {
    // Hand‑crafted IDs
    std::vector<std::string> ids = {"id1", "id2"};

    // list_entities should be hit once.
    EXPECT_CALL(*mock_fs, list_entities("Shoes"))
        .WillOnce(Return(std::make_pair(true, ids)));

    request req;
    req.method = "GET";
    req.uri    = "/api/Shoes";
    req.body   = "";
    auto res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_NE(res->body.find("id1"), std::string::npos);
    EXPECT_NE(res->body.find("id2"), std::string::npos);
}

// --------------------------- PUT Tests ---------------------------

// Checks if a PUT request without an ID returns a 400 Bad Request
TEST_F(CrudHandlerTest, PutRequestWithoutIdReturns400) {
    request req;
    req.method = "PUT";
    req.uri    = "/api/Shoes";
    req.body   = R"({"name": "Air Max", "size": 10})";
    auto res = handler.handle_request(req);
    EXPECT_EQ(res->status_code, 400);
    EXPECT_EQ(res->reason_phrase, "Bad Request");
    EXPECT_EQ(res->body, "ID must be specified for PUT operation\n");
}

// Checks if a PUT request with empty body returns a 400 Bad Request
TEST_F(CrudHandlerTest, PutRequestWithEmptyBodyReturns400) {
    request req;
    req.method = "PUT";
    req.uri    = "/api/Shoes/test-id";
    req.body   = "";
    auto res = handler.handle_request(req);
    EXPECT_EQ(res->status_code, 400);
    EXPECT_EQ(res->reason_phrase, "Bad Request");
    EXPECT_EQ(res->body, "Empty or whitespace-only body\n");
}

// Checks if a valid PUT request updates the entity and returns 200
TEST_F(CrudHandlerTest, PutRequestWithValidIdUpdatesEntity) {
    const std::string entity_type = "Shoes";
    const std::string entity_id = "123";
    const std::string updated_body = R"({"name": "Updated Max", "size": 11})";

    EXPECT_CALL(*mock_fs, exists(entity_type, entity_id))
    .WillOnce(Return(true));
    EXPECT_CALL(*mock_fs, write_entity(entity_type, entity_id, updated_body))
    .WillOnce(Return(true));

    request req;
    req.method = "PUT";
    req.uri    = "/api/Shoes/123";
    req.body   = updated_body;
    auto res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_NE(res->body.find("\"id\": \"123\""), std::string::npos);
}


// --------------------------- DELETE Tests ---------------------------

// Checks if a DELETE request removes an existing entity
TEST_F(CrudHandlerTest, DeleteRequestRemovesEntity) {
    EXPECT_CALL(*mock_fs, delete_entity("Shoes", "del‑id"))
        .WillOnce(Return(true));

    request req;
    req.method = "DELETE";
    req.uri    = "/api/Shoes/del‑id";
    req.body   = "";
    auto res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_NE(res->body.find("\"deleted\": true"), std::string::npos);
}

// Checks if a DELETE request for a nonexistent entity returns 404 Not Found
TEST_F(CrudHandlerTest, DeleteRequestForNonexistentEntityReturns404) {
    EXPECT_CALL(*mock_fs, delete_entity("Shoes", "ghost"))
        .WillOnce(Return(false));

    request req;
    req.method = "DELETE";
    req.uri    = "/api/Shoes/ghost";
    req.body   = "";
    auto res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 404);
    EXPECT_EQ(res->reason_phrase, "Not Found");
}

// Checks if a DELETE request without an ID returns a 400 Bad Request
TEST_F(CrudHandlerTest, DeleteRequestWithoutIdReturns400) {
    request req;
    req.method = "DELETE";
    req.uri    = "/api/Shoes";
    req.body   = "";
    auto res = handler.handle_request(req);
    EXPECT_EQ(res->status_code, 400);
    EXPECT_EQ(res->reason_phrase, "Bad Request");
}

// Checks if a DELETE request for an entity in a non-existent entity type still returns 404
TEST_F(CrudHandlerTest, DeleteRequestForUnknownEntityTypeReturns404) {
    EXPECT_CALL(*mock_fs,
                delete_entity("NonExistentEntityType", "some-id"))
        .WillOnce(Return(false));

    request req;
    req.method = "DELETE";
    req.uri    = "/api/NonExistentEntityType/some-id";
    req.body   = "";
    auto res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 404);
    EXPECT_EQ(res->reason_phrase, "Not Found");
    EXPECT_EQ(res->body, "Entity not found\n");
}
