//
// widget/primitives/text — minimal text drawing primitive.
//

#include "text.hpp"
#include "../../render/draw_context.hpp"

#include <utility>

namespace nandina::widget::primitives
{

    Text::Text(std::string text): text_(std::move(text)) {
        update_metrics();
    }

    void Text::set_text(std::string text) {
        text_ = std::move(text);
        update_metrics();
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
        update_metrics();
    }

    auto Text::font_size() const -> float {
        return font_size_;
    }

    void Text::on_draw(render::DrawContext& ctx) {
        if (text_.empty() || color_.alpha() <= 0.0F) {
            return;
        }

        const auto pos = ctx.world_transform().transform_point(foundation::NanPoint::zero());
        ctx.device().draw_text(
            text_,
            pos,
            font_size_,
            color_.with_alpha(color_.alpha() * ctx.opacity())
        );
    }

    void Text::update_metrics() {
        const float width = static_cast<float>(text_.size()) * font_size_ * 0.56F;
        const float height = font_size_ * 1.2F;
        set_size(foundation::NanSize(width, height));
    }

} // namespace nandina::widget::primitives
