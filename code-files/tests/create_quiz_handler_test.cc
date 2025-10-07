#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "create_quiz_handler.h"
#include "request.h"
#include "response.h"
#include "file_system_interface.h"
#include <nlohmann/json.hpp>

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

// Mock file system
class MockFileSystem : public FileSystemInterface {
public:
    MOCK_METHOD((std::pair<bool, std::string>), create_entity, (const std::string &), (override));
    MOCK_METHOD((std::pair<bool, std::string>), read_entity, (const std::string &, const std::string &), (const, override));
    MOCK_METHOD(bool, write_entity, (const std::string &, const std::string &, const std::string &), (override));
    MOCK_METHOD(bool, delete_entity, (const std::string &, const std::string &), (override));
    MOCK_METHOD((std::pair<bool, std::vector<std::string>>), list_entities, (const std::string &), (const, override));
    MOCK_METHOD(const std::string &, get_data_path, (), (const, override));
    MOCK_METHOD(bool, exists, (const std::string&, const std::string&), (const, override));
};

// Test basic quiz creation
TEST(CreateQuizHandlerTest, StoresQuizJsonOnPost) {
    auto mock_fs = std::make_shared<NiceMock<MockFileSystem>>();
    CreateQuizHandler handler(mock_fs);

    // Setup valid filesystem path
    std::string test_data_path = "./tmp_test_quizzes";
    std::filesystem::create_directories(test_data_path);

    // Setup expectation for get_data_path
    EXPECT_CALL(*mock_fs, get_data_path())
        .WillRepeatedly(::testing::ReturnRef(test_data_path));

    // Create the file ahead of time since write_entity expects it
    std::ofstream(test_data_path + "/testquiz.json").close();

    // Mock the write_entity behavior
    EXPECT_CALL(*mock_fs, write_entity("", "testquiz.json", ::testing::_))
        .WillOnce([](const std::string&, const std::string&, const std::string& data) {
            auto j = nlohmann::json::parse(data);
            EXPECT_EQ(j["title"], "Test Quiz");
            EXPECT_EQ(j["questions"].size(), 1);
            EXPECT_EQ(j["questions"][0]["prompt"], "Favorite Color?");
            EXPECT_EQ(j["results"]["cs"]["title"], "CS");
            return true;
        });

    request req;
    req.method = "POST";
    req.uri = "/quiz/create";
    req.http_version = "HTTP/1.1";

    req.body =
        "quiz_id=testquiz&title=Test+Quiz"
        "&q0_prompt=Favorite+Color%3F"
        "&q0_opt0_text=Blue&q0_opt0_val=cs"
        "&q0_opt1_text=Red&q0_opt1_val=bio"
        "&q0_opt2_text=Green&q0_opt2_val=ps"
        "&q0_opt3_text=Yellow&q0_opt3_val=be"
        "&result_0_key=cs&result_0_title=CS&result_0_desc=CS+desc&result_0_img=/img/cs.jpg"
        "&result_1_key=bio&result_1_title=Bio&result_1_desc=Bio+desc&result_1_img=/img/bio.jpg"
        "&result_2_key=ps&result_2_title=PoliSci&result_2_desc=PS+desc&result_2_img=/img/ps.jpg"
        "&result_3_key=be&result_3_title=BizEcon&result_3_desc=BE+desc&result_3_img=/img/be.jpg";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_NE(res->body.find("testquiz"), std::string::npos);
}

