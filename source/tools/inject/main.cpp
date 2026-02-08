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
    raybench::util::Log::Initialize ();
    raybench::util::Arg args (argc, argv, options, "");

    if (args.IsInvalid () || args.GetPositionalArguments ().size () == 0 ||
        args.IsOptionSet ("--help") || args.IsOptionSet ("-h"))
    {
        PrintHelp (argv[0]);
        raybench::util::Log::Release ();
        return 1;
    }

    auto process_paths = raybench::util::GetProcessPaths (args.GetPositionalArguments ().front ());
    if (process_paths.has_value() == false)
    {
        RAYBENCH_LOG_ERROR ("No process found matching the specified target: {}",
                            args.GetPositionalArguments ().front ());
        raybench::util::Log::Release ();
        return 1;
    }

    raybench::util::LaunchAndInject (process_paths.value(), 0, nullptr);

    raybench::util::Log::Release ();
    return 0;
}

// ----------------------------------------------------------------------------