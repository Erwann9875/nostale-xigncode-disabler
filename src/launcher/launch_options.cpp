#include "launch_options.hpp"

#include <cstdint>

namespace nostale::hk::launcher {

std::optional<launch_options> parse_launch_options(std::span<const std::wstring_view> arguments) {
    if (arguments.size() < 2 || arguments[0] != L"--launch-client" || arguments[1].empty() ||
        arguments[1].starts_with(L"--")) return std::nullopt;
    launch_options result{std::wstring(arguments[1])};
    for (const auto argument : arguments.subspan(2)) {
        if (argument == L"--disable-xigncode3" && !result.disable_xigncode3) {
            result.disable_xigncode3 = true;
        } else if (argument == L"--validate-only" && !result.validate_only) {
            result.validate_only = true;
        } else {
            return std::nullopt;
        }
    }
    return result;
}

bool is_local_login_endpoint(std::wstring_view ip, std::wstring_view port) noexcept {
    // No hostname resolution, implicit port, public endpoint or compiled-default fallback.
    if (ip != L"127.0.0.1" || port.empty() || port.size() > 5) return false;
    std::uint32_t value = 0;
    for (const wchar_t digit : port) {
        if (digit < L'0' || digit > L'9') return false;
        value = value * 10U + static_cast<std::uint32_t>(digit - L'0');
    }
    return value >= 1U && value <= 65535U;
}

} // namespace nostale::hk::launcher
