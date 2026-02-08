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
    std::string_view name = path;
    if (auto pos = name.find_last_of ("/\\"); pos != std::string_view::npos)
    {
        name = name.substr (pos + 1);
    }
    if (auto pos = name.find (".exe"); pos != std::string_view::npos)
    {
        name = name.substr (0, pos);
    }

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
        return 1;
    }
 
    raybench::util::Log::Release ();

    return 0;
}

// ----------------------------------------------------------------------------