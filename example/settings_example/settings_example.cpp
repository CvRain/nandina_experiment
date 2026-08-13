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

        /// 品牌主题：Catppuccin 官方调色板（MIT）——亮色取 Latte、暗色取 Mocha，
        /// 主色亮色用 Rosewater（#dc8a78）、暗色用 Peach（#fab387），强调用 Flamingo/Maroon。
        /// 中性色（Base/Mantle/Crust/Subtext/Surface）按官方 style-guide 映射；
        /// on_primary 遵循「On Accent = Base」取 Mocha Base，两套对比度均满足 AA。
        [[nodiscard]] auto hex_color(const std::uint32_t value) -> theme::NanColor {
            return theme::NanColor::from(theme::NanHexRgb {
                static_cast<std::uint8_t>(value >> 16),
                static_cast<std::uint8_t>(value >> 8),
                static_cast<std::uint8_t>(value),
                255,
            });
        }

        [[nodiscard]] auto brand_design_system() -> theme::DesignSystem {
            auto reference = theme::default_reference_palette();
            reference.primary.stops = {
                hex_color(0xFFF0E6), hex_color(0xFFDFC8), hex_color(0xFFC9A3),
                hex_color(0xFAB387), hex_color(0xFE8B52), hex_color(0xDC8A78),
                hex_color(0xD14F08), hex_color(0xA53D06), hex_color(0x7A2C04),
                hex_color(0x4F1D02), hex_color(0x1E1E2E),
            };
            reference.tertiary.stops = {
                hex_color(0xF5E0DC), hex_color(0xF2CDCD), hex_color(0xEBA0AC),
                hex_color(0xDD7878), hex_color(0xE64553), hex_color(0xD20F39),
                hex_color(0xB20F30), hex_color(0x8A0F25), hex_color(0x610D1A),
                hex_color(0x3D080F), hex_color(0x1E0408),
            };
            reference.neutral.stops = {
                hex_color(0xEFF1F5), hex_color(0xE6E9EF), hex_color(0xDCE0E8),
                hex_color(0xCCD0DA), hex_color(0xA6ADC8), hex_color(0x8C8FA1),
                hex_color(0x7F849C), hex_color(0x5C5F77), hex_color(0x45475A),
                hex_color(0x313244), hex_color(0x1E1E2E),
            };
            const auto policy = theme::PaletteVariantPolicy {
                .light_brand = theme::ColorShade::shade_500,
                .dark_brand = theme::ColorShade::shade_300,
            };
            auto design = theme::default_design_system();
            design.light = theme::make_color_scheme(
                reference, theme::ColorAppearance::light, policy
            );
            design.dark = theme::make_color_scheme(
                reference, theme::ColorAppearance::dark, policy
            );
            // 卡片风格：圆角 12/16/24，边框 2/3（软阴影待渲染层图元，见迭代清单）。
            design.tokens.radius.sm = 12.0F;
            design.tokens.radius.md = 16.0F;
            design.tokens.radius.lg = 24.0F;
            design.tokens.border.thin = 2.0F;
            design.tokens.border.medium = 3.0F;
            return design;
        }
    } // namespace

    auto build(widget::BuildContext& ui) -> std::shared_ptr<scene::NanNode2D> {
        using widget::authoring::percent;

        // 品牌主题：亮/暗 primary / on_primary 配对由品牌色阶生成（暗色提升品牌档位），
        // 开发者可参考「默认快照拷贝 → 覆盖色板 → 原子应用」的完整品牌覆盖路径。
        auto design = brand_design_system();
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
                    ui.make<widget::Slider>(interface_scale, "Interface scale", 0.75F, 1.5F, 0.05F)
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
        auto content =
            ui.column()
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
                    ui.make<widget::Label>("Brand colors").font_size(18.0F),
                    ui.row()
                        .gap(10.0F)
                        .children(
                            // on_primary 配对由下方 Save 主按钮（primary 底 + on_primary 字）演示。
                            ui.make<widget::Label>("Primary")
                                .color_token(theme::ColorToken::primary),
                            ui.make<widget::Label>("Coral")
                                .color_token(theme::ColorToken::tertiary),
                            ui.make<widget::Label>("Variant")
                                .color_token(theme::ColorToken::on_surface_variant),
                            ui.make<widget::Label>("Outline")
                                .color_token(theme::ColorToken::outline)
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
