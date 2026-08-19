//
// widget/primitives/text_presentation - instance presentation overrides for a text part.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_PRESENTATION_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_PRESENTATION_HPP

#include "../../scene/control.hpp"
#include "../../theme/design_system.hpp"
#include "../visual_property.hpp"
#include "text_layout.hpp"

#include <cmath>
#include <optional>
#include <stdexcept>
#include <utility>

namespace nandina::widget::primitives
{
    class TextPresentation {
    public:
        explicit TextPresentation(scene::NanControl& owner) noexcept: owner_(&owner) {}

        class ColorProperty {
        public:
            explicit ColorProperty(TextPresentation& text) noexcept: text_(&text) {}
            void set(foundation::NanColor color) {
                text_->set_color(std::move(color));
            }

        private:
            TextPresentation* text_;
        };

        class FontSizeProperty {
        public:
            explicit FontSizeProperty(TextPresentation& text) noexcept: text_(&text) {}
            void set(float size) {
                text_->set_font_size(size);
            }

        private:
            TextPresentation* text_;
        };

        [[nodiscard]] auto property(visual::color_t) noexcept -> ColorProperty {
            return ColorProperty {*this};
        }

        [[nodiscard]] auto property(visual::font_size_t) noexcept -> FontSizeProperty {
            return FontSizeProperty {*this};
        }

        void apply(theme::ResolvedTypeStyle& style) const {
            if (color_) {
                style.color = *color_;
            }
            if (font_size_) {
                style.font_size = *font_size_;
            }
        }

        void apply(TextStyle& style) const {
            if (color_) {
                style.color = *color_;
            }
            if (font_size_) {
                style.font_size = *font_size_;
            }
        }

        void clear_font_size() noexcept {
            if (!font_size_) {
                return;
            }
            font_size_.reset();
            owner_->mark_layout_dirty();
        }

    private:
        void set_color(foundation::NanColor color) {
            color_ = std::move(color);
            owner_->mark_dirty(scene::DirtyFlags::paint);
        }

        void set_font_size(const float size) {
            if (!std::isfinite(size) || size <= 0.0F) {
                throw std::invalid_argument(
                    "text presentation font size must be finite and positive"
                );
            }
            font_size_ = size;
            owner_->mark_layout_dirty();
        }

        scene::NanControl* owner_;
        std::optional<foundation::NanColor> color_;
        std::optional<float> font_size_;
    };
} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_PRESENTATION_HPP
