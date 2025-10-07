#include "gtest/gtest.h"
#include "nginx_config_parser.h"
#include "config_interpreter.h"

class ConfigInterpreterTest : public ::testing::Test {
  protected:
    NginxConfigParser parser;
    NginxConfig out_config;
};


// --------- Happy path tests ---------

// Config file can be opened and parse 
// Expected result: PASS 
TEST_F(ConfigInterpreterTest, ParseOpenConfigFile) {
    std::ifstream out_config("test_configs/interpreter_configs/unnested_listen_port");
    NginxConfig config;
    EXPECT_NO_THROW(process_config_file(out_config, config));
}

// Unnested listen port
// Expected result: PASS
TEST_F(ConfigInterpreterTest, UnnestedListenPort) {
    std::ifstream out_config("test_configs/interpreter_configs/unnested_listen_port");
    NginxConfig config;
    process_config_file(out_config, config);
    EXPECT_EQ(find_listen_port(&config), 80);
}

// Nested listen port
// Expected result: PASS
TEST_F(ConfigInterpreterTest, NestedListenPort) {
    std::ifstream out_config("test_configs/interpreter_configs/nested_listen_port");
    NginxConfig config;
    process_config_file(out_config, config);
    EXPECT_EQ(find_listen_port(&config), 80);
}


// Extract valid handler configs 
// Expected result: PASS 
TEST_F(ConfigInterpreterTest, ExtractHandlerConfigs_ValidConfig) {
    std::ifstream out_config("test_configs/interpreter_configs/valid_config");
    NginxConfig config;
    process_config_file(out_config, config);
    std::vector<ConfigStruct> result = extract_handler_configs(&config);

    ASSERT_EQ(result.size(), 2);

    EXPECT_EQ(result[0].uri, "/echo");
    EXPECT_EQ(result[0].handler, "EchoHandler");
    EXPECT_TRUE(result[0].args.empty());

    EXPECT_EQ(result[1].uri, "/static");
    EXPECT_EQ(result[1].handler, "StaticFileHandler");
    EXPECT_EQ(result[1].args.at("mount_point"), "/static");
    EXPECT_EQ(result[1].args.at("doc_root"), "./files");
}

// --------- Unhappy path tests ---------

// Invalid port number
// Expected result: FAIL
TEST_F(ConfigInterpreterTest, InvalidPortNumber) {
    std::ifstream out_config("test_configs/interpreter_configs/invalid_port_number");
    NginxConfig config;
    process_config_file(out_config, config);
    EXPECT_THROW({
        find_listen_port(&config);
    }, std::runtime_error);
}

// No listen directive found 
// Expected result: FAIL
TEST_F(ConfigInterpreterTest, NoListenDirectiveFound) {
    std::ifstream out_config("test_configs/interpreter_configs/no_listen_directive_found");
    NginxConfig config;
    process_config_file(out_config, config);
    EXPECT_THROW({
        find_listen_port(&config);
    }, std::runtime_error);
}

// Config file cannot be opened
// Expected result: FAIL 
TEST_F(ConfigInterpreterTest, CantOpenConfigFile) {
    std::ifstream out_config("invalid_path");
    NginxConfig config;
    EXPECT_THROW({
        process_config_file(out_config, config);
    }, std::runtime_error);
}

// File cannot be parsed 
// Expected result: FAIL 
TEST_F(ConfigInterpreterTest, FileCannotBeParsed) {
    std::ifstream out_config("test_configs/interpreter_configs/cannot_be_parsed");
    NginxConfig config;
    EXPECT_THROW({
        process_config_file(out_config, config);
    }, std::runtime_error);
}

// Duplicate locations defined 
// Expected result: FAIL 
TEST_F(ConfigInterpreterTest, DuplicateLocations) {
    std::ifstream out_config("test_configs/interpreter_configs/duplicate_locations_config");
    NginxConfig config;
    process_config_file(out_config, config);
    std::vector<ConfigStruct> config_structs = extract_handler_configs(&config);
    EXPECT_THROW({
        create_uri_to_config_map(config_structs);
    }, std::runtime_error);
}

// Trailing slashes
// Expected result: FAIL
TEST_F(ConfigInterpreterTest, TrailingSlashes) {
    std::ifstream out_config("test_configs/interpreter_configs/trailing_slash_config");
    NginxConfig config;
    process_config_file(out_config, config);
    EXPECT_THROW({
        std::vector<ConfigStruct> config_structs = extract_handler_configs(&config);
    }, std::runtime_error);
}