//
// Theme / primitives / Button tests.
//

#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "theme/button_style.hpp"
#include "widget/button.hpp"
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
