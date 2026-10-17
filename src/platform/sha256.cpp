#include <nostale_hk/platform/sha256.hpp>

#include <bcrypt.h>
namespace nostale::hk::platform {
namespace {
class algorithm_handle final {
  public:
    ~algorithm_handle() noexcept {
        if (value != nullptr) {
            ::BCryptCloseAlgorithmProvider(value, 0);
        }
    }
    BCRYPT_ALG_HANDLE value = nullptr;
};

class hash_handle final {
  public:
    ~hash_handle() noexcept {
        if (value != nullptr) {
            ::BCryptDestroyHash(value);
        }
    }
    BCRYPT_HASH_HANDLE value = nullptr;
};

} // namespace
bool sha256_file(HANDLE file, std::array<std::byte, 32>& output) noexcept {
    algorithm_handle algorithm;
    if (::BCryptOpenAlgorithmProvider(&algorithm.value, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) {
        return false;
    }

    hash_handle hash;
    // Windows owns the backing allocation and releases it with the hash handle.
    if (::BCryptCreateHash(algorithm.value, &hash.value, nullptr, 0, nullptr, 0, 0) < 0) {
        return false;
    }

    std::array<UCHAR, 64 * 1024> buffer{};
    bool success = true;
    for (;;) {
        DWORD read = 0;
        if (::ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr) ==
            FALSE) {
            success = false;
            break;
        }
        if (read == 0) {
            break;
        }
        if (::BCryptHashData(hash.value, buffer.data(), read, 0) < 0) {
            success = false;
            break;
        }
    }
    if (success && ::BCryptFinishHash(hash.value, reinterpret_cast<PUCHAR>(output.data()),
                                      static_cast<ULONG>(output.size()), 0) < 0) {
        success = false;
    }

    return success;
}

} // namespace nostale::hk::platform
