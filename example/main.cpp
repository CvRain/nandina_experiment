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

#include <memory>
#include <string>
#include <string_view>
#include <meta>

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

        auto label = widget::Label::create(graph, "", app_theme);
        label->set_position(foundation::NanPoint(48.0F, 48.0F));
        label->bind_text(*count_text_);
        root->add_child(label);

        auto decrement = widget::Button::create("-1", app_theme);
        decrement->set_position(foundation::NanPoint(48.0F, 112.0F));
        decrement->set_tone(theme::ButtonTone::secondary);
        decrement->set_treatment(theme::ButtonTreatment::outlined);
        decrement->set_on_click([this] { count_->update([](int& value) { --value; }); });
        root->add_child(decrement);

        auto increment = widget::Button::create("+1", app_theme);
        increment->set_position(foundation::NanPoint(128.0F, 112.0F));
        increment->set_tone(theme::ButtonTone::primary);
        increment->set_treatment(theme::ButtonTreatment::filled);
        increment->set_on_click([this] { count_->update([](int& value) { ++value; }); });
        root->add_child(increment);

        return root;
    }

private:
    std::unique_ptr<reactive::Signal<int>> count_;
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
