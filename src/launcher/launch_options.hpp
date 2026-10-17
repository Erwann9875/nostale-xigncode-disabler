#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace nostale::hk::launcher {

struct launch_options final {
    std::wstring executable;
    bool disable_xigncode3 = false;
    bool validate_only = false;
};

[[nodiscard]] std::optional<launch_options> parse_launch_options(
    std::span<const std::wstring_view> arguments);

} // namespace nostale::hk::launcher
