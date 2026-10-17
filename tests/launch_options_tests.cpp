#include "../src/launcher/launch_options.hpp"

#include <cstdio>
#include <initializer_list>
#include <string_view>

namespace {
int failures = 0;
void require(bool condition, const char* behavior) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", behavior); ++failures; }
}
auto parse(std::initializer_list<std::wstring_view> arguments) {
    return nostale::hk::launcher::parse_launch_options({arguments.begin(), arguments.size()});
}
}

int main() {
    const auto normal = parse({L"--launch-client", L"C:\\Games\\NosTale\\NostaleX.exe"});
    require(normal && !normal->disable_xigncode3 && !normal->validate_only,
        "normal launch must leave Xigncode unchanged");
    const auto development = parse({L"--launch-client", L"C:\\client with spaces\\NostaleX.exe",
        L"--disable-xigncode3", L"--validate-only"});
    require(development && development->disable_xigncode3 && development->validate_only &&
        development->executable == L"C:\\client with spaces\\NostaleX.exe", "explicit opt-in and path preserved");
    require(!parse({}), "missing arguments rejected");
    require(!parse({L"--launch-client", L"--disable-xigncode3"}), "missing path rejected");
    require(!parse({L"--launch-client", L"NostaleX.exe", L"--disable-xigncode3", L"--disable-xigncode3"}),
        "duplicate opt-in rejected");
    require(!parse({L"--launch-client", L"NostaleX.exe", L"--whatever"}), "unknown option rejected");
    if (failures == 0) std::puts("LAUNCH_OPTIONS=PASS");
    return failures == 0 ? 0 : 1;
}
