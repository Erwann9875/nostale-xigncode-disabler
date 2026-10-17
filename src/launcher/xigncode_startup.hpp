#pragma once

#include <Windows.h>

#include <array>
#include <cstdint>

namespace nostale::hk::launcher {

// Reviewed call site in the exact client fingerprint from verified_client.hpp.
// The replacement is process-local; it is never written to the executable file.
inline constexpr std::uint32_t xigncode_startup_rva = 0x0010116FU;
inline constexpr std::array<unsigned char, 9> xigncode_startup_original{
    0xE8, 0x5C, 0x96, 0xFD, 0xFF, 0x84, 0xC0, 0x75, 0x4C};
inline constexpr std::array<unsigned char, 5> xigncode_startup_disabled{
    0xB8, 0x01, 0x00, 0x00, 0x00};

// Only called for a newly created, still-suspended, verified client process.
[[nodiscard]] DWORD configure_xigncode_startup(HANDLE process, bool disable) noexcept;

} // namespace nostale::hk::launcher
