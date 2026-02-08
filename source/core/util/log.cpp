// ----------------------------------------------------------------------------

/// @brief Logging utility

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

export module RayBench.Util:Log;

import std;

namespace raybench::util
{

/// @brief Logging utility
export class Log
{
public:

    /// @brief Severity levels for logging
    enum class Severity : std::uint32_t
    {
        debug = 0,
        info,
        warning,
        error,
        critical,
        always = 0xFFFFFFFF
    };

    // ------------------------------------------------------------------------

    /// @brief Logging settings
    struct Settings
    {
        ///< Minimum severity level to log
        Severity min_severity {Severity::info};
        ///< Output detailed log information
        bool output_detailed_log_info {false};
        ///< Output timestamps
        bool output_timestamps {false};
        ///< Flush to file or console after every log write
        bool flush_after_write {false};
        ///< Force a break when an error occurs
        bool break_on_error {false};

        ///< Write log messages to a file
        bool write_to_file {false};
        ///< Append to previous log file instead of overwriting
        bool append_to_file {false};
        ///< Leave the log file open between writes for performance
        bool leave_file_open {true};
        ///< Name of the log file including path
        std::string file_name;

        ///< Write log messages to the console
        bool output_to_console {true};
        ///< Output error messages to stderr instead of stdout
        bool output_errors_to_stderr {true};
        ///< Output messages to OutputDebugString
        bool output_to_debug_string {false};
    };

    // ------------------------------------------------------------------------

    /// @brief Initialize logging with settings
    /// 
    /// @param settings Logger settings
    static void Initialize (const Settings& settings = {})
    {
        std::scoped_lock lock (log_mutex_);

        settings_ = settings;

        if (!settings_.file_name.empty ())
        {
            auto file_modifiers =
                std::ios::out | (settings_.append_to_file ? std::ios::app : std::ios::trunc);
            log_file_.open (settings_.file_name.data (), file_modifiers);
            if (log_file_.is_open ())
            {
                settings_.write_to_file = true;

                if (!settings_.leave_file_open)
                {
                    log_file_.close ();
                }
            }
        }
    }

    // ------------------------------------------------------------------------

    /// @brief Release logging resources
    static void Release ()
    {
        std::scoped_lock lock (log_mutex_);

        if (settings_.write_to_file && settings_.leave_file_open)
        {
            log_file_.close ();
            settings_.write_to_file = false;
        }
    }

    // ------------------------------------------------------------------------

    /// @brief Log a message
    /// 
    /// @tparam ...Args Argument types
    /// @param severity Severity level
    /// @param location Source location
    /// @param fmt Format string
    /// @param ...args Arguments
    template<typename... Args>
    static void LogMessage (Severity severity,
                            const std::source_location& location,
                            std::format_string<Args...> fmt,
                            Args&&... args)
    {
        LogMessageImpl (severity, location,
                        std::format (fmt, std::forward<Args> (args)...));
    }

    // ------------------------------------------------------------------------

    /// @brief Check if a message with the given severity will be output
    /// 
    /// @param severity Severity level
    [[nodiscard]] static bool WillOutputMessage (Severity severity)
    {
        if (severity < Severity::always)
        {
            Severity min_severity = settings_.min_severity;

            if (settings_.output_errors_to_stderr && min_severity > Severity::error)
            {
                min_severity = Severity::error;
            }

            return severity >= min_severity;
        }
        return true;
    }

    // ------------------------------------------------------------------------

    /// @brief Convert severity level to string
    /// 
    /// @param severity Severity level
    /// @return String representation of severity level
    [[nodiscard]] static constexpr std::string_view SeverityToString (Severity severity) noexcept
    {
        switch (severity)
        {
            case Severity::debug:
                return "debug";
            case Severity::info:
                return "info";
            case Severity::warning:
                return "warning";
            case Severity::error:
                return "error";
            case Severity::critical:
                return "critical";
            case Severity::always:
                return "";
            default:
                return "unknown";
        }
    }

    // ------------------------------------------------------------------------

