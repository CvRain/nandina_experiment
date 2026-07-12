//
// widget/primitives/editable_text — focused text editing primitive.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_EDITABLE_TEXT_HPP
#define NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_EDITABLE_TEXT_HPP

#include "../../scene/control.hpp"
#include "text.hpp"

#include <functional>
#include <string>
#include <string_view>

namespace nandina::widget::primitives
{

    class EditableText: public scene::NanControl {
    public:
        explicit EditableText(std::string value = {});

        void set_value(std::string value);
        [[nodiscard]] auto value() const -> std::string_view;

        void set_style(TextStyle style);
        [[nodiscard]] auto style() const -> const TextStyle&;

        void set_caret(std::size_t offset);
        [[nodiscard]] auto caret() const -> std::size_t;

        void set_on_change(std::function<void(std::string_view)> callback);

        [[nodiscard]] auto text_node() -> Text&;
        [[nodiscard]] auto text_node() const -> const Text&;

        void set_text_pipeline(TextPipeline pipeline);
        [[nodiscard]] auto text_pipeline() const -> TextPipeline;

        void draw_at(render::DrawContext& ctx, foundation::NanPoint position);

        [[nodiscard]] auto is_focusable() const -> bool override;
        auto on_input(scene::InputEvent& event) -> bool override;
        auto on_draw(render::DrawContext& ctx) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize override;

    private:
        void insert_text(std::string_view text);
        void erase_before_caret();
        void sync_text();
        void emit_change();
        [[nodiscard]] auto caret_x() const -> float;

        std::string value_;
        std::size_t caret_ = 0;
        bool focused_ = false;
        Text text_;
        std::function<void(std::string_view)> on_change_;
    };

} // namespace nandina::widget::primitives

#endif // NANDINA_EXPERIMENT_WIDGET_PRIMITIVES_EDITABLE_TEXT_HPP
