#include "launch_options.hpp"

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

} // namespace nostale::hk::launcher
