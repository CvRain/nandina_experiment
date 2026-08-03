//
// Theme DesignSystem / shared-fragment tests.
//

#include "scene/scene_tree.hpp"
#include "theme/button_style.hpp"
#include "theme/checkbox_style.hpp"
#include "theme/design_system.hpp"
#include "theme/slider_style.hpp"
#include "theme/theme.hpp"
#include "theme/theme_manager.hpp"
#include "widget/button.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string_view>

namespace
{
    using namespace nandina;

    /** revision 观察探针：统计 on_theme_revision_changed 回调次数。 */
    class RevisionProbe final: public theme::ThemeObserver {
    public:
        std::uint64_t changes = 0;

        void on_theme_revision_changed(const theme::ThemeManager& /*manager*/) override {
            ++changes;
        }
        void on_theme_manager_destroyed(const theme::ThemeManager& /*manager*/) noexcept override {}
    };
} // namespace

TEST_CASE("design system resolves fragment tokens against the active palette", "[theme][design-system]") {
    const auto system = theme::default_design_system();
    const auto box = theme::resolve(
        system,
        theme::ColorAppearance::light,
        system.components.button.base.container
    );
    REQUIRE(box.fill.oklch().light == Catch::Approx(system.light.primary.oklch().light));
    REQUIRE(box.radius == Catch::Approx(system.tokens.radius.md));

    const auto focus = theme::resolve(
        system,
        theme::ColorAppearance::light,
        system.components.button.base.focus
    );
    REQUIRE(focus.width == Catch::Approx(system.tokens.border.focus_ring));
}

TEST_CASE("resolve_button matches the legacy flat resolver with empty rules", "[theme][design-system]") {
    const auto system = theme::default_design_system();
    constexpr auto appearance = theme::ColorAppearance::light;
    const auto resolved = theme::resolve_button(
        system,
        appearance,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    const auto legacy = theme::resolve_button_style(
        theme::NanTheme {system.tokens, system.light},
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    REQUIRE(resolved.container.fill.oklch().light == Catch::Approx(legacy.background.oklch().light));
    REQUIRE(resolved.container.border_width == Catch::Approx(legacy.border_width));
    REQUIRE(resolved.container.radius == Catch::Approx(legacy.radius));
    REQUIRE(resolved.label.font_size == Catch::Approx(legacy.font_size));
    REQUIRE(resolved.metrics.height == Catch::Approx(legacy.height));
    REQUIRE(resolved.metrics.padding_x == Catch::Approx(legacy.padding_x));
}

TEST_CASE("design system button rules overlay the base resolution by selector", "[theme][design-system]") {
    auto system = theme::default_design_system();
    theme::ButtonRecipeRule rule;
    rule.selector.state = theme::ButtonVisualState::hovered;
    rule.container_radius = theme::ThemeScalar::literal(99.0F);
    rule.label_color = theme::ThemeColor::token(theme::ColorToken::error);
    system.components.button.rules.push_back(rule);

    const auto hovered = theme::resolve_button(
        system,
        theme::ColorAppearance::light,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::hovered
    );
    REQUIRE(hovered.container.radius == Catch::Approx(99.0F));
    REQUIRE(hovered.label.color.oklch().light == Catch::Approx(system.light.error.oklch().light));

    const auto normal = theme::resolve_button(
        system,
        theme::ColorAppearance::light,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    REQUIRE(normal.container.radius != Catch::Approx(99.0F));
}

TEST_CASE("resolve_checkbox and resolve_slider produce composed fragments", "[theme][design-system]") {
    const auto system = theme::default_design_system();
    constexpr auto appearance = theme::ColorAppearance::light;

    const auto checkbox = theme::resolve_checkbox(
        system,
        appearance,
        /*checked=*/true,
        theme::CheckboxVisualState::normal
    );
    REQUIRE(
        checkbox.indicator.fill.oklch().light == Catch::Approx(system.light.primary.oklch().light)
    );
    REQUIRE(checkbox.metrics.box_size == Catch::Approx(20.0F));
    REQUIRE(checkbox.metrics.gap == Catch::Approx(system.tokens.spacing.sm));

    const auto slider = theme::resolve_slider(system, appearance, theme::SliderVisualState::normal);
    REQUIRE(
        slider.active_track.box.fill.oklch().light
        == Catch::Approx(system.light.primary.oklch().light)
    );
    REQUIRE(slider.active_track.thickness == Catch::Approx(4.0F));
    REQUIRE(slider.metrics.min_height == Catch::Approx(32.0F));
}

TEST_CASE("ThemeManager apply publishes exactly one revision", "[theme][manager][atomic]") {
    // probe 先于 manager 构造，保证 manager 析构时观察者仍存活。
    RevisionProbe probe;
    theme::ThemeManager manager;
    manager.add_observer(probe);
    const auto before = manager.revision();

    auto system = theme::default_design_system();
    system.light.primary = theme::nan_color(0.51F, 0.10F, 200.0F);
    manager.apply(std::make_shared<const theme::DesignSystem>(std::move(system)));

    REQUIRE(manager.revision() == before + 1);
    REQUIRE(probe.changes == 1);
    REQUIRE(manager.design_system().light.primary.oklch().light == Catch::Approx(0.51F));
    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.51F));
}

TEST_CASE("one DesignSystem carries light and dark palettes switched by preference", "[theme][manager][appearance]") {
    theme::ThemeManager manager;
    auto system = theme::default_design_system();
    system.light.primary = theme::nan_color(0.82F, 0.08F, 250.0F);
    system.dark.primary = theme::nan_color(0.38F, 0.08F, 250.0F);
    manager.apply(std::make_shared<const theme::DesignSystem>(std::move(system)));

    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.82F));
    manager.set_preference(theme::ThemePreference::dark);
    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.38F));
    manager.set_preference(theme::ThemePreference::light);
    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.82F));
}

