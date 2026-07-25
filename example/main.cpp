//
// Nandina - paired imperative/DSL Todo application example.
//

#include "todo_app.hpp"

#include "app/nan_application.hpp"
#include "app/nan_window.hpp"
#include "text/font_family.hpp"
#include "theme/nan_style.hpp"
#include "theme/theme.hpp"
#include "theme/theme_manager.hpp"

#include <memory>
#include <utility>

using namespace nandina;

namespace
{
    class TodoWindow final: public app::NanWindow {
    public:
        using NanWindow::NanWindow;

    protected:
        void on_setup() override {
            use_router().push<examples::todo::ImperativeTodoPage>(
                examples::todo::TodoPageParams {.source = "应用启动", .visit = 1}
            );
        }
    };
} // namespace

auto main() -> int {
    app::NanApplication application(app::NanApplicationConfig::for_process("org.nandina.todo"));
    (void)application.use_store<examples::todo::TodoStore>();
    auto chinese_fallback = text::register_optional_font_fallback(
        application.font_families(),
        application.resources(),
        resource::ResourceKey("families/zh-cn"),
        resource::ResourceKey("fonts/fallback/zh-cn")
    );
    if (!chinese_fallback) {
        return 2;
    }

    auto dark_theme = theme::default_theme();
    auto light_theme = theme::default_theme();
    light_theme.palette.background = theme::nan_color(0.99F, 0.005F, 270.0F);
    light_theme.palette.on_background = theme::nan_color(0.18F, 0.02F, 275.0F);
    light_theme.palette.primary = theme::nan_color(0.56F, 0.18F, 250.0F);
    light_theme.palette.on_primary = theme::nan_color(0.98F, 0.01F, 250.0F);
    light_theme.palette.secondary = theme::nan_color(0.62F, 0.13F, 150.0F);
    light_theme.palette.on_secondary = theme::nan_color(0.16F, 0.02F, 150.0F);
    light_theme.palette.surface = theme::nan_color(0.97F, 0.01F, 270.0F);
    light_theme.palette.on_surface = theme::nan_color(0.22F, 0.02F, 275.0F);
    light_theme.palette.surface_variant = theme::nan_color(0.91F, 0.02F, 275.0F);
    light_theme.palette.on_surface_variant = theme::nan_color(0.43F, 0.03F, 275.0F);
    light_theme.palette.outline = theme::nan_color(0.58F, 0.02F, 275.0F);
    light_theme.palette.outline_variant = theme::nan_color(0.78F, 0.02F, 275.0F);
    light_theme.palette.focus_ring = light_theme.palette.primary;
    light_theme.palette.selection = light_theme.palette.primary.with_alpha(0.28F);

    auto style = std::make_shared<theme::NanStyle>();
    theme::ButtonStyleRule buttons;
    buttons.radius = theme::ThemeScalar::literal(7.0F);
    style->add_button_rule(std::move(buttons));
    theme::TextFieldStyleRule focused_field;
    focused_field.state = theme::TextFieldVisualState::focused;
    focused_field.border_color = theme::ThemeColor::token(theme::ColorToken::primary);
    focused_field.focus_ring_color = theme::ThemeColor::token(theme::ColorToken::primary);
    style->add_text_field_rule(std::move(focused_field));

    auto& themes = application.theme_manager();
    (void)themes.register_theme("todo-dark", dark_theme);
    (void)themes.register_theme("todo-light", light_theme);
    (void)themes.register_family("todo", "todo-light", "todo-dark");
    themes.set_style(std::move(style));
    (void)themes.activate_family("todo");
    themes.set_preference(theme::ThemePreference::system);

    TodoWindow window {
        application,
        app::WindowConfig {
            .title = "Nandina - Todo Authoring Demo",
            .width = 760,
            .height = 420,
            .target_fps = 120,
            .decorated = true,
            .resizable = true,
            .msaa = true,
            .vsync = false,
            .background = application.theme().palette.background,
        },
    };

    return application.run(window);
}
