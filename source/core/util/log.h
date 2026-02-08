// ----------------------------------------------------------------------------

/// @brief Logging macros

#ifndef RAYBENCH_UTIL_LOG_MACROS_H_
#define RAYBENCH_UTIL_LOG_MACROS_H_

#include <source_location>

/// @brief Log a debug message
#define RAYBENCH_LOG_DEBUG(message, ...) \
    raybench::util::Log::LogMessage (raybench::util::Log::Severity::debug, \
                                         std::source_location::current (), \
                                         message, ##__VA_ARGS__);

/// @brief Log an info message
#define RAYBENCH_LOG_INFO(message, ...) \
    raybench::util::Log::LogMessage (raybench::util::Log::Severity::info, \
                                         std::source_location::current (), \
                                         message, ##__VA_ARGS__);

/// @brief Log a warning message
#define RAYBENCH_LOG_WARNING(message, ...) \
    raybench::util::Log::LogMessage (raybench::util::Log::Severity::warning, \
                                         std::source_location::current (), \
                                         message, ##__VA_ARGS__);

/// @brief Log an error message
#define RAYBENCH_LOG_ERROR(message, ...) \
    raybench::util::Log::LogMessage (raybench::util::Log::Severity::error, \
                                         std::source_location::current (), \
                                         message, ##__VA_ARGS__);

/// @brief Log a critical message
#define RAYBENCH_LOG_CRITICAL(message, ...) \
    raybench::util::Log::LogMessage (raybench::util::Log::Severity::critical, \
                                         std::source_location::current (), \
                                         message, ##__VA_ARGS__);

/// @brief Write a message to the console
#define RAYBENCH_WRITE_CONSOLE(message, ...) \
    raybench::util::Log::LogMessage (raybench::util::Log::Severity::always, \
                                         std::source_location::current (), \
                                         message, ##__VA_ARGS__);

// ----------------------------------------------------------------------------

/// @brief Log a message only once in the current scope
#define RAYBENCH_LOG_ONCE(X) \
    { \
        static bool logged_once = false; \
        if (!logged_once) { \
            X; \
            logged_once = true; \
        } \
    }

/// @brief Log a debug message only once
#define RAYBENCH_LOG_DEBUG_ONCE(message, ...) \
    RAYBENCH_LOG_ONCE( RAYBENCH_LOG_DEBUG (message, ##__VA_ARGS__) )

/// @brief Log an info message only once
#define RAYBENCH_LOG_INFO_ONCE(message, ...) \
    RAYBENCH_LOG_ONCE( RAYBENCH_LOG_INFO (message, ##__VA_ARGS__) )

/// @brief Log a warning message only once
#define RAYBENCH_LOG_WARNING_ONCE(message, ...) \
    RAYBENCH_LOG_ONCE (RAYBENCH_LOG_WARNING (message, ##__VA_ARGS__))

/// @brief Log an error message only once
#define RAYBENCH_LOG_ERROR_ONCE(message, ...) \
    RAYBENCH_LOG_ONCE (RAYBENCH_LOG_ERROR (message, ##__VA_ARGS__))

/// @brief Log a critical message only once
#define RAYBENCH_LOG_CRITICAL_ONCE(message, ...) \
    RAYBENCH_LOG_ONCE (RAYBENCH_LOG_CRITICAL (message, ##__VA_ARGS__))

/// @brief Write a message to the console only once
#define RAYBENCH_WRITE_CONSOLE_ONCE(message, ...) \
    RAYBENCH_LOG_ONCE (RAYBENCH_WRITE_CONSOLE (message, ##__VA_ARGS__))

// ----------------------------------------------------------------------------

#ifdef NDEBUG

/// @brief Assert a condition and log a critical message if it fails
#define RAYBENCH_ASSERT(condition, message, ...) \
    ((void)0);

#else

/// @brief Assert a condition and log a critical message if it fails
#define RAYBENCH_ASSERT(condition, message, ...) \
    { \
        if (!(condition)) \
        { \
            RAYBENCH_LOG_CRITICAL ("Assertion failed: " message, ##__VA_ARGS__); \
        } \
    }

#endif

#endif

// ----------------------------------------------------------------------------