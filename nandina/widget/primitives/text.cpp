//
// widget/primitives/text — minimal text drawing primitive.
//

#include "text.hpp"
#include "../../foundation/utf8.hpp"
#include "../../render/draw_context.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nandina::widget::primitives
{

    namespace
    {
        constexpr float text_layout_epsilon = 0.01F;
    }

    Text::Text(std::string text): text_(std::move(text)) {
        update_metrics();
    }

    void Text::set_text(std::string text) {
        text_ = std::move(text);
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::text() const -> std::string_view {
        return text_;
    }

    void Text::set_style(TextStyle style) {
        style.max_lines = std::max(1, style.max_lines);
        style_ = style;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::style() const -> const TextStyle& {
        return style_;
    }

    void Text::set_color(foundation::NanColor color) {
        style_.color = color;
    }

    auto Text::color() const -> foundation::NanColor {
        return style_.color;
    }

    void Text::set_font_size(float size) {
        style_.font_size = size;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::font_size() const -> float {
        return style_.font_size;
    }

    void Text::set_overflow(TextOverflow overflow) {
        style_.overflow = overflow;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::overflow() const -> TextOverflow {
        return style_.overflow;
    }

    void Text::set_max_lines(int lines) {
        style_.max_lines = std::max(1, lines);
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::max_lines() const -> int {
        return style_.max_lines;
    }

    auto Text::measured_text_width() const -> float {
        float width = 0.0F;
        for (const auto& line: layout_.lines) {
            width = std::max(width, line.size.get_width());
        }
        return width;
    }

    auto Text::laid_out_font_size() const -> float {
        return layout_.font_size;
    }

    auto Text::layout_result() const -> const TextLayoutResult& {
        return layout_;
    }

    void Text::draw_at(render::DrawContext& ctx, foundation::NanPoint position) {
        if (layout_.lines.empty() || style_.color.alpha() <= 0.0F) {
            return;
        }

        const auto color = style_.color.with_alpha(style_.color.alpha() * ctx.opacity());
        float y = position.get_y();
        for (const auto& line: layout_.lines) {
            if (!line.visible_text.empty()) {
                ctx.device().draw_text(
                    line.visible_text,
                    foundation::NanPoint(position.get_x(), y),
                    layout_.font_size,
                    color
                );
            }
            y += line.size.get_height();
        }
    }

    void Text::on_draw(render::DrawContext& ctx) {
        const auto pos = ctx.world_transform().transform_point(foundation::NanPoint::zero());
        draw_at(ctx, pos);
    }

    auto Text::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        update_metrics(constraints);
        return size();
    }

    void Text::update_metrics(scene::LayoutConstraints constraints) {
        layout_ = layout_text(TextLayoutInput {
            .text = text_,
            .style = style_,
            .constraints = constraints,
        });
        set_size(layout_.size);
    }

    auto Text::layout_text(TextLayoutInput input) const -> TextLayoutResult {
        TextLayoutResult result;
        result.font_size = input.style.font_size;

        const bool has_width_constraint = std::isfinite(input.constraints.max_width);
        const float line_height = result.font_size * 1.2F;
        result.baseline = result.font_size;

        if (input.style.overflow == TextOverflow::wrap) {
            const float char_width = std::max(1.0F, result.font_size * 0.56F);
            const auto chars_per_line = has_width_constraint
                ? std::max<std::size_t>(
                      1,
                      static_cast<std::size_t>(std::max(0.0F, input.constraints.max_width) / char_width)
                  )
                : std::max<std::size_t>(1, foundation::utf8::codepoint_count(input.text));
            const auto max_lines = static_cast<std::size_t>(std::max(1, input.style.max_lines));

            std::size_t offset = 0;
            bool pending_empty_line = input.text.empty();
            while ((offset < input.text.size() || pending_empty_line) && result.lines.size() < max_lines) {
                const auto line_offset = offset;
                std::size_t line_end = offset;
                std::size_t codepoints = 0;
                pending_empty_line = false;

                while (line_end < input.text.size() && input.text[line_end] != '\n'
                       && codepoints < chars_per_line)
                {
                    line_end = foundation::utf8::next_boundary(input.text, line_end);
                    ++codepoints;
                }

                const auto line_text = input.text.substr(line_offset, line_end - line_offset);
                const float line_width = has_width_constraint
                    ? std::min(text_width(line_text, result.font_size), input.constraints.max_width)
                    : text_width(line_text, result.font_size);
                result.lines.push_back(TextLayoutLine {
                    .text_offset = line_offset,
                    .text_length = line_end - line_offset,
                    .visible_text = std::string(line_text),
                    .size = foundation::NanSize(line_width, line_height),
                    .baseline = result.baseline,
                });

                offset = line_end;
                if (offset < input.text.size() && input.text[offset] == '\n') {
                    ++offset;
                    pending_empty_line = offset == input.text.size();
                }
            }

            result.overflowed = offset < input.text.size() || pending_empty_line;
            float width = 0.0F;
            for (const auto& line: result.lines) {
                width = std::max(width, line.size.get_width());
            }
            result.size = foundation::NanSize(
                width,
                line_height * static_cast<float>(result.lines.size())
            );
            return result;
        }

        std::string laid_out_text {input.text};
        std::size_t source_text_length = input.text.size();
        const float unconstrained_width = text_width(input.text, input.style.font_size);

        if (has_width_constraint && input.constraints.max_width >= 0.0F
            && unconstrained_width > input.constraints.max_width + text_layout_epsilon)
        {
            result.overflowed = true;
            const float char_width = std::max(1.0F, input.style.font_size * 0.56F);
            if (input.style.overflow == TextOverflow::scale && unconstrained_width > 0.0F) {
                result.font_size = std::max(1.0F, input.style.font_size * (input.constraints.max_width / unconstrained_width));
            } else if (input.style.overflow == TextOverflow::ellipsis) {
                constexpr std::string_view dots = "...";
                const float dots_width = text_width(dots, input.style.font_size);
                const float budget = std::max(0.0F, input.constraints.max_width - dots_width);
                const auto keep = static_cast<std::size_t>(std::floor(budget / char_width));
                source_text_length = foundation::utf8::byte_offset_for_codepoints(input.text, keep);
                laid_out_text = std::string(input.text.substr(0, source_text_length)) + std::string(dots);
            } else if (input.style.overflow == TextOverflow::clip) {
                const auto keep = static_cast<std::size_t>(std::floor(input.constraints.max_width / char_width));
                source_text_length = foundation::utf8::byte_offset_for_codepoints(input.text, keep);
                laid_out_text = std::string(input.text.substr(0, source_text_length));
            }
        }

        const float width = has_width_constraint
            ? std::min(text_width(laid_out_text, result.font_size), input.constraints.max_width)
            : text_width(laid_out_text, result.font_size);

        result.size = foundation::NanSize(width, line_height);
        result.lines.push_back(TextLayoutLine {
            .text_offset = 0,
            .text_length = source_text_length,
            .visible_text = std::move(laid_out_text),
            .size = foundation::NanSize(width, line_height),
            .baseline = result.baseline,
        });
        return result;
    }

    auto Text::text_width(std::string_view text, float font_size) const -> float {
        return static_cast<float>(foundation::utf8::codepoint_count(text)) * font_size * 0.56F;
    }

} // namespace nandina::widget::primitives
