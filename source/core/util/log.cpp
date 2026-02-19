// ----------------------------------------------------------------------------

/// @brief Logging utility

module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "log.h"

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
        trace = 0,
        debug,
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
        ///< Listen via a named pipe for inter process communication
        bool listen_to_named_pipe {false};

        // --------------------------------------------------------------------

        /// @brief Serialize the settings to a string
        /// 
        /// @return Serialized settings string
        std::string Serialize ()
        {
            return std::format ("min_severity={}:"
                                "output_detailed_log_info={}:"
                                "output_timestamps={}:"
                                "flush_after_write={}:"
                                "break_on_error={}:"
                                "write_to_file={}:"
                                "append_to_file={}:"
                                "leave_file_open={}:"
                                "file_name={}:"
                                "output_to_console={}:"
                                "output_errors_to_stderr={}:"
                                "output_to_debug_string={}:"
                                "listen_to_named_pipe={}",
                                SeverityToString (min_severity),
                                output_detailed_log_info,
                                output_timestamps,
                                flush_after_write,
                                break_on_error,
                                write_to_file,
                                append_to_file,
                                leave_file_open,
                                file_name,
                                output_to_console,
                                output_errors_to_stderr,
                                output_to_debug_string,
                                listen_to_named_pipe);
        }

        /// @brief Deserialize settings from a string
        ///
        /// @param str Serialized settings string view
        bool Deserialize (std::string_view str)
        {
            auto key_value_pairs = str | std::views::split (':');
            for (auto&& pair_range : key_value_pairs)
            {
                auto pair = std::string_view (pair_range);
                auto delim_pos = pair.find ('=');

                if (delim_pos == std::string_view::npos)
                {
                    continue;
                }

                std::string_view key = pair.substr (0, delim_pos);
                std::string_view value = pair.substr (delim_pos + 1);

                auto ToBool = [] (std::string_view str)
                    {
                        return str == "1" || str == "true" || str == "True" || str == "TRUE";
                    };

                if (key == "min_severity")
                {
                    if (auto severity_result = StringToSeverity (value); severity_result.has_value ())
                    {
                        min_severity = severity_result.value ();
                    }
                }
                else if (key == "output_detailed_log_info")
                    output_detailed_log_info = ToBool (value);
                else if (key == "output_timestamps")
                    output_timestamps = ToBool (value);
                else if (key == "flush_after_write")
                    flush_after_write = ToBool (value);
                else if (key == "break_on_error")
                    break_on_error = ToBool (value);
                else if (key == "write_to_file")
                    write_to_file = ToBool (value);
                else if (key == "append_to_file")
                    append_to_file = ToBool (value);
                else if (key == "leave_file_open")
                    leave_file_open = ToBool (value);
                else if (key == "file_name")
                    file_name = value;
                else if (key == "output_to_console")
                    output_to_console = ToBool (value);
                else if (key == "output_errors_to_stderr")
                    output_errors_to_stderr = ToBool (value);
                else if (key == "output_to_debug_string")
                    output_to_debug_string = ToBool (value);
                else if (key == "listen_to_named_pipe")
                    listen_to_named_pipe = ToBool (value);
            }

            return true;
        }
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

        if (settings.listen_to_named_pipe)
        {
            log_thread_handle_ = CreateThread (nullptr, 0, LogThreadProc, nullptr, 0, nullptr);
        }

        initialized_ = true;
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

        if (settings_.listen_to_named_pipe && log_thread_handle_ != INVALID_HANDLE_VALUE)
        {
            WaitForSingleObject (log_thread_handle_, INFINITE);
            CloseHandle (log_thread_handle_);
            log_thread_handle_ = INVALID_HANDLE_VALUE;
        }

        initialized_ = false;
    }

    // ------------------------------------------------------------------------

    /// @brief Connect to the named pipe for inter process communication
    /// 
    /// Call this function in a client process to connect to the named pipe
    /// created by the log thread of the main process.
    static void ClientConnect ()
    {
        static std::once_flag connection_flag;
        std::call_once (connection_flag, [] ()
                        {
                            if (WaitNamedPipeA (named_pipe_name_, NMPWAIT_WAIT_FOREVER))
                            {
                                named_pipe_handle_ = CreateFileA (named_pipe_name_,
                                                                  GENERIC_WRITE,
                                                                  0, nullptr, OPEN_EXISTING, 0, nullptr);
                            }
                        });
    }

    // ------------------------------------------------------------------------

    /// @brief Disconnect from the named pipe
    /// 
    /// Call this function in a client process to disconnect from the named
    /// pipe created by the log thread of the main process.
    static void ClientDisconnect ()
    {
        if (named_pipe_handle_ != INVALID_HANDLE_VALUE)
        {
            DisconnectNamedPipe (named_pipe_handle_);
            CloseHandle (named_pipe_handle_);
            named_pipe_handle_ = INVALID_HANDLE_VALUE;
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
        const std::string message = std::format (fmt, std::forward<Args> (args)...);
        const std::string formatted_message = LogFormat (severity, location, message);
        if (initialized_ == true)
        {
            LogMessageImpl (severity, formatted_message);
        }
        else
        {
            DWORD bytes_written = 0;
            WriteFile (named_pipe_handle_, message.data (),
                       static_cast<DWORD> (message.size ()), &bytes_written, nullptr);
        }
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
            case Severity::trace:
                return "trace";
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

        if (IsSame (str, "trace"))
            return Severity::trace;
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

    /// @brief Get the reference to logger settings
    /// 
    /// @return Logger settings reference
    [[nodiscard]] static Settings& GetSettings () noexcept
    {
        return settings_;
    }

private:

    // ------------------------------------------------------------------------

    static [[nodiscard]] std::string LogFormat (Severity severity,
                                                const std::source_location& location,
                                                std::string_view message)
    {
        constexpr std::string_view process_tag = "ray-bench";

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

        return std::format ("{}{}", prefix, message);
    }

    // ------------------------------------------------------------------------

    /// @brief Log a message implementation
    /// 
    /// @param severity Severity level
    /// @param output_message Formatted log message
    static void LogMessageImpl (Severity severity, std::string_view output_message)
    {
        const bool output_to_stderr = (severity >= Severity::error) &&
            settings_.output_to_console &&
            settings_.output_errors_to_stderr;

        std::scoped_lock lock (log_mutex_);

        // Output to console
        if (settings_.output_to_console)
        {
            if (settings_.output_to_debug_string)
            {
                OutputDebugStringA (output_message.data ());
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

    /// @brief Thread procedure for listening to the named pipe
    static DWORD WINAPI LogThreadProc (LPVOID /*lpParam*/)
    {
        named_pipe_handle_ = CreateNamedPipeA (named_pipe_name_,
                                               PIPE_ACCESS_INBOUND,
                                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                               PIPE_UNLIMITED_INSTANCES,
                                               named_pipe_buffer_size_,
                                               named_pipe_buffer_size_,
                                               0,
                                               nullptr);
        if (named_pipe_handle_ == INVALID_HANDLE_VALUE)
        {
            RAYBENCH_LOG_CRITICAL ("Failed to create named pipe: {}", GetLastError ());
            return 1;
        }

        RAYBENCH_LOG_DEBUG ("Named pipe created successfully, waiting for connection...");

        bool connected = ConnectNamedPipe (named_pipe_handle_, nullptr) ||
            GetLastError () == ERROR_PIPE_CONNECTED;
        if (connected == false)
        {
            RAYBENCH_LOG_CRITICAL ("Failed to connect to named pipe: {}", GetLastError ());
            CloseHandle (named_pipe_handle_);
            return 1;
        }

        RAYBENCH_LOG_INFO ("Named pipe connected successfully");

        std::array<CHAR, named_pipe_buffer_size_> buffer {};
        DWORD bytes_read = 0;
        while (true)
        {
            bool result = ReadFile (named_pipe_handle_, buffer.data (),
                                    static_cast<DWORD> (buffer.size ()), &bytes_read, nullptr);
            if (result == true && bytes_read > 0)
            {
                std::string_view message (buffer.data (), bytes_read);
                LogMessageImpl (settings_.min_severity, message);
            }
            else if (result == false)
            {
                DWORD error = GetLastError ();
                if (error == ERROR_BROKEN_PIPE)
                {
                    RAYBENCH_LOG_INFO ("Named pipe client disconnected");
                }
                else
                {
                    RAYBENCH_LOG_ERROR ("Failed to read from named pipe: {}", error);
                }
                break;
            }
        }

        DisconnectNamedPipe (named_pipe_handle_);
        CloseHandle (named_pipe_handle_);
        named_pipe_handle_ = INVALID_HANDLE_VALUE;
        return 0;
    }

    // ------------------------------------------------------------------------

    ///< Logger settings
    inline static Settings settings_;
    ///< Flag to indicate if logging has been initialized
    inline static bool initialized_ = false;
    ///< Log file
    inline static std::ofstream log_file_;
    ///< Mutex for thread-safe logging
    inline static std::mutex log_mutex_;

    ///< Name of the named pipe for inter process communication
    static constexpr LPCSTR named_pipe_name_ = R"(\\.\pipe\ray-bench-log)";
    ///< Buffer size for reading from the named pipe
    static constexpr DWORD named_pipe_buffer_size_ = 4096;
    ///< Handle for the named pipe
    inline static HANDLE named_pipe_handle_ = INVALID_HANDLE_VALUE;
    ///< Handle for the log thread that listens to the named pipe
    inline static HANDLE log_thread_handle_ = INVALID_HANDLE_VALUE;
};

}

// ----------------------------------------------------------------------------