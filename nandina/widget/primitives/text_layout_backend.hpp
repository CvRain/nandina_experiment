//
// widget/primitives/text_layout_backend — pluggable text layout contract.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_BACKEND_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_BACKEND_HPP

#include "text_layout.hpp"

namespace nandina::widget::primitives
{

    class ITextLayoutBackend {
    public:
        virtual ~ITextLayoutBackend() = default;

        [[nodiscard]] virtual auto layout(TextLayoutInput input) const -> TextLayoutResult = 0;
    };

    /// Process-lifetime fallback used until a shaping backend is installed.
    [[nodiscard]] auto deterministic_text_layout_backend() -> const ITextLayoutBackend&;

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_TEXT_LAYOUT_BACKEND_HPP
