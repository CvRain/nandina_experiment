#include <catch2/catch_test_macros.hpp>

#include "app/detail/linux_clipboard.hpp"

using namespace nandina;

TEST_CASE("Wayland session detection requires a non-empty display", "[app][clipboard][linux]") {
    REQUIRE_FALSE(app::detail::is_wayland_session(nullptr));
    REQUIRE_FALSE(app::detail::is_wayland_session(""));
    REQUIRE(app::detail::is_wayland_session("wayland-0"));
}

TEST_CASE("Clipboard command reader preserves UTF-8 output", "[app][clipboard][linux]") {
    const auto text = app::detail::read_command_output("printf '\\344\\270\\255\\346\\226\\207'");

    REQUIRE(text == "中文");
    REQUIRE_FALSE(app::detail::read_command_output("exit 7"));
}

TEST_CASE("Clipboard command writer reports process success", "[app][clipboard][linux]") {
    constexpr auto verify_text = "test \"$(cat)\" = '跨应用粘贴'";

    REQUIRE(app::detail::write_command_input(verify_text, "跨应用粘贴"));
    REQUIRE_FALSE(app::detail::write_command_input(verify_text, "错误内容"));
    REQUIRE_FALSE(app::detail::write_command_input("exit 9", "跨应用粘贴"));
}
