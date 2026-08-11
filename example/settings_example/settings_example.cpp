//
// Settings example - UI composition and component logic only.
//

#include "settings_example.hpp"

#include "foundation/geometry.hpp"
#include "theme/design_system.hpp"
#include "theme/nan_style.hpp"
#include "widget/controls.hpp"
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
        using widget::authoring::percent;

        // 框架默认调色板（Skeleton 参考，见 phase7 文档）即内置主题；这里仅演示
        // 「默认快照拷贝 → 微调 → 原子应用」的品牌主题路径，保持与默认色板协调。
        auto design = theme::default_design_system();
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

        auto profile_field =
            ui.make<widget::TextField>(profile, "Profile name").autofocus().build();
        auto diagnostics_note = ui.when(diagnostics, [](widget::BuildContext branch) {
            return branch.make<widget::Label>(
                "Anonymous diagnostics help improve rendering stability"
            )
                .color_token(theme::ColorToken::on_surface_variant);
        });

        auto preferences =
            ui.column()
                .gap(6.0F)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(
                    ui.make<widget::Switch>(notifications, "Desktop notifications"),
                    ui.make<widget::Checkbox>(diagnostics, "Send anonymous diagnostics"),
                    diagnostics_note,
                    ui.make<widget::Checkbox>(reduced_motion, "Reduce interface motion"),
                    ui.make<widget::Label>(scale_label)
                        .color_token(theme::ColorToken::on_surface_variant),
                    ui.make<widget::Slider>(
                        interface_scale, "Interface scale", 0.75F, 1.5F, 0.05F
                    )
                )
                .build();

        auto save = ui.make<widget::Button>("Save preferences")
                        .tone(theme::ButtonTone::primary)
                        .width(percent(50.0F))
                        .min_width(180.0F)
                        .on_click([&] { status.set("Preferences saved for " + profile.peek()); });
        auto reset = ui.make<widget::Button>("Reset")
                         .treatment(theme::ButtonTreatment::outlined)
                         .on_click([&] {
            profile.set("Nandina developer");
            notifications.set(true);
            diagnostics.set(false);
            reduced_motion.set(false);
            interface_scale.set(1.0F);
            status.set("Preferences reset");
                         });

        // Light/Dark 切换：捕获 ThemeManager 指针（由应用持有，跨整个 run）。
        // 注意：不能捕获 ui 本身——BuildContext 是每次 build 的临时对象，
        // build() 返回后即失效，回调里再解引用会悬垂。
        auto* themes = &ui.theme_manager();
        auto appearance_row = ui.row()
                                  .gap(8.0F)
                                  .children(
                                      ui.make<widget::Button>("Light")
                                          .treatment(theme::ButtonTreatment::outlined)
                                          .on_click([themes] {
                                              themes->set_preference(theme::ThemePreference::light);
                                          }),
                                      ui.make<widget::Button>("Dark")
                                          .treatment(theme::ButtonTreatment::outlined)
                                          .on_click([themes] {
                                              themes->set_preference(theme::ThemePreference::dark);
                                          })
                                  )
                                  .build();

        auto actions = ui.row()
                           .gap(8.0F)
                           .cross_alignment(widget::LayoutAlignment::center)
                           .children(save, reset)
                           .build();
        auto content = ui.column()
                           .gap(12.0F)
                           .cross_alignment(widget::LayoutAlignment::stretch)
                           .children(
                               ui.make<widget::Label>("Nandina Settings").font_size(28.0F),
                               ui.make<widget::Label>(
                                   "A compact application authored from components and state"
                               )
                                   .color_token(theme::ColorToken::on_surface_variant),
                               ui.make<widget::Label>("Profile").font_size(18.0F),
                               profile_field,
                               ui.make<widget::Label>("Preferences").font_size(18.0F),
                               preferences,
                               ui.make<widget::Label>("Appearance").font_size(18.0F),
                               appearance_row,
                               ui.make<widget::Label>(summary)
                                   .color_token(theme::ColorToken::on_surface_variant),
                               actions,
                               ui.make<widget::Label>(status)
                                   .color_token(theme::ColorToken::primary)
                           );
        return ui.padding(foundation::NanInsets::all(20.0F)).child(content).build();
    }
} // namespace nandina::examples::settings
