//
// widget/primitives/surface — visual surface primitive.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_SURFACE_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_SURFACE_HPP

#include "../../foundation/nandina_color.hpp"
#include "../../scene/control.hpp"
#include "../../theme/nan_style.hpp"

#include <optional>

namespace nandina::widget::primitives
{

    class Surface: public scene::NanControl {
    public:
        Surface() = default;
        explicit Surface(foundation::NanSize size): scene::NanControl(size) {}

        void set_fill(foundation::NanColor color);
        void set_fill(theme::ThemeColor color);
        void clear_fill();
        [[nodiscard]] auto fill() const -> const std::optional<foundation::NanColor>&;

        void set_radius(float radius);
        [[nodiscard]] auto radius() const -> float;

        void set_border(foundation::NanColor color, float width);
        void set_border(theme::ThemeColor color, float width);
        void clear_border();
        [[nodiscard]] auto border_color() const -> const std::optional<foundation::NanColor>&;
        [[nodiscard]] auto border_width() const -> float;

        auto on_draw(render::DrawContext& ctx) -> void override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

    private:
        std::optional<foundation::NanColor> fill_;
        std::optional<theme::ThemeColor> theme_fill_;
        std::optional<foundation::NanColor> border_color_;
        std::optional<theme::ThemeColor> theme_border_;
        float radius_ = 0.0F;
        float border_width_ = 0.0F;
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_SURFACE_HPP
