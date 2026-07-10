//
// widget/primitives/editable_text — focused text editing primitive.
//

#include "editable_text.hpp"

#include "../../render/draw_context.hpp"
#include "../../scene/input_event.hpp"

#include <algorithm>
#include <utility>

namespace nandina::widget::primitives
{
    namespace
    {
        constexpr int key_backspace = 259;
        constexpr float caret_width = 1.0F;
    }

    EditableText::EditableText(std::string value): value_(std::move(value)), caret_(value_.size()), text_(value_) {}

    void EditableText::set_value(std::string value) {
        value_ = std::move(value);
        caret_ = std::min(caret_, value_.size());
        sync_text();
    }

    auto EditableText::value() const -> std::string_view {
        return value_;
    }

    void EditableText::set_style(TextStyle style) {
        text_.set_style(style);
        mark_layout_dirty();
    }

    auto EditableText::style() const -> const TextStyle& {
        return text_.style();
    }

    void EditableText::set_caret(std::size_t offset) {
        caret_ = std::min(offset, value_.size());
    }

    auto EditableText::caret() const -> std::size_t {
        return caret_;
    }

    void EditableText::set_on_change(std::function<void(std::string_view)> callback) {
        on_change_ = std::move(callback);
    }

    auto EditableText::text_node() -> Text& {
        return text_;
    }

    auto EditableText::text_node() const -> const Text& {
        return text_;
    }

    auto EditableText::is_focusable() const -> bool {
        return true;
    }

    auto EditableText::on_input(scene::InputEvent& event) -> bool {
        switch (event.type()) {
            case scene::EventType::focus_enter:
                focused_ = true;
                event.accept();
                return true;
            case scene::EventType::focus_leave:
                focused_ = false;
                event.accept();
                return true;
            case scene::EventType::text_input: {
                auto& text_event = static_cast<scene::TextInputEvent&>(event);
                insert_text(text_event.text());
                event.accept();
                return true;
            }
            case scene::EventType::key: {
                auto& key_event = static_cast<scene::KeyEvent&>(event);
                if (key_event.is_pressed() && key_event.keycode() == key_backspace) {
                    erase_before_caret();
                    event.accept();
                    return true;
                }
                return false;
            }
            case scene::EventType::mouse_button:
            case scene::EventType::mouse_move:
            case scene::EventType::mouse_enter:
            case scene::EventType::mouse_leave:
            case scene::EventType::mouse_wheel:
                return false;
        }
        return false;
    }

    void EditableText::on_draw(render::DrawContext& ctx) {
        const auto pos = ctx.world_transform().transform_point(foundation::NanPoint::zero());
        text_.draw_at(ctx, pos);

        if (!focused_) {
            return;
        }

        const auto color = text_.color().with_alpha(text_.color().alpha() * ctx.opacity());
        const float x = pos.get_x() + caret_x();
        ctx.device().draw_line(
            foundation::NanPoint(x, pos.get_y()),
            foundation::NanPoint(x, pos.get_y() + text_.height()),
            caret_width,
            color
        );
    }

    auto EditableText::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto measured = text_.measure_layout(constraints);
        set_size(measured);
        return measured;
    }

    void EditableText::insert_text(std::string_view text) {
        if (text.empty()) {
            return;
        }
        value_.insert(caret_, text);
        caret_ += text.size();
        sync_text();
        emit_change();
    }

    void EditableText::erase_before_caret() {
        if (caret_ == 0 || value_.empty()) {
            return;
        }
        const auto erase_at = caret_ - 1;
        value_.erase(erase_at, 1);
        caret_ = erase_at;
        sync_text();
        emit_change();
    }

    void EditableText::sync_text() {
        text_.set_text(value_);
        mark_layout_dirty();
        (void)text_.measure_layout(last_layout_constraints());
        set_size(text_.size());
    }

    void EditableText::emit_change() {
        if (on_change_) {
            on_change_(value_);
        }
    }

    auto EditableText::caret_x() const -> float {
        const auto& layout = text_.layout_result();
        if (layout.lines.empty() || value_.empty()) {
            return 0.0F;
        }
        const auto visible = layout.lines.front().visible_text;
        const auto visible_count = std::min(caret_, visible.size());
        if (visible.empty()) {
            return 0.0F;
        }
        return layout.lines.front().size.get_width()
            * (static_cast<float>(visible_count) / static_cast<float>(visible.size()));
    }

} // namespace nandina::widget::primitives
