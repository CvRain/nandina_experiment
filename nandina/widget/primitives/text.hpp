//
// widget/primitives/text — minimal text drawing primitive.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP

#include "../../foundation/nandina_color.hpp"
#include "../../scene/control.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace nandina::widget::primitives
{

    enum class TextOverflow {
        clip,
        ellipsis,
        wrap,
        scale,
    };

    struct TextStyle {
        foundation::NanColor color = foundation::NanColor::from(
            foundation::NanHexRgb {.red = 255, .green = 255, .blue = 255, .alpha = 255}
        );
        float font_size = 16.0F;
        TextOverflow overflow = TextOverflow::ellipsis;
        int max_lines = 1;
    };

    struct TextLayoutInput {
        std::string_view text;
        TextStyle style;
        scene::LayoutConstraints constraints = scene::LayoutConstraints::loose();
    };

    struct TextLayoutLine {
        std::size_t text_offset = 0;
        std::size_t text_length = 0;
        std::string visible_text;
        foundation::NanSize size {};
        float baseline = 0.0F;
    };

    struct TextLayoutResult {
        foundation::NanSize size {};
        std::vector<TextLayoutLine> lines;
        float font_size = 16.0F;
        float baseline = 0.0F;
        bool overflowed = false;
    };

    class Text: public scene::NanControl {
    public:
        explicit Text(std::string text = {});

        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;

        void set_style(TextStyle style);
        [[nodiscard]] auto style() const -> const TextStyle&;

        void set_color(foundation::NanColor color);
        [[nodiscard]] auto color() const -> foundation::NanColor;

        void set_font_size(float size);
        [[nodiscard]] auto font_size() const -> float;

        void set_overflow(TextOverflow overflow);
        [[nodiscard]] auto overflow() const -> TextOverflow;

        void set_max_lines(int lines);
        [[nodiscard]] auto max_lines() const -> int;

        [[nodiscard]] auto measured_text_width() const -> float;
        [[nodiscard]] auto laid_out_font_size() const -> float;
        [[nodiscard]] auto layout_result() const -> const TextLayoutResult&;

        void draw_at(render::DrawContext& ctx, foundation::NanPoint position);
        auto on_draw(render::DrawContext& ctx) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize override;

    private:
        void update_metrics(scene::LayoutConstraints constraints = scene::LayoutConstraints::loose());
        [[nodiscard]] auto layout_text(TextLayoutInput input) const -> TextLayoutResult;
        [[nodiscard]] auto text_width(std::string_view text, float font_size) const -> float;

        std::string text_;
        TextStyle style_ {};
        TextLayoutResult layout_ {};
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP
