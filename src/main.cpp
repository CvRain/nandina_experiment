// Standard headers MUST come before module imports when -fmodules-ts is active,
// otherwise GCC re-processes them as header units and hits redefinition errors.
#include <memory>
#include <print>
#include <string_view>
#include <utility>

import Nandina.Application;
import Nandina.Core;
import Nandina.Window;

class SettingsWindow final : public Nandina::WindowController {
protected:
    [[nodiscard]] auto title() const -> std::string_view override {
        return "Nandina — Composed UI MVP";
    }

    [[nodiscard]] auto initial_size() const -> std::pair<int, int> override {
        return {900, 640};
    }

    auto build_root() -> std::unique_ptr<Nandina::Widget> override {
        auto page = std::make_unique<Nandina::Component>();
        page->set_bounds(0, 0, 900, 640)
            .set_background(30, 30, 30);

        auto save_button = Nandina::Button::Create();
        save_button->text("Save Changes").set_bounds(560, 500, 220, 56);

        click_connection_ = save_button->on_click([] {
            std::println("[click] Save Changes");
        });

        auto cancel_button = Nandina::Button::Create();
        cancel_button->text("Cancel").set_bounds(330, 500, 180, 56);
        cancel_button->on_click([] {
            std::println("[click] Cancel");
        });

        page->add_child(std::move(cancel_button));
        page->add_child(std::move(save_button));
        return page;
    }

private:
    Nandina::Connection click_connection_{};
};

int main() {
    Nandina::NanApplication app;
    SettingsWindow window;
    return window.exec();
}
