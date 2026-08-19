//
// widget/primitives/box_presentation - instance presentation overrides for a box part.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_BOX_PRESENTATION_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_BOX_PRESENTATION_HPP

#include "../../scene/control.hpp"
#include "../../theme/design_system.hpp"
#include "../visual_property.hpp"

#include <cmath>
#include <optional>
#include <stdexcept>
#include <utility>

namespace nandina::widget::primitives
{
    class BoxPresentation {
    public:
        explicit BoxPresentation(scene::NanControl& owner) noexcept: owner_(&owner) {}

        class FillProperty {
        public:
            explicit FillProperty(BoxPresentation& box) noexcept: box_(&box) {}
            void set(foundation::NanColor color) { box_->set_fill(std::move(color)); }

        private:
            BoxPresentation* box_;
        };

        class BorderColorProperty {
        public:
            explicit BorderColorProperty(BoxPresentation& box) noexcept: box_(&box) {}
            void set(foundation::NanColor color) { box_->set_border_color(std::move(color)); }

        private:
            BoxPresentation* box_;
        };

        class BorderWidthProperty {
        public:
            explicit BorderWidthProperty(BoxPresentation& box) noexcept: box_(&box) {}
            void set(float width) { box_->set_border_width(width); }

        private:
            BoxPresentation* box_;
        };

        class RadiusProperty {
        public:
            explicit RadiusProperty(BoxPresentation& box) noexcept: box_(&box) {}
            void set(float radius) { box_->set_radius(radius); }

        private:
            BoxPresentation* box_;
        };

        [[nodiscard]] auto property(visual::fill_t) noexcept -> FillProperty {
            return FillProperty {*this};
        }

        [[nodiscard]] auto property(visual::border_color_t) noexcept -> BorderColorProperty {
            return BorderColorProperty {*this};
        }

        [[nodiscard]] auto property(visual::border_width_t) noexcept -> BorderWidthProperty {
            return BorderWidthProperty {*this};
        }

        [[nodiscard]] auto property(visual::radius_t) noexcept -> RadiusProperty {
            return RadiusProperty {*this};
        }

        void apply(theme::ResolvedBoxStyle& style) const {
            if (fill_) {
                style.fill = *fill_;
            }
            if (border_color_) {
                style.border = *border_color_;
            }
            if (border_width_) {
                style.border_width = *border_width_;
            }
            if (radius_) {
                style.radius = *radius_;
            }
        }

    private:
        static void require_non_negative(float value, const char* field) {
            if (!std::isfinite(value) || value < 0.0F) {
                throw std::invalid_argument(field);
            }
        }

        void set_fill(foundation::NanColor color) {
            fill_ = std::move(color);
            owner_->mark_dirty(scene::DirtyFlags::paint);
        }

        void set_border_color(foundation::NanColor color) {
            border_color_ = std::move(color);
            owner_->mark_dirty(scene::DirtyFlags::paint);
        }

        void set_border_width(const float width) {
            require_non_negative(width, "box presentation border width must be finite and non-negative");
            border_width_ = width;
            owner_->mark_dirty(scene::DirtyFlags::paint);
        }

        void set_radius(const float radius) {
            require_non_negative(radius, "box presentation radius must be finite and non-negative");
            radius_ = radius;
            owner_->mark_dirty(scene::DirtyFlags::paint);
        }

        scene::NanControl* owner_;
        std::optional<foundation::NanColor> fill_;
        std::optional<foundation::NanColor> border_color_;
        std::optional<float> border_width_;
        std::optional<float> radius_;
    };
} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_BOX_PRESENTATION_HPP
