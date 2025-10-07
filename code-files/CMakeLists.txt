cmake_minimum_required(VERSION 3.10.0)
project(server-ihardlyknowher)

# Prevent running cmake in the source directory to avoid cluttering it with build files
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed.
    Please make a new directory (called a build directory) and run CMake from there.
    You may need to remove CMakeCache.txt." )
endif()

# Enable debug builds by default if in development shell
if (CMAKE_BUILD_TYPE STREQUAL "" AND "$ENV{DEVEL_SHELL}" STREQUAL "1")
    message(STATUS "Setting debug build type by default in devel shell")
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Output binaries to a subdirectory "bin"
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable cmake testing
include(CTest)
enable_testing()

# Enable GoogleTest
include(GoogleTest)
add_subdirectory(/usr/src/googletest googletest)

# Enable Boost
# Use static libraries so binaries can be deployed without a full boost install
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.50 REQUIRED COMPONENTS system log log_setup)
message(STATUS "Boost version: ${Boost_VERSION}")

# Include directories
include_directories(include)

# --- Libraries ---
# Server Library
add_library(server_lib 
  src/server.cc
  src/session.cc
  src/echo_handler.cc
  src/static_file_handler.cc
  src/crud_handler.cc
  src/res_req_helpers.cc
  src/request_handler_factory.cc
  src/trie.cc  
  src/file_system.cc
  src/health_handler.cc
  src/sleep_handler.cc
  src/quiz_handler.cc
  src/result_handler.cc
  src/create_quiz_handler.cc
)

target_link_libraries(server_lib gtest_main)

# Config Parser Library
add_library(config_parser_lib 
  src/nginx_config.cc
  src/nginx_config_parser.cc
)
target_link_libraries(config_parser_lib logger_lib gtest_main)

# Config Interpreter Library
add_library(config_interpreter_lib 
  src/config_interpreter.cc
)
target_link_libraries(config_interpreter_lib logger_lib gtest_main)

# Logger Library
add_library(logger_lib 
  src/logger.cc
)
target_link_libraries(logger_lib Boost::log_setup Boost::log)

# --- Executables ---

# Main Webserver Executable
add_executable(webserver 
  src/server_main.cc
  src/server.cc
  src/session.cc
  src/nginx_config.cc
  src/nginx_config_parser.cc
  src/config_interpreter.cc
  src/logger.cc
  src/echo_handler.cc
  src/static_file_handler.cc
  src/not_found_handler.cc
  src/crud_handler.cc
  src/res_req_helpers.cc
  src/request_handler_factory.cc
  src/trie.cc      
  src/file_system.cc
  src/health_handler.cc
  src/sleep_handler.cc
  src/quiz_handler.cc
  src/result_handler.cc
  src/create_quiz_handler.cc
)
target_link_libraries(webserver Boost::system Boost::log_setup Boost::log logger_lib)

# Server Tests
add_executable(server_test tests/server_test.cc)
target_link_libraries(server_test server_lib logger_lib gtest_main)
gtest_discover_tests(server_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Session Tests
add_executable(session_test tests/session_test.cc src/not_found_handler.cc)
target_link_libraries(session_test server_lib logger_lib gtest_main)
gtest_discover_tests(session_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Config Parser Tests
add_executable(config_parser_test tests/config_parser_test.cc)
target_link_libraries(config_parser_test config_parser_lib gtest_main)
gtest_discover_tests(config_parser_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Config Interpreter Tests
add_executable(config_interpreter_test tests/config_interpreter_test.cc)
target_link_libraries(config_interpreter_test config_interpreter_lib config_parser_lib gtest_main)
gtest_discover_tests(config_interpreter_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Logger Tests
add_executable(logger_test tests/logger_test.cc)
target_link_libraries(logger_test logger_lib gtest_main)
gtest_discover_tests(logger_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Request/Response Helper Tests
add_executable(res_req_helpers_test
  tests/res_req_helpers_test.cc
  src/res_req_helpers.cc
)
target_link_libraries(res_req_helpers_test gtest_main ${Boost_LIBRARIES} logger_lib)
gtest_discover_tests(res_req_helpers_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Echo Handler Test
add_executable(echo_handler_test
  tests/echo_handler_test.cc
  src/echo_handler.cc
)
target_link_libraries(echo_handler_test gtest_main ${Boost_LIBRARIES} logger_lib)
gtest_discover_tests(echo_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Static File Handler Test
add_executable(static_file_handler_test
  tests/static_file_handler_test.cc
)
target_link_libraries(static_file_handler_test PRIVATE server_lib logger_lib ${Boost_LIBRARIES} gtest_main)
target_include_directories(static_file_handler_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
gtest_discover_tests(static_file_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# CRUD Handler Test
add_executable(crud_handler_test
  tests/crud_handler_test.cc
)
target_link_libraries(crud_handler_test PRIVATE server_lib logger_lib ${Boost_LIBRARIES} GTest::gmock gtest_main)
target_include_directories(crud_handler_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
gtest_discover_tests(crud_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Health Handler Test
add_executable(health_handler_test
  tests/health_handler_test.cc
)
target_link_libraries(health_handler_test PRIVATE server_lib logger_lib ${Boost_LIBRARIES} gtest_main)
target_include_directories(health_handler_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
gtest_discover_tests(health_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

#Multithreading Test
add_executable(concurrency_test
  tests/concurrency_test.cc
)
target_link_libraries(concurrency_test PRIVATE server_lib logger_lib ${Boost_LIBRARIES} gtest_main)
gtest_discover_tests(concurrency_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Quiz Handler Test
add_executable(quiz_handler_test
  tests/quiz_handler_test.cc
)
target_link_libraries(quiz_handler_test PRIVATE server_lib logger_lib ${Boost_LIBRARIES} gtest_main)
target_include_directories(quiz_handler_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
gtest_discover_tests(quiz_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Result Handler Test
add_executable(result_handler_test
  tests/result_handler_test.cc
)
target_link_libraries(result_handler_test PRIVATE server_lib logger_lib ${Boost_LIBRARIES} gtest_main)
target_include_directories(result_handler_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
gtest_discover_tests(result_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Create Quiz Handler Test
add_executable(create_quiz_handler_test
  tests/create_quiz_handler_test.cc
)

target_link_libraries(create_quiz_handler_test PRIVATE server_lib logger_lib ${Boost_LIBRARIES} gtest_main gmock)
target_include_directories(create_quiz_handler_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
gtest_discover_tests(create_quiz_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# --- Code Coverage ---
# Include code coverage configuration and generate report
include(c/CodeCoverageReportConfig.cmake)
generate_coverage_report(
  TARGETS config_parser_lib config_interpreter_lib server_lib
  TESTS config_parser_test config_interpreter_test session_test server_test echo_handler_test logger_test static_file_handler_test crud_handler_test health_handler_test res_req_helpers_test quiz_handler_test result_handler_test create_quiz_handler_test
)

# --- Bash Integration Test ---
configure_file(${CMAKE_SOURCE_DIR}/tests/app/index.txt
               ${CMAKE_BINARY_DIR}/tests/app/index.txt
               COPYONLY)
add_test(NAME WebServerIntegrationTest
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_test.sh)
