//
// Settings example - UI composition and component logic only.
//

#include "settings_example.hpp"

#include "foundation/geometry.hpp"
#include "theme/builtin_themes.hpp"
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

    auto build(const widget::BuildContext& ui) -> std::shared_ptr<scene::NanNode2D> {
        using widget::authoring::percent;

        // 内置主题族：注册框架自带主题，选择 butter（黄油卡片 + Catppuccin 暖调）。
        // 开发者可在此拷贝族快照覆盖色板后原子 apply，或注册自定义族。
        theme::register_default_theme_families(ui.theme_manager());
        (void)ui.theme_manager().activate_family("butter");

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
        auto& active_tab = ui.signal<int>(0);
        auto& active_tab_label = ui.computed([&] {
            const std::string names[] = {"General", "Appearance", "About"};
            return std::string("Active tab: ") + names[active_tab.get()];
        });
        auto& language = ui.signal<int>(0);

        // 该设置直接驱动框架统一动效策略；页面销毁时 effect 随 ReactiveScope 回收。
        auto* themes = &ui.theme_manager();
        (void)ui.effect([themes, &reduced_motion] {
            themes->set_motion_preference(
                reduced_motion.get() ? theme::MotionPreference::reduced
                                     : theme::MotionPreference::system
            );
        });

        auto profile_field =
            ui.make<widget::TextField>(profile, "Profile name").autofocus().build();
        auto diagnostics_note = ui.when(diagnostics, [](widget::BuildContext branch) {
            return branch
                .make<widget::Label>("Anonymous diagnostics help improve rendering stability")
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
                    ui.make<widget::Slider>(interface_scale, "Interface scale", 0.75F, 1.5F, 0.05F),
                    ui.make<widget::Select>(
                        language,
                        std::vector<std::string> {"English", "中文", "日本語", "Test str"}
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
        auto reset_tooltip =
            ui.make<widget::Tooltip>("Restore default preferences", reset.build()).build();

        // Appearance 单选组：互斥 + 键盘方向键漫游，选中直接驱动主题外观。
        // 捕获 ThemeManager 指针（由应用持有）；不能捕获 ui（每次 build 的临时对象）。
        auto appearance_group = widget::RadioGroup::create();
        auto appearance_system = ui.make<widget::RadioButton>("System", appearance_group).build();
        auto appearance_light = ui.make<widget::RadioButton>("Light", appearance_group).build();
        auto appearance_dark = ui.make<widget::RadioButton>("Dark", appearance_group).build();
        appearance_group->select(appearance_system.get());
        ui.connect(appearance_group->selection_changed(), [themes](const int index) {
            static constexpr theme::ThemePreference preferences[] = {
                theme::ThemePreference::system,
                theme::ThemePreference::light,
                theme::ThemePreference::dark,
            };
            themes->set_preference(preferences[index]);
        });
        auto appearance_row = ui.row()
                                  .gap(12.0F)
                                  .cross_alignment(widget::LayoutAlignment::center)
                                  .children(appearance_system, appearance_light, appearance_dark)
                                  .build();

        auto actions = ui.row()
                           .gap(8.0F)
                           .cross_alignment(widget::LayoutAlignment::center)
                           .children(save, reset_tooltip)
                           .build();
        auto content =
            ui.column()
                .gap(12.0F)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(
                    ui.row()
                        .gap(10.0F)
                        .cross_alignment(widget::LayoutAlignment::center)
                        .children(
                            ui.make<widget::Label>("Nandina Settings").font_size(28.0F),
                            ui.make<widget::Badge>("Beta")
                        )
                        .build(),
                    ui.make<widget::Label>(
                          "A compact application authored from components and state"
                    )
                        .color_token(theme::ColorToken::on_surface_variant),
                    ui.make<widget::Label>("Profile").font_size(18.0F),
                    profile_field,
                    ui.make<widget::Label>("Preferences").font_size(18.0F),
                    ui.make<widget::Card>().child(preferences),
                    ui.make<widget::Label>("Appearance").font_size(18.0F),
                    appearance_row,
                    ui.make<widget::Label>("Tabs").font_size(18.0F),
                    ui.make<widget::Card>().child(
                        ui.column()
                            .gap(12.0F)
                            .cross_alignment(widget::LayoutAlignment::stretch)
                            .children(
                                ui.make<widget::Label>("Underline")
                                    .color_token(theme::ColorToken::on_surface_variant),
                                ui.make<widget::Tabs>(
                                    active_tab,
                                    std::vector<std::string> {"General", "Appearance"}
                                ),
                                ui.make<widget::Label>("Segmented")
                                    .color_token(theme::ColorToken::on_surface_variant),
                                ui.make<widget::Tabs>(
                                      std::vector<std::string> {"General", "Appearance"}
                                )
                                    .configure([](widget::Tabs& tabs) {
                                        tabs.set_override(
                                            theme::TabsRecipeRule {
                                                .container_fill = theme::ThemeColor::token(
                                                    theme::ColorToken::surface_variant
                                                ),
                                                .container_radius =
                                                    theme::ThemeScalar::literal(8.0F),
                                                .selected_background_fill =
                                                    theme::ThemeColor::token(
                                                        theme::ColorToken::surface
                                                    ),
                                                .selected_background_radius =
                                                    theme::ThemeScalar::literal(6.0F),
                                                .indicator_color = theme::ThemeColor::transparent(
                                                    theme::ColorToken::primary
                                                ),
                                                .metrics_gap = theme::ThemeScalar::literal(8.0F),
                                                .metrics_padding_x =
                                                    theme::ThemeScalar::literal(6.0F),
                                            }
                                        );
                                    })
                            )
                            .build()
                    ),
                    ui.make<widget::Label>(active_tab_label)
                        .color_token(theme::ColorToken::on_surface_variant),
                    ui.make<widget::Label>("Brand colors").font_size(18.0F),
                    ui.row()
                        .gap(10.0F)
                        .children(
                            // on_primary 配对由下方 Save 主按钮（primary 底 + on_primary 字）演示。
                            ui.make<widget::Label>("Primary").color_token(
                                theme::ColorToken::primary
                            ),
                            ui.make<widget::Label>("Coral").color_token(
                                theme::ColorToken::tertiary
                            ),
                            ui.make<widget::Label>("Variant").color_token(
                                theme::ColorToken::on_surface_variant
                            ),
                            ui.make<widget::Label>("Outline").color_token(
                                theme::ColorToken::outline
                            )
                        )
                        .build(),
                    ui.make<widget::Label>(summary).color_token(
                        theme::ColorToken::on_surface_variant
                    ),
                    actions,
                    ui.make<widget::Label>(status).color_token(
                        theme::ColorToken::on_surface_variant
                    )
                );
        return ui.padding(foundation::NanInsets::all(20.0F)).child(content).build();
    }
} // namespace nandina::examples::settings
