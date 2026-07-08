//
// Nandina — standard single-page application example.
//

#include "app/nan_application.hpp"
#include "app/nan_page.hpp"
#include "app/nan_window.hpp"
#include "foundation/geometry.hpp"
#include "reactive/computed.hpp"
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

    ~CounterPage() override {
        if (count_text_ != nullptr) {
            count_text_->dispose();
        }
    }

    [[nodiscard]] auto route_key() const -> std::string_view override {
        return "counter";
    }

    [[nodiscard]] auto build(app::PageContext& context)
        -> std::shared_ptr<scene::NanNode2D> override {
        auto& graph = context.graph();
        const auto& app_theme = context.theme();

        count_ = std::make_unique<reactive::Signal<int>>(graph, 0);
        count_text_ = reactive::make_computed(graph, [this] {
            return std::string("Count: ") + std::to_string(count_->get());
        });

        auto root = std::make_shared<scene::NanControl>(foundation::NanSize(640.0F, 360.0F));
        root->set_background(app_theme.palette.surface);

        const auto test_label =
            widget::Label::create(graph, "A quick brown fox jump to a lazy dog", app_theme);
        test_label->set_font_size(26);

        const auto label = widget::Label::create(graph, "", app_theme);
        label->bind_text(*count_text_);

        const auto decrement = widget::Button::create("-1", app_theme);
        decrement->set_tone(theme::ButtonTone::secondary);
        decrement->set_treatment(theme::ButtonTreatment::outlined);
        decrement->set_on_click([this] { count_->update([](int& value) { --value; }); });

        const auto increment = widget::Button::create("+1", app_theme);
        increment->set_tone(theme::ButtonTone::primary);
        increment->set_treatment(theme::ButtonTreatment::filled);
        increment->set_on_click([this] { count_->update([](int& value) { ++value; }); });

        const auto reset = widget::Button::create("reset", app_theme);
        reset->set_tone(theme::ButtonTone::secondary);
        reset->set_treatment(theme::ButtonTreatment::tonal);
        reset->set_on_click([this] { count_->update([](int& value) { value = 0; }); });

        const auto actions = widget::Flow::create();
        actions->set_gap(12.0F)
            .set_run_gap(12.0F)
            .set_main_alignment(widget::LayoutAlignment::center)
            .add(decrement)
            .add(increment)
            .add(reset);

        const auto content = widget::Column::create();
        content->set_gap(24.0F)
            .set_cross_alignment(widget::LayoutAlignment::center)
            .add(test_label)
            .add(label)
            .add(actions);

        const auto padded = widget::Padding::create(foundation::NanInsets::all(48.0F));
        padded->set_child(content);

        const auto centered = widget::Center::create();
        centered->set_child(padded);
        root->add_child(centered);

        return root;
    }

private:
    reactive::UniqueSignal<int> count_;
    reactive::Computed<std::string>* count_text_ = nullptr;
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
