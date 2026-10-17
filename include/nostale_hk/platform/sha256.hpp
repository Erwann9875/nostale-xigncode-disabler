#pragma once
#include <Windows.h>
#include <array>
#include <cstddef>
namespace nostale::hk::platform {
// Hashes from the file's current cursor. Caller owns the file handle.
[[nodiscard]] bool sha256_file(HANDLE file, std::array<std::byte, 32>& output) noexcept;
}
