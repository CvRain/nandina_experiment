//
// Settings example - UI composition and component logic only.
//

#include "settings_example.hpp"

#include "foundation/geometry.hpp"
#include "theme/nan_style.hpp"
#include "widget/layout.hpp"

#include <string>

namespace nandina::examples::settings
{
    namespace
    {
        [[nodiscard]] auto preference_summary(
            const std::string& profile,
            const bool notifications,
            const bool diagnostics,
            const bool reduced_motion
        ) -> std::string {
            auto result = profile.empty() ? std::string("Unnamed profile") : profile;
            result += notifications ? " · notifications on" : " · notifications off";
            result += diagnostics ? " · diagnostics on" : " · diagnostics off";
            result += reduced_motion ? " · reduced motion" : " · full motion";
            return result;
        }
    } // namespace

    auto build(widget::BuildContext& ui) -> std::shared_ptr<scene::NanNode2D> {
        auto& profile = ui.signal<std::string>("Nandina developer");
        auto& notifications = ui.signal<bool>(true);
        auto& diagnostics = ui.signal<bool>(false);
        auto& reduced_motion = ui.signal<bool>(false);
        auto& status = ui.signal<std::string>("Changes are applied locally");
        auto& summary = ui.computed([&] {
            return preference_summary(
                profile.get(),
                notifications.get(),
                diagnostics.get(),
                reduced_motion.get()
            );
        });

        auto profile_field = ui.text_field(profile, "Profile name").autofocus().build();
        auto diagnostics_note = ui.when(diagnostics, [](widget::BuildContext branch) {
            return branch.label("Anonymous diagnostics help improve rendering stability")
                .color_token(theme::ColorToken::on_surface_variant);
        });

        auto preferences = ui.column()
                               .gap(6.0F)
                               .cross_alignment(widget::LayoutAlignment::stretch)
                               .children(
                                   ui.checkbox(notifications, "Desktop notifications"),
                                   ui.checkbox(diagnostics, "Send anonymous diagnostics"),
                                   diagnostics_note,
                                   ui.checkbox(reduced_motion, "Reduce interface motion")
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
