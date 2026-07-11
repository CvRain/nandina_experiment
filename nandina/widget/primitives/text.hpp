//
// widget/primitives/text — minimal text drawing primitive.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP

#include "text_layout_backend.hpp"

#include <string>
#include <string_view>

namespace nandina::widget::primitives
{

    class Text: public scene::NanControl {
    public:
        explicit Text(
            std::string text = {},
            const ITextLayoutBackend& backend = deterministic_text_layout_backend()
        );

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

        /// The referenced backend must outlive this Text instance.
        void set_layout_backend(const ITextLayoutBackend& backend);
        [[nodiscard]] auto layout_backend() const -> const ITextLayoutBackend&;

        void draw_at(render::DrawContext& ctx, foundation::NanPoint position);
        auto on_draw(render::DrawContext& ctx) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize override;

    private:
        void update_metrics(scene::LayoutConstraints constraints = scene::LayoutConstraints::loose());

        std::string text_;
        TextStyle style_ {};
        TextLayoutResult layout_ {};
        const ITextLayoutBackend* backend_ = nullptr;
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_HPP
