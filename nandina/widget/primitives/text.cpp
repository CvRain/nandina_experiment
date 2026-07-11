//
// widget/primitives/text — minimal text drawing primitive.
//

#include "text.hpp"
#include "../../render/draw_context.hpp"

#include <algorithm>
#include <utility>

namespace nandina::widget::primitives
{

    Text::Text(std::string text, const ITextLayoutBackend& backend):
        text_(std::move(text)),
        backend_(&backend) {
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

    void Text::set_layout_backend(const ITextLayoutBackend& backend) {
        backend_ = &backend;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::layout_backend() const -> const ITextLayoutBackend& {
        return *backend_;
    }

    void Text::set_layout_renderer(ITextLayoutRenderer* renderer) {
        renderer_ = renderer;
    }

    auto Text::layout_renderer() const -> ITextLayoutRenderer* {
        return renderer_;
    }

    void Text::draw_at(render::DrawContext& ctx, foundation::NanPoint position) {
        if (layout_.lines.empty() || style_.color.alpha() <= 0.0F) {
            return;
        }

        const auto color = style_.color.with_alpha(style_.color.alpha() * ctx.opacity());
        if (renderer_ != nullptr) {
            renderer_->draw(layout_, ctx, position, color);
            return;
        }

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
        layout_ = backend_->layout(TextLayoutInput {
            .text = text_,
            .style = style_,
            .constraints = constraints,
        });
        set_size(layout_.size);
    }

} // namespace nandina::widget::primitives
