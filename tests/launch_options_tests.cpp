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
    using nostale::hk::launcher::is_configured_login_endpoint;
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
    require(is_configured_login_endpoint(L"127.0.0.1", L"4000"), "loopback login allowed");
    require(is_configured_login_endpoint(L"127.0.0.1", L"1"), "valid minimum port allowed");
    require(is_configured_login_endpoint(L"127.0.0.1", L"65535"), "valid maximum port allowed");
    require(is_configured_login_endpoint(L"203.0.113.1", L"4000"), "public-address setting allowed");
    require(is_configured_login_endpoint(L"192.168.1.20", L"4000"), "LAN login allowed");
    require(is_configured_login_endpoint(L"localhost", L"4000"), "local hostname allowed");
    require(is_configured_login_endpoint(L"login.example.test", L"4000"), "remote hostname allowed");
    for (const auto host : {L"", L" ", L"bad host", L"bad\thost", L"bad\r\nhost"})
        require(!is_configured_login_endpoint(host, L"4000"), "missing/whitespace host rejected");
    for (const auto port : {L"", L"0", L"65536", L"-1", L"4000junk", L"99999999999999999999"})
        require(!is_configured_login_endpoint(L"login.example.test", port), "missing/invalid login port rejected");
    if (failures == 0) std::puts("LAUNCH_OPTIONS_AND_ENDPOINT_VALIDATION=PASS");
    return failures == 0 ? 0 : 1;
}
