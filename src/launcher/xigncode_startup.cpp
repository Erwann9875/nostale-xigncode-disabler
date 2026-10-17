#include "xigncode_startup.hpp"

#include <nostale_hk/contracts/verified_client.hpp>

#include <algorithm>

namespace nostale::hk::launcher {
namespace {

DWORD read_exact(HANDLE process, const void* address, void* output, SIZE_T size) noexcept {
    SIZE_T read = 0;
    if (!::ReadProcessMemory(process, address, output, size, &read)) return ::GetLastError();
    return read == size ? ERROR_SUCCESS : ERROR_PARTIAL_COPY;
}

} // namespace

DWORD configure_xigncode_startup(HANDLE process, bool disable) noexcept {
    const auto base = static_cast<std::uintptr_t>(contracts::preferred_base);
    MEMORY_BASIC_INFORMATION mapping{};
    if (::VirtualQueryEx(process, reinterpret_cast<const void*>(base), &mapping,
            sizeof(mapping)) != sizeof(mapping)) return ERROR_INVALID_ADDRESS;
    if (mapping.Type != MEM_IMAGE || reinterpret_cast<std::uintptr_t>(mapping.AllocationBase) != base)
        return ERROR_INVALID_ADDRESS;

    IMAGE_DOS_HEADER dos{};
    DWORD error = read_exact(process, reinterpret_cast<const void*>(base), &dos, sizeof(dos));
    if (error != ERROR_SUCCESS) return error;
    if (dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < static_cast<LONG>(sizeof(IMAGE_DOS_HEADER)) ||
        dos.e_lfanew > static_cast<LONG>(0x400 - sizeof(IMAGE_NT_HEADERS32))) return ERROR_BAD_EXE_FORMAT;
    IMAGE_NT_HEADERS32 pe{};
    error = read_exact(process, reinterpret_cast<const void*>(base + static_cast<std::uintptr_t>(dos.e_lfanew)),
        &pe, sizeof(pe));
    if (error != ERROR_SUCCESS) return error;
    if (pe.Signature != IMAGE_NT_SIGNATURE || pe.FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        pe.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        pe.OptionalHeader.ImageBase != base || pe.OptionalHeader.SizeOfImage != 0x514000U ||
        pe.OptionalHeader.AddressOfEntryPoint != 0x32B06CU) return ERROR_BAD_EXE_FORMAT;

    void* site = reinterpret_cast<void*>(base + xigncode_startup_rva);
    std::array<unsigned char, xigncode_startup_original.size()> bytes{};
    error = read_exact(process, site, bytes.data(), bytes.size());
    if (error != ERROR_SUCCESS) return error;
    if (bytes != xigncode_startup_original) return ERROR_REVISION_MISMATCH;
    if (!disable) return ERROR_SUCCESS;

    // The client has not executed: no Xigncode module/callback may already be initialized.
    std::uint32_t module = 1;
    error = read_exact(process, reinterpret_cast<const void*>(base + 0x4CA4B8U),
        &module, sizeof(module));
    if (error != ERROR_SUCCESS) return error;
    if (module != 0) return ERROR_INVALID_STATE;

    DWORD previous = 0;
    if (!::VirtualProtectEx(process, site, xigncode_startup_disabled.size(),
            PAGE_EXECUTE_READWRITE, &previous)) return ::GetLastError();
    SIZE_T written = 0;
    const bool complete = ::WriteProcessMemory(process, site, xigncode_startup_disabled.data(),
        xigncode_startup_disabled.size(), &written) != FALSE;
    error = complete ? ERROR_SUCCESS : ::GetLastError();
    DWORD unused = 0;
    if (!::VirtualProtectEx(process, site, xigncode_startup_disabled.size(), previous, &unused))
        return ::GetLastError();
    if (error != ERROR_SUCCESS) return error;
    if (written != xigncode_startup_disabled.size()) return ERROR_PARTIAL_COPY;
    if (!::FlushInstructionCache(process, site, xigncode_startup_disabled.size())) return ::GetLastError();

    auto expected = xigncode_startup_original;
    std::copy(xigncode_startup_disabled.begin(), xigncode_startup_disabled.end(), expected.begin());
    error = read_exact(process, site, bytes.data(), bytes.size());
    if (error != ERROR_SUCCESS) return error;
    return bytes == expected ? ERROR_SUCCESS : ERROR_WRITE_FAULT;
}

} // namespace nostale::hk::launcher
