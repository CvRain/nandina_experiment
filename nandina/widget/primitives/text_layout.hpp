//
// widget/primitives/text_layout — backend-neutral text layout values.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_HPP

#include "../../foundation/nandina_color.hpp"
#include "../../scene/control.hpp"

#include <cstddef>
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

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_HPP
