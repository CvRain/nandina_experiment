//
// Nandina — standard single-page application example.
//

#include "app/nan_application.hpp"
#include "app/nan_page.hpp"
#include "app/nan_window.hpp"
#include "foundation/geometry.hpp"
#include "reactive/computed.hpp"
#include "reactive/scope.hpp"
#include "reactive/signal.hpp"
#include "scene/control.hpp"
#include "theme/theme.hpp"
#include "widget/button.hpp"
#include "widget/label.hpp"
#include "widget/layout.hpp"

#include <memory>
#include <string>
#include <string_view>

using namespace nandina;

class CounterPage final: public app::NanPageT<app::NoParams> {
public:
    CounterPage() = default;

    [[nodiscard]] auto route_key() const -> std::string_view override {
        return "counter";
    }

    [[nodiscard]] auto build(app::PageContext& context)
        -> std::shared_ptr<scene::NanNode2D> override {
        auto& graph = context.graph();
        const auto& app_theme = context.theme();

        auto root = std::make_shared<scene::NanControl>(foundation::NanSize(640.0F, 360.0F));
        root->set_background(app_theme.palette.surface);

        const auto todo_label = widget::Label::create(graph, "Current todo", app_theme);
        todo_label->set_font_size(22);
        todo_label->set_overflow(widget::primitives::TextOverflow::clip);

        const auto todo_item_1 =
            widget::Label::create(graph, "todo item 1: Hello world!", app_theme);
        todo_item_1->set_font_size(18);

        const auto todo_item_2 = widget::Label::create(graph, "todo item 2", app_theme);
        todo_item_2->set_font_size(18);

        const auto todo_item_3 = widget::Label::create(graph, "todo item 3", app_theme);
        todo_item_3->set_font_size(18);

        const auto todo_item_group = widget::Column::create();
        todo_item_group->set_gap(5).add(todo_item_1).add(todo_item_2).add(todo_item_3);

        const auto actions = widget::Column::create();
        actions->set_gap(15.0F)
            .set_main_alignment(widget::LayoutAlignment::start)
            .set_cross_alignment(widget::LayoutAlignment::start)
            .add(todo_label)
            .add(todo_item_group);

        root->add_child(actions);
        return root;
    }

private:
};

auto main() -> int {
    app::NanApplication application;
    application.set_theme(theme::default_theme());

    app::NanWindow window {
        application,
        app::WindowConfig {
            .title = "Nandina - Counter Demo",
            .width = 640,
            .height = 360,
            .target_fps = 120,
            .decorated = true,
            .resizable = true,
            .msaa = true,
            .vsync = false,
            .background = application.theme().palette.surface,
        },
    };

    window.use_router().push<CounterPage>();
    return application.run(window);
}
