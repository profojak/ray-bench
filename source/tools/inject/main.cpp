import RayBench.Util;

int main() {
    raybench::util::Log::Initialize ();
    raybench::util::Log::LogMessage (raybench::util::Log::Severity::info, "Hello!");
    raybench::util::Log::Release ();

    raybench::util::Log::Settings settings {
        .min_severity = raybench::util::Log::Severity::debug,
        .output_detailed_log_info = true,
        .output_timestamps = true,
        .break_on_error = true,
        .file_name = "log.txt",
    };
    raybench::util::Log::Initialize (settings);
    raybench::util::Log::LogMessage (raybench::util::Log::Severity::error, "Detailed error with breakpoint!");
    raybench::util::Log::Release ();

    return 0;
}
