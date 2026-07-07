//
// widget/primitives/text — minimal text drawing primitive.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP

#include "../../foundation/nandina_color.hpp"
#include "../../scene/control.hpp"

#include <string>
#include <string_view>

namespace nandina::widget::primitives
{

    class Text: public scene::NanControl {
    public:
        explicit Text(std::string text = {});

        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;

        void set_color(foundation::NanColor color);
        [[nodiscard]] auto color() const -> foundation::NanColor;

        void set_font_size(float size);
        [[nodiscard]] auto font_size() const -> float;

        auto on_draw(render::DrawContext& ctx) -> void override;

    private:
        void update_metrics();

        std::string text_;
        foundation::NanColor color_ = foundation::NanColor::from(
            foundation::NanHexRgb {.red = 255, .green = 255, .blue = 255, .alpha = 255}
        );
        float font_size_ = 16.0F;
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP
