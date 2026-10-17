#include "local_client_launcher.hpp"

#include <Windows.h>
#include <nostale_hk/contracts/verified_client.hpp>

#include <array>
#include <filesystem>
#include <string>
#include <system_error>

namespace nostale::hk::launcher {

int run_adjacent_client_launcher() {
    std::array<wchar_t, 32768> self{};
    const DWORD length = ::GetModuleFileNameW(nullptr, self.data(), static_cast<DWORD>(self.size()));
    if (length == 0 || length >= self.size()) return ERROR_INSUFFICIENT_BUFFER;
    const auto directory = std::filesystem::path(self.data()).parent_path();
    // The original launcher uses .dat; an installation may use .exe instead.
    // Show the exact selected path before launching, including when both exist.
    auto client = directory / contracts::executable_name;
    std::error_code error;
    if (!std::filesystem::is_regular_file(client, error)) client = directory / contracts::alternate_executable_name;
    if (!std::filesystem::is_regular_file(client, error)) {
        ::MessageBoxW(nullptr,
            L"Place this EXE in your game folder beside NostaleX.dat or NostaleX.exe first. "
            L"No additional launcher DLL is required.", L"NosTale Xigncode3 Disabler", MB_OK | MB_ICONERROR);
        return ERROR_FILE_NOT_FOUND;
    }
    const std::wstring message = L"Launch this client?\n\n" + client.native() +
        L"\n\nYES: temporarily disable Xigncode3 for this launch."
        L"\nNO: leave Xigncode3 unchanged.\nCANCEL: do not launch."
        L"\n\nNo client or Xigncode files are changed or deleted."
        L"\nConfig.ini must specify your LOGIN server IP/hostname and port."
        L"\nUse only where you have permission.";
    const int choice = ::MessageBoxW(nullptr, message.c_str(), L"NosTale Xigncode3 Disabler",
        MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON3);
    if (choice != IDYES && choice != IDNO) return ERROR_CANCELLED;
    std::wstring launch_flag = L"--launch-client";
    std::wstring client_path = client.native();
    std::wstring disable_flag = L"--disable-xigncode3";
    std::array<wchar_t*, 4> arguments{self.data(), launch_flag.data(), client_path.data(), disable_flag.data()};
    return run_local_client_launcher(choice == IDYES ? 4 : 3, arguments.data());
}

} // namespace nostale::hk::launcher