    /// @brief Convert string to severity level
    /// 
    /// @param str String representation of severity level
    /// @return Severity level or error message
    [[nodiscard]] static std::expected<Severity, std::string_view>
        StringToSeverity (std::string_view str) noexcept
    {
        auto IsSame = [] (std::string_view a, std::string_view b)
            {
                return std::ranges::equal (a, b,
                                           [] (char c1, char c2)
                                           {
                                               return std::tolower (c1) == std::tolower (c2);
                                           });
            };

        if (IsSame (str, "debug"))
            return Severity::debug;
        if (IsSame (str, "info"))
            return Severity::info;
        if (IsSame (str, "warning"))
            return Severity::warning;
        if (IsSame (str, "error"))
            return Severity::error;
        if (IsSame (str, "critical"))
            return Severity::critical;
        if (IsSame (str, "always"))
            return Severity::always;

        return std::unexpected ("Unknown severity level");
    }

    // ------------------------------------------------------------------------

    /// @brief Get the minimum severity level to log
    /// 
    /// @return Minimum severity level
    [[nodiscard]] static Severity GetSeverity () noexcept
    {
        return settings_.min_severity;
    }

private:

    // ------------------------------------------------------------------------

    /// @brief Log a message implementation
    /// @param severity Severity level
    /// @param location Source location
    /// @param message Log message
    static void LogMessageImpl (Severity severity,
                                const std::source_location& location,
                                std::string_view message)
    {
        constexpr std::string_view process_tag = "ray-bench";

        const bool output_to_stderr = (severity >= Severity::error) &&
            settings_.output_to_console &&
            settings_.output_errors_to_stderr;

        std::string prefix;
        if (severity != Severity::always)
        {
            prefix = std::format ("{} [", process_tag);

            if (settings_.output_timestamps)
            {
                auto now = std::chrono::system_clock::now ();
                prefix += std::format ("{:%H:%M:%S}|", now);
            }

            prefix += std::format ("{}]", SeverityToString (severity));

            if (settings_.output_detailed_log_info)
            {
                std::string_view full_path = location.file_name ();
                std::string_view relative_path = full_path;

                if (auto pos = full_path.find ("source\\");
                    pos != std::string_view::npos)
                {
                    relative_path = full_path.substr (pos + 7);
                }

                prefix += std::format (" [{}({},{}): {}]",
                                       relative_path,
                                       location.line (),
                                       location.column (),
                                       location.function_name ());
            }

            prefix += " ";
        }

        std::string output_message = std::format ("{}{}", prefix, message);

        std::scoped_lock lock (log_mutex_);

        // Output to console
        if (settings_.output_to_console)
        {
            if (settings_.output_to_debug_string)
            {
                OutputDebugStringA (output_message.c_str ());
            }
            else
            {
                if (output_to_stderr)
                {
                    std::println (std::cerr, "{}", output_message);
                }
                else
                {
                    std::println ("{}", output_message);
                }

                if (settings_.flush_after_write)
                {
                    if (output_to_stderr)
                    {
                        std::cerr.flush ();
                    }
                    else
                    {
                        std::cout.flush ();
                    }
                }
            }
        }

        // Output to file
        if (settings_.write_to_file && severity >= settings_.min_severity)
        {
            bool opened_file = false;

            if (!settings_.leave_file_open)
            {
                log_file_.open (settings_.file_name.data (), std::ios::app);
                opened_file = true;
            }

            if (log_file_.is_open ())
            {
                std::println (log_file_, "{}", output_message);

                if (settings_.flush_after_write || settings_.leave_file_open)
                {
                    log_file_.flush ();
                }

                if (opened_file && !settings_.leave_file_open)
                {
                    log_file_.close ();
                }
            }
        }

        if (severity >= Severity::error && severity < Severity::always && settings_.break_on_error)
        {
            __debugbreak ();
        }
    }

    // ------------------------------------------------------------------------

    ///< Logger settings
    inline static Settings settings_;
    ///< Log file
    inline static std::ofstream log_file_;
    ///< Mutex for thread-safe logging
    inline static std::mutex log_mutex_;
};

}

// ----------------------------------------------------------------------------