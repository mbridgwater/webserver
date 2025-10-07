#include "logger.h"
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/core.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <filesystem>
#include <iostream>

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace fs = std::filesystem;

// Declare the global logger instance for logging
boost::log::sources::severity_logger<boost::log::trivial::severity_level> Logger::lg_;

// Getter for the logger instance
boost::log::sources::severity_logger<boost::log::trivial::severity_level>& Logger::get() {
    return lg_;
}

// Initialize the logging system
void Logger::init() {
    // Check if 'logs' directory exists
    if (!fs::exists("../logs")) {
        fs::create_directory("../logs");
    }
    // Add common attributes such as timestamp, thread id, etc.
    logging::add_common_attributes();

    // Add global attribute for thread ID to include in logs
    logging::core::get()->add_global_attribute("ThreadID", logging::attributes::current_thread_id());

    // Set up console logging with a specific format
    logging::add_console_log(
        std::clog,
        keywords::format = (
            expr::stream
                << "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")  // Timestamp format
                << "] [" << expr::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID")  // Thread ID
                << "] [" << logging::trivial::severity  // Log severity level (info, warning, error, etc.)
                << "] " << expr::smessage  // The log message itself
        )
    );

    // Set up file logging with daily rotation and size-based rotation
    auto sink = logging::add_file_log(
        // Needed to change this to make sure it doesn't rotate too many times
        keywords::file_name = "../logs/server_%Y-%m-%d_%N.log",  // File path pattern (log files will be named by date)
        keywords::rotation_size = 10 * 1024 * 1024,  // Rotate log file after it reaches 10MB
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),  // Rotate daily at midnight
        keywords::open_mode = std::ios_base::app,  // Open log file in append mode
        keywords::format = (
            expr::stream
                << "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")  // Timestamp format
                << "] [" << expr::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID")  // Thread ID
                << "] [" << logging::trivial::severity  // Log severity level
                << "] " << expr::smessage  // Log message
        )
    );
    
    // Automatically flush the log backend to ensure logs are written immediately
    sink->locked_backend()->auto_flush(true);

    // Set the log severity level filter (logs at or above the 'info' level will be logged)
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info
    );
}
