#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace nandina::app::detail
{
    [[nodiscard]] auto is_wayland_session(const char* wayland_display) noexcept -> bool;

    [[nodiscard]] auto read_command_output(const char* command) -> std::optional<std::string>;
    auto write_command_input(const char* command, std::string_view text) -> bool;

    [[nodiscard]] auto read_wayland_clipboard() -> std::optional<std::string>;
    auto write_wayland_clipboard(std::string_view text) -> bool;
} // namespace nandina::app::detail
