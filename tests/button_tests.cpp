//
// Theme / primitives / Button tests.
//

#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "theme/button_style.hpp"
#include "theme/nan_style.hpp"
#include "theme/style_document.hpp"
#include "theme/theme_manager.hpp"
#include "widget/button.hpp"
#include "widget/text_field.hpp"
#include "widget/primitives/pressable.hpp"
#include "widget/primitives/surface.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    using namespace nandina;

    class RecordingDevice final: public render::IRenderDevice {
    public:
        struct RectCall {
            foundation::NanRect rect;
            float alpha = 0.0F;
            bool outline = false;
            bool rounded = false;
        };
        struct TextCall {
            std::string text;
            float font_size = 0.0F;
            float alpha = 0.0F;
        };

        std::vector<RectCall> rects;
        std::vector<TextCall> texts;

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}

        void draw_rect(const foundation::NanRect& rect, const foundation::NanColor& color) override {
            rects.push_back({.rect = rect, .alpha = color.alpha(), .outline = false, .rounded = false});
        }

        void draw_rect_outline(const foundation::NanRect& rect, float, const foundation::NanColor& color) override {
            rects.push_back({.rect = rect, .alpha = color.alpha(), .outline = true, .rounded = false});
        }

        void draw_rounded_rect(const foundation::NanRect& rect, float, const foundation::NanColor& color) override {
            rects.push_back({.rect = rect, .alpha = color.alpha(), .outline = false, .rounded = true});
        }

        void draw_line(const foundation::NanPoint&, const foundation::NanPoint&, float, const foundation::NanColor&) override {}
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {}

        void draw_text(
            std::string_view text,
            const foundation::NanPoint&,
            float font_size,
            const foundation::NanColor& color
        ) override {
            texts.push_back({.text = std::string(text), .font_size = font_size, .alpha = color.alpha()});
        }
    };

    class TestPressable final: public widget::primitives::Pressable {
    public:
        explicit TestPressable(foundation::NanSize size): Pressable(size) {}
        int state_changes = 0;

    protected:
        void on_pressable_state_changed() override {
            ++state_changes;
        }
    };

} // namespace

TEST_CASE("button style resolver keeps tone and treatment orthogonal", "[theme][button]") {
    const auto theme = theme::default_theme();

    const auto filled = theme::resolve_button_style(
        theme,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::filled,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    REQUIRE(filled.background.alpha() == Catch::Approx(1.0F));
    REQUIRE(filled.border_width == Catch::Approx(0.0F));

    const auto outlined = theme::resolve_button_style(
        theme,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::outlined,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    REQUIRE(outlined.background.alpha() == Catch::Approx(0.0F));
    REQUIRE(outlined.border_width == Catch::Approx(theme.tokens.border.thin));

    const auto danger = theme::button_color_pair(theme, theme::ButtonTone::danger);
    REQUIRE(danger.accent.alpha() == Catch::Approx(1.0F));
}

TEST_CASE("NanStyle resolves token rules and preserves literal rules", "[theme][style]") {
    auto style = std::make_shared<theme::NanStyle>();
    theme::ButtonStyleRule rule;
    rule.selector.treatment = theme::ButtonTreatment::ghost;
    rule.background = theme::ThemeColor::token(theme::ColorToken::primary);
    rule.radius = theme::ThemeScalar::literal(13.0F);
    rule.padding_x = theme::ThemeScalar::token(theme::ScalarToken::spacing_xl);
    style->add_button_rule(std::move(rule));

    auto first = theme::default_theme();
    first.palette.primary = theme::nan_color(0.42F, 0.1F, 120.0F);
    first.tokens.spacing.xl = 31.0F;
    const auto first_result = style->resolve_button(
        first,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::ghost,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );

    auto second = first;
    second.palette.primary = theme::nan_color(0.78F, 0.1F, 120.0F);
    second.tokens.spacing.xl = 37.0F;
    const auto second_result = style->resolve_button(
        second,
        theme::ButtonTone::primary,
        theme::ButtonTreatment::ghost,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );

    REQUIRE(first_result.background.oklch().light == Catch::Approx(0.42F));
    REQUIRE(second_result.background.oklch().light == Catch::Approx(0.78F));
    REQUIRE(first_result.padding_x == Catch::Approx(31.0F));
    REQUIRE(second_result.padding_x == Catch::Approx(37.0F));
    REQUIRE(first_result.radius == Catch::Approx(13.0F));
    REQUIRE(second_result.radius == Catch::Approx(13.0F));
}

TEST_CASE("NanStyle supports application-specific rule algorithms", "[theme][style]") {
    class ApplicationStyle final: public theme::NanStyle {
    public:
        [[nodiscard]] auto resolve_button(
            const theme::NanTheme& current,
            theme::ButtonTone tone,
            theme::ButtonTreatment treatment,
            theme::ButtonSize size,
            theme::ButtonVisualState state
        ) const -> theme::ButtonStyle override {
            auto result = NanStyle::resolve_button(current, tone, treatment, size, state);
            if (treatment == theme::ButtonTreatment::link) {
                result.height = 27.0F;
            }
            return result;
        }
    };

    const ApplicationStyle style;
    const auto result = style.resolve_button(
        theme::default_theme(),
        theme::ButtonTone::primary,
        theme::ButtonTreatment::link,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    REQUIRE(result.height == Catch::Approx(27.0F));
}

TEST_CASE("ThemeManager switches attached widget trees by revision", "[theme][manager]") {
    theme::ThemeManager manager;
    auto light = theme::default_theme();
    light.palette.primary = theme::nan_color(0.72F, 0.12F, 120.0F);
    auto dark = theme::default_theme();
    dark.palette.primary = theme::nan_color(0.38F, 0.12F, 120.0F);
    REQUIRE(manager.register_theme("light", light));
    REQUIRE(manager.register_theme("dark", dark));

    auto style = std::make_shared<theme::NanStyle>();
    theme::ButtonStyleRule rule;
    rule.selector.treatment = theme::ButtonTreatment::ghost;
    rule.background = theme::ThemeColor::token(theme::ColorToken::primary);
    style->add_button_rule(std::move(rule));
    manager.set_style(style);
    REQUIRE(manager.activate("light"));

    scene::NanSceneTree tree;
    tree.set_theme_manager(manager);
    auto button = std::make_shared<widget::Button>("Switch");
    button->set_treatment(theme::ButtonTreatment::ghost);
    tree.set_root(button);

    const auto before = manager.revision();
    (void)tree.layout_root(foundation::NanSize(320.0F, 120.0F));
    REQUIRE_FALSE(button->layout_dirty());
    REQUIRE(button->resolved_style().background.oklch().light == Catch::Approx(0.72F));
    REQUIRE(manager.activate("dark"));
    REQUIRE(manager.revision() == before + 1);
    REQUIRE(button->layout_dirty());
    REQUIRE(button->theme_ref().palette.primary.oklch().light == Catch::Approx(0.38F));
    REQUIRE(button->resolved_style().background.oklch().light == Catch::Approx(0.38F));
}

TEST_CASE("TextField rules compose focused and invalid states", "[theme][text-field]") {
    theme::ThemeManager manager;
    auto style = std::make_shared<theme::NanStyle>();
    theme::TextFieldStyleRule focused;
    focused.state = theme::TextFieldVisualState::focused;
    focused.height = theme::ThemeScalar::literal(46.0F);
    style->add_text_field_rule(std::move(focused));
    theme::TextFieldStyleRule invalid;
    invalid.state = theme::TextFieldVisualState::invalid;
    invalid.border_color = theme::ThemeColor::token(theme::ColorToken::error);
    style->add_text_field_rule(std::move(invalid));
    manager.set_style(style);

    scene::NanSceneTree tree;
    tree.set_theme_manager(manager);
    auto field = std::make_shared<widget::TextField>("value");
    tree.set_root(field);
    tree.set_focus(field.get());
    field->set_invalid(true);

    REQUIRE(theme::has_text_field_state(
        field->visual_state(), theme::TextFieldVisualState::focused
    ));
    REQUIRE(theme::has_text_field_state(
        field->visual_state(), theme::TextFieldVisualState::invalid
    ));
    REQUIRE(field->resolved_style().height == Catch::Approx(46.0F));
    REQUIRE(
        field->resolved_style().border_color.oklch().light
        == Catch::Approx(manager.theme().palette.error.oklch().light)
    );
    REQUIRE(field->resolved_style().focus_ring_width > 0.0F);
}

TEST_CASE("SceneTree releases a destroyed ThemeManager", "[theme][manager][lifetime]") {
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("Lifetime");
    tree.set_root(button);
    {
        theme::ThemeManager manager;
        tree.set_theme_manager(manager);
        REQUIRE(tree.theme_manager() == &manager);
    }
    REQUIRE(tree.theme_manager() == nullptr);
}

TEST_CASE("styles.toml compiles into the same theme runtime objects", "[theme][toml]") {
    constexpr std::string_view source = R"toml(
active_theme = "dark"

[themes.dark.palette]
primary = [0.31, 0.12, 250.0, 1.0]

[themes.dark.tokens.spacing]
xl = 33.0

[[styles.button]]
treatment = "ghost"
background = "$palette.primary"
radius = 12.0
padding_x = "$tokens.spacing.xl"

[[styles.text_field]]
state = "focused"
border_color = "$palette.primary"
height = 44.0

[[fonts.family]]
name = "families/application-ui"
default = true
fallbacks = ["families/fallback-ui"]
faces = [{ resource = "fonts/application-ui/medium", weight = 500, slant = "normal" }]

[[fonts.family]]
name = "families/fallback-ui"
faces = [{ resource = "fonts/fallback-ui/regular", weight = 400 }]
)toml";

    auto document = theme::parse_style_document(source);
    REQUIRE(document.has_value());
    REQUIRE(document->themes.size() == 1);
    REQUIRE(document->font_families.size() == 2);
    REQUIRE(document->style->button_rules().size() == 1);
    REQUIRE(document->style->text_field_rules().size() == 1);

    theme::ThemeManager manager;
    text::FontFamilyRegistry fonts;
    const auto applied = document->apply(manager, &fonts);
    REQUIRE(applied.has_value());
    REQUIRE(manager.active_name() == "dark");

    const auto resolved = manager.style().resolve_button(
        manager.theme(),
        theme::ButtonTone::primary,
        theme::ButtonTreatment::ghost,
        theme::ButtonSize::medium,
        theme::ButtonVisualState::normal
    );
    REQUIRE(resolved.background.oklch().light == Catch::Approx(0.31F));
    REQUIRE(resolved.padding_x == Catch::Approx(33.0F));
    REQUIRE(resolved.radius == Catch::Approx(12.0F));

    const auto field_style = manager.style().resolve_text_field(
        manager.theme(), theme::TextFieldVisualState::focused
    );
    REQUIRE(field_style.border_color.oklch().light == Catch::Approx(0.31F));
    REQUIRE(field_style.height == Catch::Approx(44.0F));

    const auto invalid = theme::parse_style_document(R"toml(
[[styles.button]]
background = "$palette.missing"
)toml");
    REQUIRE_FALSE(invalid.has_value());
}

TEST_CASE("Button instance theme and StyleContext keep their cascade priority", "[theme][cascade]") {
    theme::ThemeManager manager;
    auto application_theme = theme::default_theme();
    application_theme.palette.primary = theme::nan_color(0.35F, 0.1F, 40.0F);
    manager.set_theme(application_theme);

    scene::NanSceneTree tree;
    tree.set_theme_manager(manager);
    auto button = std::make_shared<widget::Button>("Fixed");
    auto instance_theme = theme::default_theme();
    instance_theme.palette.primary = theme::nan_color(0.81F, 0.1F, 40.0F);
    button->set_theme(instance_theme);

    theme::StyleContext context;
    context.font_size = theme::StyleValue<float>::explicit_value(29.0F);
    context.text_color = theme::StyleValue<foundation::NanColor>::explicit_value(
        theme::nan_color(0.66F, 0.08F, 180.0F, 0.4F)
    );
    button->set_style_context(context);
    tree.set_root(button);

    REQUIRE(button->theme_ref().palette.primary.oklch().light == Catch::Approx(0.81F));
    REQUIRE(button->resolved_style().background.oklch().light == Catch::Approx(0.81F));
    REQUIRE(button->text_node().font_size() == Catch::Approx(29.0F));
    REQUIRE(button->text_node().color().oklch().light == Catch::Approx(0.66F));
    REQUIRE(button->text_node().color().alpha() == Catch::Approx(0.4F));

    auto replacement = application_theme;
    replacement.palette.primary = theme::nan_color(0.22F, 0.1F, 40.0F);
    manager.set_theme(replacement);
    REQUIRE(button->theme_ref().palette.primary.oklch().light == Catch::Approx(0.81F));
}

TEST_CASE("surface draws rounded fill and outline", "[widget][primitive][surface]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto surface = std::make_shared<widget::primitives::Surface>(foundation::NanSize(40.0F, 20.0F));
    surface->set_radius(8.0F);
    surface->set_fill(theme::nan_color(0.5F, 0.1F, 120.0F));
    surface->set_border(theme::nan_color(0.8F, 0.1F, 120.0F), 2.0F);
    tree.set_root(surface);

    tree.draw(dev);

    REQUIRE(dev.rects.size() == 2);
    REQUIRE(dev.rects[0].rounded);
    REQUIRE(dev.rects[1].outline);
}

TEST_CASE("pressable tracks hover press and emits click", "[widget][primitive][pressable]") {
    TestPressable pressable {foundation::NanSize(100.0F, 40.0F)};
    int clicks = 0;
    pressable.set_on_click([&] { ++clicks; });

    scene::MouseEnterEvent enter {foundation::NanPoint(2.0F, 2.0F)};
    REQUIRE_FALSE(pressable.on_input(enter));
    REQUIRE(pressable.hovered());

    scene::MouseButtonEvent down {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(2.0F, 2.0F)
    };
    REQUIRE(pressable.on_input(down));
    REQUIRE(pressable.pressed());

    scene::MouseButtonEvent up {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        foundation::NanPoint(2.0F, 2.0F)
    };
    REQUIRE(pressable.on_input(up));
    REQUIRE_FALSE(pressable.pressed());
    REQUIRE(clicks == 1);
    REQUIRE(pressable.state_changes >= 2);
}

TEST_CASE("button draws background and text and reacts to press state", "[widget][button]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("Open");
    button->set_position(foundation::NanPoint(10.0F, 20.0F));
    int clicks = 0;
    button->set_on_click([&] { ++clicks; });
    tree.set_root(button);

    tree.draw(dev);
    REQUIRE(dev.rects.size() == 1);
    REQUIRE(dev.rects[0].rounded);
    REQUIRE(dev.texts.size() == 1);
    REQUIRE(dev.texts[0].text == "Open");

    tree.dispatch_mouse_move(scene::MouseMoveEvent {foundation::NanPoint(20.0F, 30.0F), foundation::NanPoint::zero()});
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::press,
        foundation::NanPoint(20.0F, 30.0F)
    });
    REQUIRE(button->pressed());
    tree.dispatch_mouse_button(scene::MouseButtonEvent {
        scene::MouseButtonEvent::Button::left,
        scene::MouseButtonEvent::Action::release,
        foundation::NanPoint(20.0F, 30.0F)
    });
    REQUIRE(clicks == 1);
}

TEST_CASE("button forwards text state through its text primitive", "[widget][button][text]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("A very long button label");
    button->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 72.0F, button->height()));
    tree.set_root(button);

    tree.draw(dev);

    REQUIRE(button->text_node().font_size() == Catch::Approx(button->resolved_style().font_size));
    REQUIRE(button->text_node().layout_result().overflowed);
    REQUIRE(dev.texts.size() == 1);
    REQUIRE(dev.texts[0].text.ends_with("..."));
}

TEST_CASE("button text overflow controls its internal text primitive", "[widget][button][text]") {
    RecordingDevice dev;
    scene::NanSceneTree tree;
    auto button = std::make_shared<widget::Button>("A very long button label");
    button->set_text_overflow(widget::primitives::TextOverflow::clip);
    button->layout_to(foundation::NanRect::from_xywh(0.0F, 0.0F, 72.0F, button->height()));
    tree.set_root(button);

    tree.draw(dev);

    REQUIRE(button->text_overflow() == widget::primitives::TextOverflow::clip);
    REQUIRE(button->text_node().overflow() == widget::primitives::TextOverflow::clip);
    REQUIRE(button->text_node().layout_result().overflowed);
    REQUIRE(dev.texts.size() == 1);
    REQUIRE_FALSE(dev.texts[0].text.ends_with("..."));
}