TEST_CASE("attached widget follows an atomic DesignSystem apply", "[theme][manager][widget]") {
    theme::ThemeManager manager;
    auto system = theme::default_design_system();
    system.light.primary = theme::nan_color(0.61F, 0.12F, 200.0F);
    manager.apply(std::make_shared<const theme::DesignSystem>(std::move(system)));

    scene::NanSceneTree tree;
    tree.set_theme_manager(manager);
    auto button = std::make_shared<widget::Button>("Apply");
    tree.set_root(button);

    REQUIRE(button->theme_ref().palette.primary.oklch().light == Catch::Approx(0.61F));
    REQUIRE(button->resolved_style().container.fill.oklch().light == Catch::Approx(0.61F));
}

TEST_CASE("legacy named themes and families still route through the design system", "[theme][manager][compat]") {
    theme::ThemeManager manager;
    auto light = theme::default_theme();
    light.palette.primary = theme::nan_color(0.72F, 0.12F, 120.0F);
    auto dark = theme::default_theme();
    dark.palette.primary = theme::nan_color(0.38F, 0.12F, 120.0F);
    REQUIRE(manager.register_theme("brand-light", light));
    REQUIRE(manager.register_theme("brand-dark", dark));
    REQUIRE(manager.register_family("brand", "brand-light", "brand-dark"));
    REQUIRE(manager.activate_family("brand"));

    REQUIRE(manager.active_name() == "brand-light");
    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.72F));
    REQUIRE(
        manager.design_system().palette(theme::ColorAppearance::light).primary.oklch().light
        == Catch::Approx(0.72F)
    );

    manager.set_system_appearance(theme::ColorAppearance::dark);
    REQUIRE(manager.active_name() == "brand-dark");
    REQUIRE(manager.theme().palette.primary.oklch().light == Catch::Approx(0.38F));
    REQUIRE(
        manager.design_system().palette(theme::ColorAppearance::dark).primary.oklch().light
        == Catch::Approx(0.38F)
    );
}
