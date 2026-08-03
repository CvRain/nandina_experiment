//
// Settings example - UI composition and component logic only.
//

#include "settings_example.hpp"

#include "foundation/geometry.hpp"
#include "theme/design_system.hpp"
#include "theme/nan_style.hpp"
#include "widget/layout.hpp"

#include <format>
#include <memory>
#include <string>

namespace nandina::examples::settings
{
    namespace
    {
        [[nodiscard]] auto preference_summary(
            const std::string& profile,
            const bool notifications,
            const bool diagnostics,
            const bool reduced_motion,
            const float interface_scale
        ) -> std::string {
            auto result = profile.empty() ? std::string("Unnamed profile") : profile;
            result += notifications ? " · notifications on" : " · notifications off";
            result += diagnostics ? " · diagnostics on" : " · diagnostics off";
            result += reduced_motion ? " · reduced motion" : " · full motion";
            result += std::format(" · {:.0f}% scale", interface_scale * 100.0F);
            return result;
        }
    } // namespace

    auto build(widget::BuildContext& ui) -> std::shared_ptr<scene::NanNode2D> {
        // 品牌设计系统：默认快照拷贝 → 改 primary 与圆角 → 原子应用（单次 revision）。
        auto design = theme::default_design_system();
        design.light.primary = theme::nan_color(0.62F, 0.16F, 255.0F);
        design.dark.primary = theme::nan_color(0.70F, 0.14F, 255.0F);
        design.tokens.radius.md = 10.0F;
        ui.theme_manager().apply(std::make_shared<const theme::DesignSystem>(std::move(design)));

        auto& profile = ui.signal<std::string>("Nandina developer");
        auto& notifications = ui.signal<bool>(true);
        auto& diagnostics = ui.signal<bool>(false);
        auto& reduced_motion = ui.signal<bool>(false);
        auto& interface_scale = ui.signal<float>(1.0F);
        auto& status = ui.signal<std::string>("Changes are applied locally");
        auto& summary = ui.computed([&] {
            return preference_summary(
                profile.get(),
                notifications.get(),
                diagnostics.get(),
                reduced_motion.get(),
                interface_scale.get()
            );
        });
        auto& scale_label = ui.computed([&] {
            return std::format("Interface scale · {:.0f}%", interface_scale.get() * 100.0F);
        });

        auto profile_field = ui.text_field(profile, "Profile name").autofocus().build();
        auto diagnostics_note = ui.when(diagnostics, [](widget::BuildContext branch) {
            return branch.label("Anonymous diagnostics help improve rendering stability")
                .color_token(theme::ColorToken::on_surface_variant);
        });

        auto preferences =
            ui.column()
                .gap(6.0F)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(
                    ui.checkbox(notifications, "Desktop notifications"),
                    ui.checkbox(diagnostics, "Send anonymous diagnostics"),
                    diagnostics_note,
                    ui.checkbox(reduced_motion, "Reduce interface motion"),
                    ui.label(scale_label).color_token(theme::ColorToken::on_surface_variant),
                    ui.slider(interface_scale, "Interface scale", 0.75F, 1.5F, 0.05F)
                )
                .build();

        auto save = ui.button("Save preferences").tone(theme::ButtonTone::primary).on_click([&] {
            status.set("Preferences saved for " + profile.peek());
        });
        auto reset = ui.button("Reset").treatment(theme::ButtonTreatment::outlined).on_click([&] {
            profile.set("Nandina developer");
            notifications.set(true);
            diagnostics.set(false);
            reduced_motion.set(false);
            interface_scale.set(1.0F);
            status.set("Preferences reset");
        });

        auto actions = ui.row()
                           .gap(8.0F)
                           .cross_alignment(widget::LayoutAlignment::center)
                           .children(save, reset)
                           .build();
        auto content = ui.column()
                           .gap(12.0F)
                           .cross_alignment(widget::LayoutAlignment::stretch)
                           .children(
                               ui.label("Nandina Settings").font_size(28.0F),
                               ui.label("A compact application authored from components and state")
                                   .color_token(theme::ColorToken::on_surface_variant),
                               ui.label("Profile").font_size(18.0F),
                               profile_field,
                               ui.label("Preferences").font_size(18.0F),
                               preferences,
                               ui.label(summary).color_token(theme::ColorToken::on_surface_variant),
                               actions,
                               ui.label(status).color_token(theme::ColorToken::primary)
                           );
        return ui.padding(foundation::NanInsets::all(20.0F)).child(content).build();
    }
} // namespace nandina::examples::settings
