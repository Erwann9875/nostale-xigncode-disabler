#include <Windows.h>

#include <cstdio>
#include <exception>
#include <string_view>

#include "launcher/local_client_launcher.hpp"

int wmain(int argc, wchar_t** argv) {
    try {
        if (argc == 1) return nostale::hk::launcher::run_adjacent_client_launcher();
        if (argc == 2 && (std::wstring_view(argv[1]) == L"--help" || std::wstring_view(argv[1]) == L"-h")) {
            std::puts("NosTale Xigncode3 launcher\n"
                "Usage: Nostale-Xigncode-Disabler.exe --launch-client <absolute NostaleX.dat/.exe path> "
                "--disable-xigncode3 [--validate-only]\n"
                "Without arguments: place beside the supported client and double-click.\n"
                "Only supported client fingerprints are accepted; any configured IP/hostname is allowed.\n"
                "Config.ini must specify a login endpoint and valid port. Use only with permission.\n"
                "No game files, services or drivers are changed.");
            return ERROR_SUCCESS;
        }
        return nostale::hk::launcher::run_local_client_launcher(argc, argv);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Launch failed: %s\n", error.what());
        return ERROR_UNHANDLED_EXCEPTION;
    } catch (...) {
        std::fputs("Launch failed: unexpected error.\n", stderr);
        return ERROR_UNHANDLED_EXCEPTION;
    }
}
