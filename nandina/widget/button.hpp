//
// widget/button — first semantic control built from tone/treatment tokens.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BUTTON_HPP
#define NANDINA_EXPERIMENT_WIDGET_BUTTON_HPP

#include "../theme/button_style.hpp"
#include "../theme/design_system.hpp"
#include "primitives/pressable.hpp"
#include "primitives/text.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace nandina::widget
{

    class Button: public primitives::Pressable {
    public:
        explicit Button(std::string text, theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto
        create(std::string text, theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Button>;

        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;
        [[nodiscard]] auto text_node() -> primitives::Text&;
        [[nodiscard]] auto text_node() const -> const primitives::Text&;

        void set_text_pipeline(primitives::TextPipeline pipeline);
        [[nodiscard]] auto text_pipeline() const -> primitives::TextPipeline;
        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;
        void set_font(text::FontRequest request);
        void set_font_family(resource::ResourceKey family);
        void set_font_weight(int weight);
        void set_font_slant(text::FontSlant slant);

        void set_text_overflow(primitives::TextOverflow overflow);
        [[nodiscard]] auto text_overflow() const -> primitives::TextOverflow;

        /// 高级接口：以完整 NanTheme 覆盖控件主题（等价整份快照覆盖，不再跟随系统切换）。
        void set_theme(theme::NanTheme theme);
        /// 当前生效主题视图（tokens + 当前外观 palette），遗留读取兼容。
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;

        void set_tone(theme::ButtonTone tone);
        [[nodiscard]] auto tone() const -> theme::ButtonTone;

        void set_treatment(theme::ButtonTreatment treatment);
        [[nodiscard]] auto treatment() const -> theme::ButtonTreatment;

        void set_button_size(theme::ButtonSize size);
        [[nodiscard]] auto button_size() const -> theme::ButtonSize;

        [[nodiscard]] auto visual_state() const -> theme::ButtonVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ResolvedButtonStyle;

        auto on_draw(render::DrawContext& ctx) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_pressable_state_changed() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;
        auto on_semantics_action(const semantics::ActionRequest& request) -> bool override;

    private:
        void apply_metrics();
        void apply_text_style(theme::ButtonVisualState state);

        primitives::Text text_;
        /// 解析用的设计系统快照（树内 = ThemeManager 的有效快照；detached = 回退）。
        std::shared_ptr<const theme::DesignSystem> system_;
        theme::ColorAppearance appearance_ = theme::ColorAppearance::light;
        /// theme_ref() 兼容视图（tokens + 当前外观 palette）。
        theme::NanTheme theme_view_;
        /// set_theme(NanTheme) 整份覆盖后不再跟随系统切换。
        bool system_explicit_ = false;
        bool font_explicit_ = false;
        theme::ButtonTone tone_ = theme::ButtonTone::primary;
        theme::ButtonTreatment treatment_ = theme::ButtonTreatment::filled;
        theme::ButtonSize size_ = theme::ButtonSize::medium;
        primitives::TextOverflow text_overflow_ = primitives::TextOverflow::ellipsis;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_BUTTON_HPP
