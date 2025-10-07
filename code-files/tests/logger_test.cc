#include <filesystem>
#include <regex>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include "gtest/gtest.h"
#include <regex>
#include "logger.h"

// --------- Happy path tests ---------

// Logger handles an empty log message without crashing
// Expected result: PASS
TEST(LoggerTest, HandlesEmptyLogMessage) {
    Logger::init();
    BOOST_LOG_SEV(Logger::get(), boost::log::trivial::info) << "";

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // If it doesn't crash, it's a pass.
    SUCCEED();
}
