#include "linux_clipboard.hpp"

#include <array>
#include <cstdio>

#include <sys/wait.h>

namespace nandina::app::detail
{
    namespace
    {
        [[nodiscard]] auto command_succeeded(const int status) noexcept -> bool {
            return status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
        }
    } // namespace

    auto is_wayland_session(const char* wayland_display) noexcept -> bool {
        return wayland_display != nullptr && wayland_display[0] != '\0';
    }

    auto read_command_output(const char* command) -> std::optional<std::string> {
        if (command == nullptr || command[0] == '\0') {
            return std::nullopt;
        }

        auto* pipe = popen(command, "r");
        if (pipe == nullptr) {
            return std::nullopt;
        }

        std::string output;
        std::array<char, 4096> buffer {};
        bool read_succeeded = true;
        while (true) {
            const auto count = std::fread(buffer.data(), 1, buffer.size(), pipe);
            output.append(buffer.data(), count);
            if (count == buffer.size()) {
                continue;
            }
            if (std::feof(pipe) != 0) {
                break;
            }
            read_succeeded = false;
            break;
        }

        const auto status = pclose(pipe);
        if (!read_succeeded || !command_succeeded(status)) {
            return std::nullopt;
        }
        return output;
    }

    auto write_command_input(const char* command, const std::string_view text) -> bool {
        if (command == nullptr || command[0] == '\0') {
            return false;
        }

        auto* pipe = popen(command, "w");
        if (pipe == nullptr) {
            return false;
        }

        std::size_t offset = 0;
        while (offset < text.size()) {
            const auto count = std::fwrite(text.data() + offset, 1, text.size() - offset, pipe);
            if (count == 0) {
                break;
            }
            offset += count;
        }

        const bool write_succeeded = offset == text.size() && std::fflush(pipe) == 0;
        const auto status = pclose(pipe);
        return write_succeeded && command_succeeded(status);
    }

    auto read_wayland_clipboard() -> std::optional<std::string> {
        return read_command_output("wl-paste --no-newline 2>/dev/null");
    }

    auto write_wayland_clipboard(const std::string_view text) -> bool {
        return write_command_input("wl-copy --type 'text/plain;charset=utf-8' 2>/dev/null", text);
    }
} // namespace nandina::app::detail
