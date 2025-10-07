#ifndef LOGGER_H
#define LOGGER_H

#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>

class Logger {
public:
    // Initializes the global logging system with console and file output.
    static void init();
    
    // Returns a reference to the global severity logger instance.
    // @return: reference to the logger instance.
    static boost::log::sources::severity_logger<boost::log::trivial::severity_level>& get();

private:
    // Static member to hold the global severity logger instance
    static boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg_;
};

// Macros for easier logging at different severity levels
#define LOG_TRACE BOOST_LOG_SEV(Logger::get(), boost::log::trivial::trace)   // For tracing log messages
#define LOG_DEBUG BOOST_LOG_SEV(Logger::get(), boost::log::trivial::debug)   // For debug log messages
#define LOG_INFO  BOOST_LOG_SEV(Logger::get(), boost::log::trivial::info)    // For informational log messages
#define LOG_WARNING BOOST_LOG_SEV(Logger::get(), boost::log::trivial::warning)  // For warning log messages
#define LOG_ERROR BOOST_LOG_SEV(Logger::get(), boost::log::trivial::error)    // For error log messages
#define LOG_FATAL BOOST_LOG_SEV(Logger::get(), boost::log::trivial::fatal)    // For fatal error log messages

#endif // LOGGER_H