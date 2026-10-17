#include "local_client_launcher.hpp"

#include <Windows.h>
#include <shellapi.h>

#include <nostale_hk/contracts/verified_client.hpp>
#include <nostale_hk/platform/sha256.hpp>
#include <nostale_hk/platform/unique_handle.hpp>

#include <array>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "launch_options.hpp"
#include "xigncode_startup.hpp"

namespace nostale::hk::launcher {
namespace {

// Never leave an unresumed client behind, including on exceptions or partial writes.
class suspended_client final {
public:
    ~suspended_client() {
        if (process && !resumed) (void)discard();
    }
    [[nodiscard]] DWORD discard() noexcept {
        if (!::TerminateProcess(process.get(), ERROR_PROCESS_ABORTED)) return ::GetLastError();
        const DWORD waited = ::WaitForSingleObject(process.get(), 5'000U);
        if (waited != WAIT_OBJECT_0) return waited == WAIT_TIMEOUT ? ERROR_TIMEOUT : ::GetLastError();
        thread.reset();
        process.reset();
        return ERROR_SUCCESS;
    }
    platform::unique_handle process;
    platform::unique_handle thread;
    bool resumed = false;
};

DWORD verify_client_file(HANDLE file) noexcept {
    LARGE_INTEGER size{};
    if (!::GetFileSizeEx(file, &size)) return ::GetLastError();
    if (size.QuadPart != static_cast<LONGLONG>(contracts::disk_size)) return ERROR_REVISION_MISMATCH;
    std::array<std::byte, 32> hash{};
    if (!platform::sha256_file(file, hash)) return ERROR_CRC;
    return contracts::profile_for_hash(hash) != nullptr ? ERROR_SUCCESS : ERROR_REVISION_MISMATCH;
}

bool read_local_endpoint(const std::wstring& ini) noexcept {
    std::array<wchar_t, 64> ip{}, port{};
    const DWORD ip_length = ::GetPrivateProfileStringW(L"NosTale_Network", L"IP", L"",
        ip.data(), static_cast<DWORD>(ip.size()), ini.c_str());
    const DWORD port_length = ::GetPrivateProfileStringW(L"NosTale_Network", L"Port", L"",
        port.data(), static_cast<DWORD>(port.size()), ini.c_str());
    if (ip_length == ip.size() - 1 || port_length == port.size() - 1) return false;
    return is_local_login_endpoint({ip.data(), ip_length}, {port.data(), port_length});
}

bool is_elevated() noexcept {
    HANDLE raw_token = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &raw_token)) return true;
    platform::unique_handle token(raw_token);
    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    // If the token cannot be queried, refuse an automatic elevation retry.
    return !::GetTokenInformation(token.get(), TokenElevation, &elevation, sizeof(elevation), &size) ||
        elevation.TokenIsElevated != 0;
}

DWORD request_elevated_launch(const launch_options& options) {
    std::array<wchar_t, 32768> self{};
    const DWORD length = ::GetModuleFileNameW(nullptr, self.data(), static_cast<DWORD>(self.size()));
    if (length == 0 || length >= self.size()) return ERROR_INSUFFICIENT_BUFFER;
    const auto executable = std::filesystem::canonical(options.executable);
    std::wstring arguments = L"--launch-client \"" + executable.native() + L"\"";
    if (options.disable_xigncode3) arguments += L" --disable-xigncode3";
    SHELLEXECUTEINFOW request{};
    request.cbSize = sizeof(request);
    request.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
    request.lpVerb = L"runas";
    request.lpFile = self.data();
    request.lpParameters = arguments.c_str();
    request.nShow = SW_HIDE;
    if (!::ShellExecuteExW(&request)) return ::GetLastError();
    platform::unique_handle elevated_process(request.hProcess);
    return ERROR_SUCCESS;
}

DWORD launch(const launch_options& options, const wchar_t*& stage, DWORD& child_id) {
    stage = L"client path";
    const std::filesystem::path requested(options.executable);
    if (!requested.is_absolute() || !contracts::is_supported_executable_name(requested.filename().native()))
        return ERROR_INVALID_NAME;
    const auto executable = std::filesystem::canonical(requested);
    if (!contracts::is_supported_executable_name(executable.filename().native())) return ERROR_INVALID_NAME;
    const auto directory = executable.parent_path();
    const auto ini = (directory / L"Config.ini").native();

    // Deny concurrent replacement/writes while verifying and preparing the child.
    stage = L"client fingerprint";
    platform::unique_handle file(::CreateFileW(executable.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!file) return ::GetLastError();
    DWORD error = verify_client_file(file.get());
    if (error != ERROR_SUCCESS) return error;

    stage = L"loopback Config.ini (127.0.0.1 and an explicit valid login port required)";
    platform::unique_handle config(::CreateFileW(ini.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!config) return ::GetLastError();
    if (!read_local_endpoint(ini)) return ERROR_BAD_CONFIGURATION;

    std::wstring command = L"\"" + executable.native() + L"\" EntwellNostaleClientLoadFromIni";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    suspended_client child;
    stage = L"create suspended client";
    if (!::CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE,
            CREATE_SUSPENDED, nullptr, directory.c_str(), &startup, &process)) return ::GetLastError();
    child.process.reset(process.hProcess);
    child.thread.reset(process.hThread);
    child_id = process.dwProcessId;

    stage = L"verified process-local Xigncode startup";
    error = configure_xigncode_startup(child.process.get(), options.disable_xigncode3);
    if (error != ERROR_SUCCESS) return error;
    if (options.validate_only) {
        stage = L"discard suspended validation client";
        error = child.discard();
        if (error != ERROR_SUCCESS) return error;
        stage = L"validated; client never resumed";
        return ERROR_SUCCESS;
    }

    stage = L"resume client";
    const DWORD previous_suspend_count = ::ResumeThread(child.thread.get());
    if (previous_suspend_count == static_cast<DWORD>(-1)) return ::GetLastError();
    if (previous_suspend_count != 1) return ERROR_INVALID_STATE;
    child.resumed = true;
    stage = L"started";
    return ERROR_SUCCESS;
}

} // namespace

int run_local_client_launcher(int argc, wchar_t** argv) {
    std::vector<std::wstring_view> arguments;
    for (int index = 1; index < argc; ++index) arguments.emplace_back(argv[index]);
    const auto options = parse_launch_options(arguments);
    if (!options) {
        std::fputws(L"Usage: --launch-client <absolute NostaleX.exe/.dat path> "
            L"[--disable-xigncode3] [--validate-only]\n", stderr);
        return ERROR_INVALID_PARAMETER;
    }
    const wchar_t* stage = L"path resolution";
    DWORD child_id = 0;
    DWORD error = ERROR_SUCCESS;
    try {
        error = launch(*options, stage, child_id);
        if (error == ERROR_ELEVATION_REQUIRED && !options->validate_only && !is_elevated()) {
            // The pinned client manifest requires Administrator. Use the normal UAC
            // consent flow; never change its manifest or system compatibility settings.
            stage = L"Windows administrator consent";
            error = request_elevated_launch(*options);
            if (error == ERROR_SUCCESS) {
                std::puts("CLIENT_LAUNCH_DELEGATED=UAC");
                return ERROR_SUCCESS;
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        error = ERROR_PATH_NOT_FOUND;
    }
    if (error != ERROR_SUCCESS) {
        const std::wstring message = std::wstring(L"Local client launch failed at ") + stage +
            L". Windows error: " + std::to_wstring(error) + L". Startup was aborted.";
        std::fwprintf(stderr, L"%ls\n", message.c_str());
        if (!options->validate_only) ::MessageBoxW(nullptr, message.c_str(), L"NosTale Xigncode3 launcher", MB_OK | MB_ICONERROR);
        return static_cast<int>(error);
    }
    std::wprintf(L"%ls PID=%lu XIGNCODE3=%ls (%ls)\n",
        options->validate_only ? L"CLIENT_LAUNCH_CHECK=PASS" : L"CLIENT_STARTED",
        child_id, options->disable_xigncode3 ? L"disabled for this process" : L"unchanged", stage);
    return ERROR_SUCCESS;
}

} // namespace nostale::hk::launcher
