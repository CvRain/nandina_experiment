#include <spdlog/spdlog.h>
#include <nandina_greet.hpp>

auto main() -> int {
    spdlog::log(spdlog::level::info, NandinaGreet::hello_world());
}
