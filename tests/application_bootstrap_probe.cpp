#include "app/nan_application.hpp"

#include <filesystem>
#include <iostream>

using namespace nandina;

auto main(int argc, char** argv) -> int {
    if (argc != 4) { return 2; }
    app::NanApplication application({
        .application_id = argv[1],
        .executable_path = std::filesystem::absolute(argv[2]),
        .environment = {{"HOME", "/nonexistent-home"}, {"XDG_DATA_DIRS", "/nonexistent-data"}},
    });
    auto font = application.resources().require(resource::ResourceKey("fonts/default"));
    if (!font) { std::cerr << font.error().message; return 1; }
    const std::string_view expected(argv[3]);
    if (expected == "package") {
        return (*font)->key().value() == "fonts/default-ui.ttf" ? 0 : 1;
    }
    if (expected == "builtin") {
        return (*font)->key().value() == "fonts/default"
            && (*font)->storage() == resource::ResourceStorage::embedded_blob ? 0 : 1;
    }
    return 2;
}
