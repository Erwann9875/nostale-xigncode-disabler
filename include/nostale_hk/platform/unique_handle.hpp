#pragma once

#include <Windows.h>

#include <utility>

namespace nostale::hk::platform {

class unique_handle final {
public:
    unique_handle() noexcept = default;
    explicit unique_handle(HANDLE value) noexcept : value_(value) {}

    unique_handle(const unique_handle&) = delete;
    unique_handle& operator=(const unique_handle&) = delete;

    unique_handle(unique_handle&& other) noexcept : value_(other.release()) {}

    unique_handle& operator=(unique_handle&& other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    ~unique_handle() noexcept { reset(); }

    [[nodiscard]] HANDLE get() const noexcept { return value_; }
    [[nodiscard]] explicit operator bool() const noexcept { return is_valid(value_); }

    [[nodiscard]] HANDLE release() noexcept {
        HANDLE value = value_;
        value_ = nullptr;
        return value;
    }

    void reset(HANDLE value = nullptr) noexcept {
        if (is_valid(value_)) {
            ::CloseHandle(value_);
        }
        value_ = value;
    }

private:
    static bool is_valid(HANDLE value) noexcept {
        return value != nullptr && value != INVALID_HANDLE_VALUE;
    }

    HANDLE value_ = nullptr;
};

} // namespace nostale::hk::platform