// Test complex quiz creation
TEST(CreateQuizHandlerTest, StoresRobustMultiQuestionQuiz) {
    auto mock_fs = std::make_shared<NiceMock<MockFileSystem>>();
    CreateQuizHandler handler(mock_fs);

    std::string test_data_path = "./tmp_test_quizzes_complex";
    std::filesystem::create_directories(test_data_path);

    EXPECT_CALL(*mock_fs, get_data_path())
        .WillRepeatedly(::testing::ReturnRef(test_data_path));

    std::ofstream(test_data_path + "/robustquiz.json").close();

    std::string captured_json;
    EXPECT_CALL(*mock_fs, write_entity("", "robustquiz.json", ::testing::_))
        .WillOnce([&captured_json](const std::string&, const std::string&, const std::string& data) {
            captured_json = data;
            return true;
        });

    request req;
    req.method = "POST";
    req.uri = "/quiz/create";
    req.http_version = "HTTP/1.1";

    req.body =
        "quiz_id=robustquiz&title=Robust+Quiz"
        "&q0_prompt=Choose+a+major"
        "&q0_opt0_text=CS&q0_opt0_val=cs"
        "&q0_opt1_text=Bio&q0_opt1_val=bio"
        "&q0_opt2_text=Econ&q0_opt2_val=be"
        "&q0_opt3_text=PoliSci&q0_opt3_val=ps"
        "&q1_prompt=Pick+a+meal"
        "&q1_opt0_text=BPlate&q1_opt0_val=cs"
        "&q1_opt2_text=FEAST&q1_opt2_val=ps"
        "&q2_prompt=Favorite+dorm%3F"
        "&q2_opt0_text=Dykstra&q2_opt0_val=cs"
        "&q2_opt1_text=Hedrick&q2_opt1_val=bio"
        "&q2_opt2_text=Sproul&q2_opt2_val=ps"
        "&q2_opt3_text=Rieber&q2_opt3_val=be"
        "&q3_opt0_text=ignore&q3_opt0_val=skip"
        "&result_0_key=cs&result_0_title=Computer+Science&result_0_desc=You+love+logic!&result_0_img=/img/cs.jpg"
        "&result_1_key=bio&result_1_title=Biology&result_1_desc=You+love+cells!&result_1_img=/img/bio.jpg"
        "&result_2_key=ps&result_2_title=Political+Science&result_2_desc=You+love+debating!&result_2_img=/img/ps.jpg"
        "&result_3_key=be&result_3_title=Biz+Econ&result_3_desc=You+love+money!&result_3_img=/img/be.jpg";

    std::unique_ptr<response> res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 200);
    EXPECT_EQ(res->reason_phrase, "OK");
    EXPECT_NE(res->body.find("robustquiz"), std::string::npos);

    //  Parse the captured JSON and assert fields safely
    auto j = nlohmann::json::parse(captured_json);

    EXPECT_EQ(j["title"], "Robust Quiz");

    ASSERT_EQ(j["questions"].size(), 3);
    EXPECT_EQ(j["questions"][0]["prompt"], "Choose a major");
    EXPECT_EQ(j["questions"][0]["options"].size(), 4);

    EXPECT_EQ(j["questions"][1]["prompt"], "Pick a meal");
    EXPECT_EQ(j["questions"][1]["options"].size(), 2);

    EXPECT_EQ(j["questions"][2]["prompt"], "Favorite dorm?");
    EXPECT_EQ(j["questions"][2]["options"].size(), 4);

    EXPECT_EQ(j["results"]["cs"]["title"], "Computer Science");
    EXPECT_EQ(j["results"]["ps"]["description"], "You love debating!");
}

// Test invalid file names 
TEST(CreateQuizHandlerTest, RejectsInvalidQuizIdWithBadCharacters) {
    auto mock_fs = std::make_shared<NiceMock<MockFileSystem>>();
    CreateQuizHandler handler(mock_fs);

    // Should not attempt to write when quiz_id is invalid
    EXPECT_CALL(*mock_fs, write_entity(_, _, _)).Times(0);

    request req;
    req.method = "POST";
    req.uri = "/quiz/create";
    req.http_version = "HTTP/1.1";

    // Invalid quiz_id: contains space and slash
    req.body =
        "quiz_id=bad quiz/id&title=Oops"
        "&q0_prompt=What%3F&q0_opt0_text=A&q0_opt0_val=a"
        "&q0_opt1_text=B&q0_opt1_val=b"
        "&q0_opt2_text=C&q0_opt2_val=c"
        "&q0_opt3_text=D&q0_opt3_val=d"
        "&result_0_key=a&result_0_title=A&result_0_desc=desc"
        "&result_1_key=b&result_1_title=B&result_1_desc=desc";

    std::unique_ptr<response> res = handler.handle_request(req);

    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status_code, 400);
    EXPECT_EQ(res->reason_phrase, "Bad Request");
    EXPECT_NE(res->body.find("Quiz ID"), std::string::npos); // should mention ID is invalid
}

