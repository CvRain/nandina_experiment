module;

#include <memory>
#include <string>

export module Nandina.Core.Button;

import Nandina.Core.Component;
import Nandina.Core.Event;

export namespace Nandina {

    // ── Button ────────────────────────────────────────────────────────────────
    // A clickable button composed of a FocusComponent ring layer and a
    // RectangleComponent fill layer, following the Action composition model.
    class Button final : public Component {
    public:
        static auto Create() -> std::unique_ptr<Button> {
            return std::unique_ptr<Button>(new Button());
        }

        auto set_background(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                            std::uint8_t a = 255) noexcept -> Button& {
            if (rect_layer_) {
                rect_layer_->color(r, g, b, a);
            }
            return *this;
        }

        auto text(std::string value) -> Button& {
            text_ = std::move(value);
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto get_text() const -> const std::string& { return text_; }

        // Override set_bounds to keep child layers in sync
        auto set_bounds(float x, float y, float width, float height) noexcept -> Widget& override {
            Component::set_bounds(x, y, width, height);
            if (focus_layer_) {
                focus_layer_->set_bounds(x - 2.0f, y - 2.0f, width + 4.0f, height + 4.0f);
            }
            if (rect_layer_) {
                rect_layer_->set_bounds(x, y, width, height);
            }
            return *this;
        }

    protected:
        auto handle_event(Event& ev) -> bool override {
            if (ev.type == EventType::mouse_button_press && contains(ev.x, ev.y)) {
                pressed_ = true;
                if (rect_layer_) { rect_layer_->color(29, 78, 216); }
                ev.mark_handled();
                return true;
            }

            if (ev.type == EventType::mouse_button_release && pressed_) {
                pressed_ = false;
                if (rect_layer_) { rect_layer_->color(37, 99, 235); }
                ev.mark_handled();
                return true;
            }

            if (ev.type == EventType::click && contains(ev.x, ev.y)) {
                if (focus_layer_) { focus_layer_->focused(true); }
                return Component::handle_event(ev);
            }

            return false;
        }

    private:
        Button() {
            set_background(0, 0, 0, 0);

            auto focus = FocusComponent::Create();
            focus_layer_ = focus.get();
            focus_layer_->focused(false);
            add_child(std::move(focus));

            auto rect = RectangleComponent::Create();
            rect_layer_ = rect.get();
            rect_layer_->color(37, 99, 235).radius(8.0f);
            add_child(std::move(rect));
        }

        std::string         text_;
        bool                pressed_      = false;
        FocusComponent*     focus_layer_  = nullptr;
        RectangleComponent* rect_layer_   = nullptr;
    };

} // export namespace Nandina
