//
// widget/primitives/editable_text — focused text editing primitive.
//

#include "editable_text.hpp"

#include "../../foundation/utf8.hpp"
#include "../../render/draw_context.hpp"
#include "../../scene/input_event.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace nandina::widget::primitives
{
    namespace
    {
        constexpr int key_backspace = 259;
        constexpr int key_delete = 261;
        constexpr int key_right = 262;
        constexpr int key_left = 263;
        constexpr int key_home = 268;
        constexpr int key_end = 269;
        constexpr int key_a = 65;
        constexpr float caret_width = 1.0F;

        [[nodiscard]] auto clamp_grapheme_boundary(
            const std::string_view text,
            const std::size_t offset
        ) -> std::size_t {
            const auto requested = std::min(offset, text.size());
            for (const auto& grapheme: foundation::utf8::grapheme_ranges(text)) {
                const auto end = grapheme.offset + grapheme.length;
                if (requested < end) {
                    return grapheme.offset;
                }
            }
            return text.size();
        }

        [[nodiscard]] auto next_grapheme_boundary(
            const std::string_view text,
            const std::size_t offset
        ) -> std::size_t {
            const auto requested = std::min(offset, text.size());
            for (const auto& grapheme: foundation::utf8::grapheme_ranges(text)) {
                const auto end = grapheme.offset + grapheme.length;
                if (requested == grapheme.offset) {
                    return requested;
                }
                if (requested < end) {
                    return end;
                }
            }
            return text.size();
        }
    }

    EditableText::EditableText(std::string value): value_(std::move(value)), caret_(value_.size()), text_(value_) {
        clear_selection();
    }

    void EditableText::set_value(std::string value) {
        value_ = std::move(value);
        caret_ = clamp_grapheme_boundary(value_, caret_);
        caret_affinity_ = TextAffinity::downstream;
        clear_selection();
        clear_composition();
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

    void EditableText::set_caret(const std::size_t offset, const TextAffinity affinity) {
        caret_ = clamp_grapheme_boundary(value_, offset);
        caret_affinity_ = affinity;
        clear_selection();
    }

    auto EditableText::caret() const -> std::size_t {
        return caret_;
    }

    auto EditableText::caret_affinity() const -> TextAffinity {
        return caret_affinity_;
    }

    void EditableText::set_selection(TextSelection selection) {
        selection.anchor = clamp_grapheme_boundary(value_, selection.anchor);
        selection.focus = clamp_grapheme_boundary(value_, selection.focus);
        selection_ = selection;
        caret_ = selection.focus;
        caret_affinity_ = selection.focus_affinity;
    }

    auto EditableText::selection() const -> TextSelection { return selection_; }

    auto EditableText::has_selection() const -> bool {
        return selection_.anchor != selection_.focus;
    }

    void EditableText::select_all() {
        selection_ = TextSelection {
            .anchor = 0,
            .anchor_affinity = TextAffinity::downstream,
            .focus = value_.size(),
            .focus_affinity = TextAffinity::upstream,
        };
        caret_ = selection_.focus;
        caret_affinity_ = selection_.focus_affinity;
    }

    void EditableText::clear_selection() {
        selection_ = TextSelection {
            .anchor = caret_,
            .anchor_affinity = caret_affinity_,
            .focus = caret_,
            .focus_affinity = caret_affinity_,
        };
    }

    void EditableText::set_read_only(const bool value) { read_only_ = value; }
    auto EditableText::read_only() const -> bool { return read_only_; }
    void EditableText::set_selection_color(const foundation::NanColor color) {
        selection_color_ = color;
    }

    void EditableText::set_composition(TextComposition composition) {
        composition.selection_start = std::min(composition.selection_start, composition.text.size());
        composition.selection_end = std::min(composition.selection_end, composition.text.size());
        composition_ = std::move(composition);
    }
    void EditableText::clear_composition() { composition_.reset(); }
    auto EditableText::composition() const -> const std::optional<TextComposition>& {
        return composition_;
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

    void EditableText::set_text_pipeline(TextPipeline pipeline) {
        text_.set_text_pipeline(pipeline);
        mark_layout_dirty();
        (void)text_.measure_layout(last_layout_constraints());
        set_size(text_.size());
    }

    auto EditableText::text_pipeline() const -> TextPipeline {
        return text_.text_pipeline();
    }

    void EditableText::draw_at(render::DrawContext& ctx, foundation::NanPoint position) {
        if (has_selection()) {
            const auto& layout = text_.layout_result();
            if (!layout.lines.empty()) {
                const auto& line = layout.lines.front();
                const auto lower = std::min(selection_.anchor, selection_.focus);
                const auto upper = std::max(selection_.anchor, selection_.focus);
                const auto color = selection_color_.with_alpha(
                    selection_color_.alpha() * ctx.opacity()
                );
                for (std::size_t index = 1; index < line.caret_stops.size(); ++index) {
                    const auto& left = line.caret_stops[index - 1];
                    const auto& right = line.caret_stops[index];
                    const auto source_begin = std::min(left.source_offset, right.source_offset);
                    const auto source_end = std::max(left.source_offset, right.source_offset);
                    if (source_end <= lower || source_begin >= upper || left.x == right.x) {
                        continue;
                    }
                    ctx.device().draw_rect(
                        foundation::NanRect::from_xywh(
                            position.get_x() + std::min(left.x, right.x),
                            position.get_y(),
                            std::abs(right.x - left.x),
                            line.size.get_height()
                        ),
                        color
                    );
                }
            }
        }
        text_.draw_at(ctx, position);

        if (!focused_) {
            return;
        }

        const auto color = text_.color().with_alpha(text_.color().alpha() * ctx.opacity());
        const float x = position.get_x() + caret_x();
        ctx.device().draw_line(
            foundation::NanPoint(x, position.get_y()),
            foundation::NanPoint(x, position.get_y() + text_.height()),
            caret_width,
            color
        );
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
                if (read_only_) {
                    event.accept();
                    return true;
                }
                auto& text_event = static_cast<scene::TextInputEvent&>(event);
                insert_text(text_event.text());
                event.accept();
                return true;
            }
            case scene::EventType::key: {
                auto& key_event = static_cast<scene::KeyEvent&>(event);
                if (key_event.is_pressed() && key_event.modifiers().ctrl
                    && key_event.keycode() == key_a)
                {
                    select_all();
                    event.accept();
                    return true;
                }
                if (key_event.is_pressed() && key_event.keycode() == key_backspace) {
                    if (!read_only_) {
                        has_selection() ? erase_selection() : erase_before_caret();
                    }
                    event.accept();
                    return true;
                }
                if (!key_event.is_pressed()) {
                    return false;
                }
                const bool extend = key_event.modifiers().shift;
                switch (key_event.keycode()) {
                    case key_delete:
                        if (!read_only_) { has_selection() ? erase_selection() : erase_after_caret(); }
                        break;
                    case key_left: move_caret_visual(-1, extend); break;
                    case key_right: move_caret_visual(1, extend); break;
                    case key_home: move_caret_to_visual_edge(false, extend); break;
                    case key_end: move_caret_to_visual_edge(true, extend); break;
                    default: return false;
                }
                event.accept();
                return true;
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
        draw_at(ctx, pos);
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
        if (has_selection()) {
            const auto lower = std::min(selection_.anchor, selection_.focus);
            const auto upper = std::max(selection_.anchor, selection_.focus);
            value_.erase(lower, upper - lower);
            caret_ = lower;
            caret_affinity_ = TextAffinity::downstream;
            clear_selection();
        }
        value_.insert(caret_, text);
        caret_ += text.size();
        caret_ = next_grapheme_boundary(value_, caret_);
        caret_affinity_ = TextAffinity::downstream;
        clear_selection();
        clear_composition();
        sync_text();
        emit_change();
    }

    void EditableText::erase_before_caret() {
        if (caret_ == 0 || value_.empty()) {
            return;
        }
        const auto graphemes = foundation::utf8::grapheme_ranges(value_);
        const foundation::utf8::ByteRange* previous = nullptr;
        for (const auto& grapheme: graphemes) {
            if (grapheme.offset + grapheme.length <= caret_) {
                previous = &grapheme;
            }
        }
        if (previous == nullptr) {
            return;
        }
        value_.erase(previous->offset, previous->length);
        caret_ = previous->offset;
        caret_affinity_ = TextAffinity::downstream;
        clear_selection();
        sync_text();
        emit_change();
    }

    void EditableText::erase_after_caret() {
        const auto graphemes = foundation::utf8::grapheme_ranges(value_);
        const auto next = std::ranges::find_if(graphemes, [&](const auto& grapheme) {
            return grapheme.offset >= caret_;
        });
        if (next == graphemes.end()) {
            return;
        }
        value_.erase(next->offset, next->length);
        caret_affinity_ = TextAffinity::downstream;
        clear_selection();
        sync_text();
        emit_change();
    }

    void EditableText::erase_selection() {
        if (!has_selection()) { return; }
        const auto lower = std::min(selection_.anchor, selection_.focus);
        const auto upper = std::max(selection_.anchor, selection_.focus);
        value_.erase(lower, upper - lower);
        caret_ = lower;
        caret_affinity_ = TextAffinity::downstream;
        clear_selection();
        clear_composition();
        sync_text();
        emit_change();
    }

    void EditableText::move_caret_visual(const int direction, const bool extend) {
        const auto& layout = text_.layout_result();
        if (layout.lines.empty() || layout.lines.front().caret_stops.empty() || direction == 0) {
            return;
        }
        const auto& line = layout.lines.front();
        const auto current = line.caret_for_source(caret_, caret_affinity_);
        auto current_index = std::size_t {0};
        float nearest = std::numeric_limits<float>::infinity();
        for (std::size_t index = 0; index < line.caret_stops.size(); ++index) {
            const auto& stop = line.caret_stops[index];
            if (stop.source_offset != current.source_offset) {
                continue;
            }
            const float distance = std::abs(stop.x - current.x);
            if (distance < nearest) {
                current_index = index;
                nearest = distance;
            }
        }
        if (has_selection() && !extend) {
            const auto anchor = line.caret_for_source(selection_.anchor, selection_.anchor_affinity);
            const auto focus = line.caret_for_source(selection_.focus, selection_.focus_affinity);
            update_selection_focus(direction < 0
                ? (anchor.x <= focus.x ? anchor : focus)
                : (anchor.x >= focus.x ? anchor : focus), false);
            return;
        }
        const auto target_index = direction < 0
            ? (current_index == 0 ? 0 : current_index - 1)
            : std::min(current_index + 1, line.caret_stops.size() - 1);
        const auto& target = line.caret_stops[target_index];
        update_selection_focus(target, extend);
    }

    void EditableText::move_caret_to_visual_edge(const bool end, const bool extend) {
        const auto& layout = text_.layout_result();
        if (layout.lines.empty() || layout.lines.front().caret_stops.empty()) {
            return;
        }
        const auto& stops = layout.lines.front().caret_stops;
        const auto& target = end ? stops.back() : stops.front();
        update_selection_focus(target, extend);
    }

    void EditableText::update_selection_focus(const TextCaretStop stop, const bool extend) {
        if (!extend) {
            selection_.anchor = stop.source_offset;
            selection_.anchor_affinity = stop.affinity;
        }
        selection_.focus = stop.source_offset;
        selection_.focus_affinity = stop.affinity;
        caret_ = stop.source_offset;
        caret_affinity_ = stop.affinity;
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
        if (layout.lines.empty()) {
            return 0.0F;
        }
        return layout.lines.front().caret_for_source(caret_, caret_affinity_).x;
    }

} // namespace nandina::widget::primitives
