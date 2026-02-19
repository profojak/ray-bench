// ----------------------------------------------------------------------------

/// @brief Entry point for injecting dynamic-link libraries into target
///        application

#include "util/log.h"

import std;
import RayBench.Util;

///< Command line options
constexpr const char* options = "-h|--help";

// ----------------------------------------------------------------------------

/// @brief Print help message for the inject tool
/// 
/// @param path Path to the inject tool executable
static void PrintHelp (const std::string_view path)
{
    const std::string name = std::filesystem::path (path).stem ().string ();

    RAYBENCH_WRITE_CONSOLE ("{}: inject tool\n\n"
                            "Usage:\n"
                            "  {} [-h|--help] <target>\n\n"
                            "Arguments:\n"
                            "  <target>            Target application to inject into\n\n"
                            "Options:\n"
                            "  -h, --help          Show this help message and exit\n", name, name);
}

// ----------------------------------------------------------------------------

/// @brief Main entry point for the inject tool
/// 
/// @param argc Command line argument count
/// @param argv Command line argument values
/// @return Return code
int main (int argc, const char** argv)
{
    raybench::util::Log::Settings log_settings {
        .min_severity = raybench::util::Log::Severity::trace,
        .listen_to_named_pipe = true,
    };

    raybench::util::Log::Initialize (log_settings);
    raybench::util::Arg args (argc, argv, options, "");

    RAYBENCH_LOG_INFO ("Started inject tool");
    RAYBENCH_LOG_TRACE ("Parsing command line arguments...");

    if (args.IsInvalid () || args.GetPositionalArguments ().size () == 0 ||
        args.IsOptionSet ("--help") || args.IsOptionSet ("-h"))
    {
        PrintHelp (argv[0]);
        raybench::util::Log::Release ();
        return 1;
    }

    // Get the process paths matching the specified target
    auto create_process_info = raybench::util::GetCreateProcessInfo (args.GetPositionalArguments ().front ());
    if (create_process_info.has_value () == false)
    {
        RAYBENCH_LOG_CRITICAL ("Failed to get process information for target: {}",
                               args.GetPositionalArguments ().front ());
        raybench::util::Log::Release ();
        return 1;
    }

    RAYBENCH_LOG_DEBUG ("Process information obtained successfully for target: {}",
                        args.GetPositionalArguments ().front ());

    // Ensure the required dynamic-link library exists before attempting injection
    std::filesystem::path dll_path = std::filesystem::path (argv[0]).parent_path () / "ray-bench-winapi.dll";
    if (std::filesystem::exists (dll_path) == false)
    {
        RAYBENCH_LOG_CRITICAL ("Required dynamic-link library not found for injection: {}",
                               dll_path.string ());
        return 1;
    }

    RAYBENCH_LOG_DEBUG ("Required dynamic-link library found for injection: {}",
                        dll_path.string ());
    RAYBENCH_LOG_TRACE ("Setting environment variable for required dynamic-link library path...");

    raybench::util::EnvVar::Set (raybench::util::EnvVar::winapi_dll_path, dll_path.string ());

    RAYBENCH_LOG_TRACE ("Setting environment variable for logging settings...");

    raybench::util::EnvVar::Set (raybench::util::EnvVar::log_settings,
                                 raybench::util::Log::GetSettings ().Serialize ());

    raybench::util::LaunchInject (create_process_info.value (), dll_path.string ().c_str ());

    RAYBENCH_LOG_INFO ("Launched target with injected dynamic-link library");

    raybench::util::Log::Release ();
    return 0;
}

// ----------------------------------------------------------------------------