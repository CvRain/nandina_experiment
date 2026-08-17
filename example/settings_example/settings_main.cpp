//
// Nandina application bootstrap - router-driven settings dashboard.
//

#include "settings_example.hpp"

#include "app/nan_application.hpp"

using namespace nandina;

auto main() -> int {
    app::NanApplication application(app::NanApplicationConfig::for_process("org.nandina.example"));
    application.use_store<examples::settings::SettingsStore>();
    return application.run_page<examples::settings::ShellPage>({
        .title = "Nandina Settings",
        .width = 960,
        .height = 640,
        .target_fps = 120,
        .resizable = true,
        .vsync = true,
    });
}
