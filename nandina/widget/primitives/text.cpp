//
// widget/primitives/text — minimal text drawing primitive.
//

#include "text.hpp"
#include "../../render/draw_context.hpp"
#include "../../text/font_pipeline.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nandina::widget::primitives
{

    Text::Text(std::string text, const ITextLayoutBackend& backend):
        text_(std::move(text), [this](const std::string& value) { apply_text(value); }),
        backend_(&backend) {
        update_metrics();
    }

    void Text::set_text(std::string text) {
        (void)text_.set(std::move(text));
    }

    void Text::apply_text(const std::string&) {
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::text() const -> std::string_view {
        return text_.get();
    }

    auto Text::text_property() -> reactive::Property<std::string>& {
        return text_;
    }

    auto Text::text_property() const -> reactive::ReadProperty<std::string> {
        return text_.as_readonly();
    }

    void Text::set_style(TextStyle style) {
        style.max_lines = std::max(1, style.max_lines);
        if (style.font.weight < 1 || style.font.weight > 1000) {
            throw std::invalid_argument("Text font weight must be between 1 and 1000");
        }
        const bool font_changed = style_.font != style.font;
        style_ = style;
        if (font_changed) {
            resolve_font();
        }
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

    void Text::set_font(text::FontRequest request) {
        if (request.weight < 1 || request.weight > 1000) {
            throw std::invalid_argument("Text font weight must be between 1 and 1000");
        }
        if (style_.font == request) {
            return;
        }
        style_.font = std::move(request);
        resolve_font();
    }

    void Text::set_font_family(resource::ResourceKey family) {
        auto request = style_.font;
        request.family = std::move(family);
        set_font(std::move(request));
    }

    void Text::clear_font_family() {
        auto request = style_.font;
        request.family.reset();
        set_font(std::move(request));
    }

    void Text::set_font_weight(const int weight) {
        auto request = style_.font;
        request.weight = weight;
        set_font(std::move(request));
    }

    void Text::set_font_slant(const text::FontSlant slant) {
        auto request = style_.font;
        request.slant = slant;
        set_font(std::move(request));
    }

    auto Text::font() const -> const text::FontRequest& {
        return style_.font;
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

    void Text::set_text_pipeline(TextPipeline pipeline) {
        if (pipeline.backend == nullptr) {
            throw std::invalid_argument("TextPipeline requires a layout backend");
        }
        backend_ = pipeline.backend;
        renderer_ = pipeline.renderer;
        pipeline_explicit_ = true;
        resolved_font_pipeline_.reset();
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::text_pipeline() const -> TextPipeline {
        return {.backend = backend_, .renderer = renderer_};
    }

    void Text::apply_default_text_pipeline(const TextPipeline& pipeline) {
        if (pipeline_explicit_) {
            return;
        }
        backend_ = pipeline.backend;
        renderer_ = pipeline.renderer;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    void Text::apply_font_context(text::FontPipelineCache& context) {
        font_context_ = &context;
        resolve_font();
    }

    void Text::set_layout_backend(const ITextLayoutBackend& backend) {
        backend_ = &backend;
        pipeline_explicit_ = true;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

    auto Text::layout_backend() const -> const ITextLayoutBackend& {
        return *backend_;
    }

    void Text::set_layout_renderer(ITextLayoutRenderer* renderer) {
        renderer_ = renderer;
        pipeline_explicit_ = true;
    }

    auto Text::layout_renderer() const -> ITextLayoutRenderer* {
        return renderer_;
    }

    void Text::draw_at(render::DrawContext& ctx, foundation::NanPoint position) {
        if (layout_.lines.empty() || style_.color.alpha() <= 0.0F) {
            return;
        }

        auto clip = style_.overflow == TextOverflow::clip
            ? ctx.clip().push(
                  foundation::NanRect::from_xywh(
                      position.get_x(),
                      position.get_y(),
                      layout_.size.get_width(),
                      layout_.size.get_height()
                  )
              )
            : render::ClipStack::Guard {nullptr, false};

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
        layout_ = backend_->layout(
            TextLayoutInput {
                .text = text_.get(),
                .style = style_,
                .constraints = constraints,
            }
        );
        set_size(layout_.size);
    }

    void Text::resolve_font() {
        if (pipeline_explicit_ || font_context_ == nullptr) {
            return;
        }
        auto pipeline = font_context_->get(style_.font);
        if (!pipeline) {
            throw std::runtime_error("Text cannot resolve font: " + pipeline.error().message);
        }
        resolved_font_pipeline_ = std::move(*pipeline);
        const auto resolved = resolved_font_pipeline_->pipeline();
        backend_ = resolved.backend;
        renderer_ = resolved.renderer;
        mark_layout_dirty();
        update_metrics(last_layout_constraints());
    }

} // namespace nandina::widget::primitives
