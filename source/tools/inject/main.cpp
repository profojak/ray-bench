import RayBench.Util;

int main(int argc, const char** argv) {
    int return_code = 0;

    raybench::util::Log::Initialize ();
    raybench::util::Arg args (argc, argv, "", "");

    raybench::util::Log::Release ();

    return return_code;
}
