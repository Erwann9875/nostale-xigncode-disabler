#pragma once

#include <array>
#include <string_view>

#include <nostale_hk/contracts/client_profiles.hpp>

namespace nostale::hk::contracts {

[[nodiscard]] constexpr bool is_supported_executable_name(std::wstring_view name) noexcept {
    constexpr auto names = std::array{executable_name, alternate_executable_name};
    const auto lower = [](wchar_t value) constexpr {
        return value >= L'A' && value <= L'Z' ? static_cast<wchar_t>(value + (L'a' - L'A')) : value;
    };
    for (const auto expected : names) {
        if (name.size() != expected.size()) continue;
        bool matches = true;
        for (std::size_t index = 0; index < name.size(); ++index) {
            if (lower(name[index]) != lower(expected[index])) { matches = false; break; }
        }
        if (matches) return true;
    }
    return false;
}

} // namespace nostale::hk::contracts
