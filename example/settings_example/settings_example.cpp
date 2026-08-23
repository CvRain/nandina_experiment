//
// Settings example - router-driven dashboard implementation.
//

#include "settings_example.hpp"

#include "animation/motion.hpp"
#include "foundation/geometry.hpp"
#include "theme/builtin_themes.hpp"
#include "widget/controls.hpp"
#include "widget/layout.hpp"

#include <format>
#include <string>
#include <utility>
#include <vector>

namespace nandina::examples::settings
{
    namespace
    {
        using widget::authoring::percent;

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

        [[nodiscard]] auto section_heading(const widget::BuildContext& ui, const std::string& text)
            -> std::shared_ptr<widget::Label> {
            return ui.make<widget::Label>(text).font_size(20.0F).build();
        }
    } // namespace

    SettingsStore::SettingsStore(reactive::Graph& graph):
        profile(graph, "Nandina developer"),
        notifications(graph, true),
        diagnostics(graph, false),
        reduced_motion(graph, false),
        interface_scale(graph, 1.0F),
        language(graph, 0),
        family(graph, 0),
        status(graph, "Changes are applied locally"),
        confirm_reset(graph, false) {}

    ShellPage::~ShellPage() = default;

    auto ShellPage::build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> {
        auto& store = context.store<SettingsStore>();
        auto ui = context.ui();

        // 内置主题族：注册 butter / fluent / material，默认 butter；Appearance 页可切换。
        theme::register_default_theme_families(ui.theme_manager());
        (void)ui.theme_manager().activate_family("butter");

        // 主题族切换挂在常驻 shell（Appearance 页的 Select 驱动它），跨页保持生效。
        auto* themes = &ui.theme_manager();
        (void)ui.effect([themes, &store] {
            static constexpr const char* names[] = {"butter", "fluent", "material"};
            (void)themes->activate_family(names[store.family.get()]);
        });

        // 嵌套内容 router（真实 StackView）：侧边栏切换 section 页，About 推 detail 页。
        // 开启页面转场：replace/push/pop 淡入淡出，且被替换页面的生命周期在淡出期间保留；
        // 全局 reduced-motion 打开时由 AnimationHost 统一直跳。
        content_ = std::make_unique<app::NanRouter>(
            context.graph(),
            context.theme_manager(),
            &store,
            app::nan_type_key<SettingsStore>(),
            nullptr,
            nullptr,
            nullptr,
            &context.dispatcher(),
            nullptr
        );
        content_->set_transition_enabled(true);
        content_->set_transition_duration(0.22F);
        content_->push<GeneralPage>();

        auto nav_button = [&ui](const std::string& text, auto&& action) {
            return ui.make<widget::Button>(text)
                .on_click(std::forward<decltype(action)>(action))
                .build();
        };
        auto sidebar =
            ui.column()
                .gap(8.0F)
                .children(
                    ui.make<widget::Label>("Nandina").font_size(22.0F),
                    ui.make<widget::Badge>("Beta"),
                    nav_button(
                        "General",
                        [this] { (void)content_->request_replace<GeneralPage>(); }
                    ),
                    nav_button(
                        "Appearance",
                        [this] { (void)content_->request_replace<AppearancePage>(); }
                    ),
                    nav_button(
                        "Components",
                        [this] { (void)content_->request_replace<ComponentsPage>(); }
                    ),
                    nav_button("About", [this] { (void)content_->request_replace<AboutPage>(); })
                )
                .build();

        // 重置确认对话框：General 页 Reset 按钮置位 confirm_reset，effect 打开对话框。
        auto reset_dialog = ui.make<widget::Dialog>("Reset preferences?").build();
        auto cancel =
            ui.make<widget::Button>("Cancel").on_click(
                                                 [reset_dialog] { reset_dialog->close(); }
            ).build();
        auto confirm_reset_button = ui.make<widget::Button>("Confirm reset")
                                        .tone(theme::ButtonTone::primary)
                                        .on_click([reset_dialog, &store] {
                                            store.profile.set("Nandina developer");
                                            store.notifications.set(true);
                                            store.diagnostics.set(false);
                                            store.reduced_motion.set(false);
                                            store.interface_scale.set(1.0F);
                                            store.status.set("Preferences reset");
                                            store.confirm_reset.set(false);
                                            reset_dialog->close();
                                        })
                                        .build();
        (void)reset_dialog->set_content(
            ui.row().gap(8.0F).children(cancel, confirm_reset_button).build()
        );
        reset_dialog->set_on_close([&store] { store.confirm_reset.set(false); });
        (void)ui.effect([reset_dialog, &store] {
            if (store.confirm_reset.get()) {
                reset_dialog->open();
            }
        });

        auto content = ui.padding(foundation::NanInsets::all(16.0F))
                           .child(ui.row()
                                      .gap(16.0F)
                                      .cross_alignment(widget::LayoutAlignment::stretch)
                                      .children(sidebar, ui.expanded().child(content_->host())))
                           .build();
        return ui.stack().children(content, reset_dialog).build();
    }

