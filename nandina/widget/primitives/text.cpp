//
// widget/primitives/text — minimal text drawing primitive.
//

#include "text.hpp"
#include "../../render/draw_context.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nandina::widget::primitives
{

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

    void Text::set_color(foundation::NanColor color) {
        color_ = color;
    }

    auto Text::color() const -> foundation::NanColor {
        return color_;
    }

    void Text::set_font_size(float size) {
        font_size_ = size;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::font_size() const -> float {
        return font_size_;
    }

    void Text::set_overflow(TextOverflow overflow) {
        overflow_ = overflow;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::overflow() const -> TextOverflow {
        return overflow_;
    }

    void Text::set_max_lines(int lines) {
        max_lines_ = std::max(1, lines);
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::max_lines() const -> int {
        return max_lines_;
    }

    void Text::on_draw(render::DrawContext& ctx) {
        if (laid_out_text_.empty() || color_.alpha() <= 0.0F) {
            return;
        }

        const auto pos = ctx.world_transform().transform_point(foundation::NanPoint::zero());
        ctx.device()
            .draw_text(laid_out_text_, pos, laid_out_font_size_, color_.with_alpha(color_.alpha() * ctx.opacity()));
    }

    auto Text::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        update_metrics(constraints);
        return size();
    }

    void Text::update_metrics(scene::LayoutConstraints constraints) {
        laid_out_font_size_ = font_size_;
        laid_out_text_ = text_;

        const bool has_width_constraint = std::isfinite(constraints.max_width);
        const float unconstrained_width = text_width(text_, font_size_);

        if (has_width_constraint && constraints.max_width >= 0.0F && unconstrained_width > constraints.max_width) {
            const float char_width = std::max(1.0F, font_size_ * 0.56F);
            if (overflow_ == TextOverflow::scale && unconstrained_width > 0.0F) {
                laid_out_font_size_ = std::max(1.0F, font_size_ * (constraints.max_width / unconstrained_width));
            } else if (overflow_ == TextOverflow::ellipsis) {
                constexpr std::string_view dots = "...";
                const float dots_width = text_width(dots, font_size_);
                const float budget = std::max(0.0F, constraints.max_width - dots_width);
                const auto keep = static_cast<std::size_t>(std::floor(budget / char_width));
                laid_out_text_ = text_.substr(0, std::min(keep, text_.size())) + std::string(dots);
            } else if (overflow_ == TextOverflow::clip) {
                const auto keep = static_cast<std::size_t>(std::floor(constraints.max_width / char_width));
                laid_out_text_ = text_.substr(0, std::min(keep, text_.size()));
            } else if (overflow_ == TextOverflow::wrap) {
                const auto chars_per_line = std::max<std::size_t>(1, static_cast<std::size_t>(constraints.max_width / char_width));
                const auto line_count = std::min<std::size_t>(
                    static_cast<std::size_t>(max_lines_),
                    (text_.size() + chars_per_line - 1) / chars_per_line
                );
                laid_out_text_ = text_.substr(0, std::min(text_.size(), chars_per_line * line_count));
            }
        }

        const float width = has_width_constraint
            ? std::min(text_width(laid_out_text_, laid_out_font_size_), constraints.max_width)
            : text_width(laid_out_text_, laid_out_font_size_);
        const float line_count = overflow_ == TextOverflow::wrap && has_width_constraint
            ? static_cast<float>(std::max(1, max_lines_))
            : 1.0F;
        const float height = laid_out_font_size_ * 1.2F * line_count;
        set_size(foundation::NanSize(width, height));
    }

    auto Text::text_width(std::string_view text, float font_size) const -> float {
        return static_cast<float>(text.size()) * font_size * 0.56F;
    }

} // namespace nandina::widget::primitives
