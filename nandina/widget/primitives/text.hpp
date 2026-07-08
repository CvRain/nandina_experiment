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

    enum class TextOverflow {
        clip,
        ellipsis,
        wrap,
        scale,
    };

    class Text: public scene::NanControl {
    public:
        explicit Text(std::string text = {});

        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;

        void set_color(foundation::NanColor color);
        [[nodiscard]] auto color() const -> foundation::NanColor;

        void set_font_size(float size);
        [[nodiscard]] auto font_size() const -> float;

        void set_overflow(TextOverflow overflow);
        [[nodiscard]] auto overflow() const -> TextOverflow;

        void set_max_lines(int lines);
        [[nodiscard]] auto max_lines() const -> int;

        auto on_draw(render::DrawContext& ctx) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize override;

    private:
        void update_metrics(scene::LayoutConstraints constraints = scene::LayoutConstraints::loose());
        [[nodiscard]] auto text_width(std::string_view text, float font_size) const -> float;

        std::string text_;
        std::string laid_out_text_;
        foundation::NanColor color_ = foundation::NanColor::from(
            foundation::NanHexRgb {.red = 255, .green = 255, .blue = 255, .alpha = 255}
        );
        float font_size_ = 16.0F;
        float laid_out_font_size_ = 16.0F;
        TextOverflow overflow_ = TextOverflow::ellipsis;
        int max_lines_ = 1;
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP
