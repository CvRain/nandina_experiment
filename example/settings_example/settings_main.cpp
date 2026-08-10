//
// Minimal Nandina application bootstrap.
//

#include "settings_example.hpp"

#include "app/nan_application.hpp"

using namespace nandina;

auto main() -> int {
    return app::run(
        {
            .id = "org.nandina.example",
            .window =
                {
                    .title = "Nandina Settings",
                    .width = 720,
                    .height = 520,
                    .target_fps = 120,
                    .resizable = true,
                    .vsync = true,
                },
        },
        examples::settings::build
    );
}