    auto GeneralPage::build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> {
        auto& store = context.store<SettingsStore>();
        auto ui = context.ui();

        auto& summary = ui.computed([&store] {
            return preference_summary(
                store.profile.get(),
                store.notifications.get(),
                store.diagnostics.get(),
                store.reduced_motion.get(),
                store.interface_scale.get()
            );
        });
        auto& scale_label = ui.computed([&store] {
            return std::format("Interface scale · {:.0f}%", store.interface_scale.get() * 100.0F);
        });

        // 该设置直接驱动框架统一动效策略。
        auto* themes = &ui.theme_manager();
        (void)ui.effect([themes, &store] {
            themes->set_motion_preference(
                store.reduced_motion.get() ? theme::MotionPreference::reduced
                                           : theme::MotionPreference::system
            );
        });

        auto profile_field =
            ui.make<widget::TextField>(store.profile, "Profile name").autofocus().build();
        auto diagnostics_note = ui.when(store.diagnostics, [](widget::BuildContext branch) {
            return branch
                .make<widget::Label>("Anonymous diagnostics help improve rendering stability")
                .color_token(theme::ColorToken::on_surface_variant);
        });

        auto preferences =
            ui.column()
                .gap(6.0F)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(
                    ui.make<widget::Switch>(store.notifications, "Desktop notifications"),
                    ui.make<widget::Checkbox>(store.diagnostics, "Send anonymous diagnostics"),
                    diagnostics_note,
                    ui.make<widget::Checkbox>(store.reduced_motion, "Reduce interface motion"),
                    ui.make<widget::Label>(scale_label)
                        .color_token(theme::ColorToken::on_surface_variant),
                    ui.make<widget::Slider>(
                        store.interface_scale,
                        "Interface scale",
                        0.75F,
                        1.5F,
                        0.05F
                    )
                        .configure([](widget::Slider& slider) {
                            // 值标签：在轨道上方显示当前值（raw value），并随拖动实时更新。
                            slider.set_show_value_label(true);
                        }),
                    ui.make<widget::Select>(
                        store.language,
                        std::vector<std::string> {"English", "中文", "日本語", "Test str"}
                    )
                )
                .build();

        auto save = ui.make<widget::Button>("Save preferences")
                        .tone(theme::ButtonTone::primary)
                        .width(percent(50.0F))
                        .min_width(180.0F)
                        .on_click([&store] {
                            store.status.set("Preferences saved for " + store.profile.peek());
                        });
        auto reset = ui.make<widget::Button>("Reset")
                         .treatment(theme::ButtonTreatment::outlined)
                         .on_click([&store] { store.confirm_reset.set(true); });
        auto reset_tooltip =
            ui.make<widget::Tooltip>("Restore default preferences", reset.build()).build();
        auto actions = ui.row()
                           .gap(8.0F)
                           .cross_alignment(widget::LayoutAlignment::center)
                           .children(save, reset_tooltip)
                           .build();

        auto content = ui.column()
                           .gap(12.0F)
                           .cross_alignment(widget::LayoutAlignment::stretch)
                           .children(
                               section_heading(ui, "General"),
                               ui.make<widget::Label>("Profile").font_size(18.0F),
                               profile_field,
                               ui.make<widget::Label>("Preferences").font_size(18.0F),
                               ui.make<widget::Card>().child(preferences),
                               ui.make<widget::Label>(summary).color_token(
                                   theme::ColorToken::on_surface_variant
                               ),
                               actions,
                               ui.make<widget::Label>(store.status)
                                   .color_token(theme::ColorToken::on_surface_variant)
                           );
        return ui.padding(foundation::NanInsets::all(20.0F))
            .child(ui.scroll_view().child(content))
            .build();
    }

