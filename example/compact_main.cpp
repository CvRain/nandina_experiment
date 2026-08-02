//
// Minimal Nandina application bootstrap.
//

#include "compact_todo.hpp"

#include "app/nan_application.hpp"

using namespace nandina;

auto main() -> int {
    app::NanApplication application(
        app::NanApplicationConfig::for_process("org.nandina.compact-todo")
    );
    (void)application.use_store<examples::compact_todo::Store>();
    return application.run(
        app::WindowConfig {
            .title = "Nandina Compact Todo",
            .width = 720,
            .height = 420,
            .target_fps = 120,
            .resizable = true,
            .vsync = true,
        },
        examples::compact_todo::build
    );
}