// Inconsistent results returns 400 error 
TEST(CreateQuizHandlerTest, RejectsQuizWithMissingResultDefinitions) {
    auto mock_fs = std::make_shared<NiceMock<MockFileSystem>>();
    CreateQuizHandler handler(mock_fs);

    EXPECT_CALL(*mock_fs, write_entity(_, _, _)).Times(0);  // Must not write anything

    request req;
    req.method = "POST";
    req.uri = "/quiz/create";
    req.http_version = "HTTP/1.1";

    // 'bio' and 'ps' are used but not defined in results
    req.body =
        "quiz_id=badquiz&title=Bad+Quiz"
        "&q0_prompt=Major?"
        "&q0_opt0_text=CS&q0_opt0_val=cs"
        "&q0_opt1_text=Bio&q0_opt1_val=bio"
        "&q0_opt2_text=PoliSci&q0_opt2_val=ps"
        "&q0_opt3_text=Econ&q0_opt3_val=be"
        "&result_0_key=cs&result_0_title=CS&result_0_desc=desc"
        "&result_1_key=be&result_1_title=Econ&result_1_desc=desc";

    std::unique_ptr<response> res = handler.handle_request(req);

    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status_code, 400);
    EXPECT_EQ(res->reason_phrase, "Bad Request");
    std::cout << "RESPONSE BODY:\n" << res->body << std::endl;
    EXPECT_TRUE(
    res->body.find("Missing: bio") != std::string::npos ||
    res->body.find("Missing: ps") != std::string::npos
    );  // Mentions missing key
}

// Too many results causes 400 error
TEST(CreateQuizHandlerTest, RejectsQuizWithTooManyResults) {
    auto mock_fs = std::make_shared<NiceMock<MockFileSystem>>();
    CreateQuizHandler handler(mock_fs);

    EXPECT_CALL(*mock_fs, write_entity(_, _, _)).Times(0);  // Must not write anything

    request req;
    req.method = "POST";
    req.uri = "/quiz/create";
    req.http_version = "HTTP/1.1";

    // Includes 5 result definitions instead of 4
    req.body =
        "quiz_id=toomany&title=Overloaded+Quiz"
        "&q0_prompt=Pick one"
        "&q0_opt0_text=A&q0_opt0_val=r1"
        "&q0_opt1_text=B&q0_opt1_val=r2"
        "&q0_opt2_text=C&q0_opt2_val=r3"
        "&q0_opt3_text=D&q0_opt3_val=r4"
        "&result_0_key=r1&result_0_title=R1&result_0_desc=desc"
        "&result_1_key=r2&result_1_title=R2&result_1_desc=desc"
        "&result_2_key=r3&result_2_title=R3&result_2_desc=desc"
        "&result_3_key=r4&result_3_title=R4&result_3_desc=desc"
        "&result_4_key=extra&result_4_title=Extra&result_4_desc=desc";

    std::unique_ptr<response> res = handler.handle_request(req);

    ASSERT_NE(res, nullptr);
    EXPECT_EQ(res->status_code, 400);
    EXPECT_EQ(res->reason_phrase, "Bad Request");
    EXPECT_NE(res->body.find("Exactly 4 result categories"), std::string::npos);
}

TEST(CreateQuizHandlerTest, RejectsInvalidUTF8InSubmission) {
    std::shared_ptr<FileSystemInterface> fs = std::make_shared<FileSystem>("/tmp/test_quizzes");
    CreateQuizHandler handler(fs);

    request req;
    req.method = "POST";
    req.uri = "/quiz/create";

    // This includes an invalid UTF-8 byte (0x92 is a smart quote in Windows-1252)
    std::string bad_utf8 = "quiz_id=badquiz&title=Bad\x92Title";

    req.body = bad_utf8;

    auto res = handler.handle_request(req);

    EXPECT_EQ(res->status_code, 400);
    EXPECT_NE(res->body.find("invalid characters in field: title"), std::string::npos);
}