    auto AppearancePage::build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> {
        auto& store = context.store<SettingsStore>();
        auto ui = context.ui();

        auto* themes = &ui.theme_manager();
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
        auto family_select = ui.make<widget::Select>(
                                   store.family,
                                   std::vector<std::string> {"butter", "fluent", "material"}
        )
                                 .build();

        auto content = ui.column()
                           .gap(12.0F)
                           .cross_alignment(widget::LayoutAlignment::stretch)
                           .children(
                               section_heading(ui, "Appearance"),
                               appearance_row,
                               ui.make<widget::Label>("Theme family").font_size(18.0F),
                               family_select,
                               ui.make<widget::Label>("Brand colors").font_size(18.0F),
                               ui.row()
                                   .gap(10.0F)
                                   .children(
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
                                   .build()
                           );
        return ui.padding(foundation::NanInsets::all(20.0F))
            .child(ui.scroll_view().child(content))
            .build();
    }

    auto ComponentsPage::build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> {
        auto& store = context.store<SettingsStore>();
        auto ui = context.ui();

        auto& active_tab = ui.signal<int>(0);
        auto& active_tab_label = ui.computed([&active_tab] {
            const std::string names[] = {"General", "Appearance", "About"};
            return std::string("Active tab: ") + names[active_tab.get()];
        });

        // Divider 样式变体：按钮循环切换 solid/dashed/double/double_dashed。
        auto& divider_style = ui.signal<int>(0);
        auto divider = ui.make<widget::Divider>().build();
        (void)ui.effect([divider, &divider_style] {
            constexpr widget::Divider::Pattern patterns[] = {
                widget::Divider::Pattern::solid,
                widget::Divider::Pattern::dashed,
                widget::Divider::Pattern::double_line,
                widget::Divider::Pattern::double_dashed,
            };
            divider->set_pattern(patterns[divider_style.get()]);
        });
        auto& divider_label = ui.computed([&divider_style] {
            const std::string names[] = {"Solid", "Dashed", "Double", "Double dashed"};
            return std::string("Divider · ") + names[divider_style.get()];
        });
        auto cycle_divider =
            ui.make<widget::Button>("Cycle divider style")
                .on_click([&divider_style] { divider_style.set((divider_style.peek() + 1) % 4); })
                .build();

        // Button treatment 变体：循环 filled/tonal/outlined/ghost/link。
        auto& treatment_style = ui.signal<int>(0);
        auto demo_button = ui.make<widget::Button>("Stylized button").build();
        (void)ui.effect([demo_button, &treatment_style] {
            constexpr theme::ButtonTreatment treatments[] = {
                theme::ButtonTreatment::filled,
                theme::ButtonTreatment::tonal,
                theme::ButtonTreatment::outlined,
                theme::ButtonTreatment::ghost,
                theme::ButtonTreatment::link,
            };
            demo_button->set_treatment(treatments[treatment_style.get()]);
        });
        auto& treatment_label = ui.computed([&treatment_style] {
            const std::string names[] = {"Filled", "Tonal", "Outlined", "Ghost", "Link"};
            return std::string("Button · ") + names[treatment_style.get()];
        });
        auto cycle_treatment = ui.make<widget::Button>("Cycle button style")
                                   .on_click([&treatment_style] {
                                       treatment_style.set((treatment_style.peek() + 1) % 5);
                                   })
                                   .build();

        // 声明式动画示范：点击让圆角在 4 ↔ 24 之间平滑过渡（reduced-motion 下由 Host 直跳）。
        // motion::tween 走固定时长缓动，motion::spring 走阻尼弹簧（带 overshoot）。
        auto& motion_radius = ui.signal<float>(4.0F);
        auto motion_button = ui.make<widget::Button>("Animate radius (tween)")
                                 .behavior(
                                     widget::visual::container.radius,
                                     animation::motion::tween(0.3F).easing(
                                         animation::motion::ease_standard
                                     )
                                 )
                                 .bind(widget::visual::container.radius, motion_radius)
                                 .on_click([&motion_radius] {
                                     motion_radius.set(motion_radius.peek() > 12.0F ? 4.0F : 24.0F);
                                 })
                                 .build();
        auto& spring_radius = ui.signal<float>(4.0F);
        auto spring_button = ui.make<widget::Button>("Animate radius (spring)")
                                 .spring(
                                     widget::visual::container.radius,
                                     animation::motion::spring().stiffness(200.0F).damping(10.0F)
                                 )
                                 .bind(widget::visual::container.radius, spring_radius)
                                 .on_click([&spring_radius] {
                                     spring_radius.set(spring_radius.peek() > 12.0F ? 4.0F : 24.0F);
                                 })
                                 .build();
        auto& motion_label = ui.computed([&motion_radius, &spring_radius] {
            return std::format(
                "Radius · tween {:.0f}px · spring {:.0f}px",
                motion_radius.get(),
                spring_radius.get()
            );
        });

        auto content =
            ui.column()
                .gap(12.0F)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(
                    section_heading(ui, "Components"),
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
                    ui.make<widget::Label>("Basics").font_size(18.0F),
                    ui.row()
                        .gap(10.0F)
                        .cross_alignment(widget::LayoutAlignment::center)
                        .children(
                            ui.make<widget::Avatar>("Nandina"),
                            ui.make<widget::Chip>("Design"),
                            ui.make<widget::Chip>("Removable", true)
                                .configure([&store](widget::Chip& chip) {
                                    chip.set_on_remove([&store] {
                                        store.status.set("Removed the chip");
                                    });
                                })
                        )
                        .build(),
                    ui.row()
                        .gap(10.0F)
                        .cross_alignment(widget::LayoutAlignment::center)
                        .children(
                            divider,
                            ui.make<widget::Label>(divider_label)
                                .color_token(theme::ColorToken::on_surface_variant)
                        )
                        .build(),
                    cycle_divider,
                    ui.row()
                        .gap(10.0F)
                        .cross_alignment(widget::LayoutAlignment::center)
                        .children(
                            demo_button,
                            ui.make<widget::Label>(treatment_label)
                                .color_token(theme::ColorToken::on_surface_variant)
                        )
                        .build(),
                    cycle_treatment,
                    ui.make<widget::Label>("Motion").font_size(18.0F),
                    motion_button,
                    spring_button,
                    ui.make<widget::Label>(motion_label)
                        .color_token(theme::ColorToken::on_surface_variant)
                );
        return ui.padding(foundation::NanInsets::all(20.0F))
            .child(ui.scroll_view().child(content))
            .build();
    }

    auto AboutPage::build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> {
        auto ui = context.ui();
        auto& router = context.router();

        // 图片/纹理子系统示范：按文件路径（相对可执行文件目录）加载一张品牌图。
        auto logo = widget::Image::create("nandina_logo.png");
        logo->set_size(foundation::NanSize(360.0F, 200.0F));

        auto content =
            ui.column()
                .gap(12.0F)
                .cross_alignment(widget::LayoutAlignment::stretch)
                .children(
                    section_heading(ui, "About"),
                    logo,
                    ui.make<widget::Label>("Nandina is a C++ + raylib game-engine-as-UI framework")
                        .color_token(theme::ColorToken::on_surface_variant),
                    ui.make<widget::Button>("Open component detail")
                        .on_click([&router] {
                            (void)router.request_push<DetailPage>(DetailParams {
                                .title = "Select",
                                .body = "Pushed with route params; press Back to pop.",
                            });
                        })
                        .build()
                );
        return ui.padding(foundation::NanInsets::all(20.0F))
            .child(ui.scroll_view().child(content))
            .build();
    }

    auto DetailPage::build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> {
        auto ui = context.ui();
        auto& router = context.router();

        auto content = ui.column()
                           .gap(12.0F)
                           .cross_alignment(widget::LayoutAlignment::stretch)
                           .children(
                               section_heading(ui, "Detail"),
                               ui.make<widget::Label>(params().title).font_size(22.0F),
                               ui.make<widget::Label>(params().body)
                                   .color_token(theme::ColorToken::on_surface_variant),
                               ui.make<widget::Button>("Back")
                                   .on_click([&router] { (void)router.request_pop(); })
                                   .build()
                           );
        return ui.padding(foundation::NanInsets::all(20.0F))
            .child(ui.scroll_view().child(content))
            .build();
    }

} // namespace nandina::examples::settings
