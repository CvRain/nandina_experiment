//
// Animation easing + tween tests.
//

#include "animation/animated_property.hpp"
#include "animation/animation_host.hpp"
#include "animation/behavior.hpp"
#include "animation/easing.hpp"
#include "animation/tween.hpp"
#include "foundation/nandina_color.hpp"
#include "reactive/scope.hpp"
#include "reactive/signal.hpp"
#include "scene/control.hpp"
#include "scene/scene_tree.hpp"
#include "theme/theme_manager.hpp"
#include "widget/build_context.hpp"
#include "widget/builtin_component_traits.hpp"
#include "widget/button.hpp"
#include "widget/label.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <stdexcept>

using namespace nandina;

namespace
{
    class AnimatedProbe final: public scene::NanControl {
    public:
        animation::AnimatedProperty<float> paint_value {0.0F};
        animation::AnimatedProperty<float> layout_value {0.0F};
    };

    void advance(scene::NanSceneTree& tree, const float dt) {
        auto phase = tree.enter_phase(scene::FramePhase::animation);
        tree.advance_animations(dt);
    }

    constexpr auto all_dirty_flags = scene::DirtyFlags::measure | scene::DirtyFlags::layout
        | scene::DirtyFlags::paint | scene::DirtyFlags::semantics;
} // namespace

static_assert(
    static_cast<int>(scene::FramePhase::reactive) < static_cast<int>(scene::FramePhase::animation)
);
static_assert(
    static_cast<int>(scene::FramePhase::animation) < static_cast<int>(scene::FramePhase::layout)
);
static_assert(widget::property::Animatable<widget::Label, decltype(widget::visual::label.color)>);
static_assert(
    widget::property::Animatable<widget::Button, decltype(widget::visual::container.radius)>
);
static_assert(
    !widget::property::Animatable<widget::Label, decltype(widget::visual::container.radius)>
);

TEST_CASE("easing curves map 0->0 and 1->1", "[animation][easing]") {
    for (const auto easing:
         {animation::Easing::linear,
          animation::Easing::ease_in,
          animation::Easing::ease_out,
          animation::Easing::ease_in_out})
    {
        REQUIRE(animation::ease(easing, 0.0F) == Catch::Approx(0.0F));
        REQUIRE(animation::ease(easing, 1.0F) == Catch::Approx(1.0F));
    }
}

TEST_CASE("easing curves stay within [0,1] and ease-in lags", "[animation][easing]") {
    const float mid = animation::ease(animation::Easing::ease_in, 0.5F);
    REQUIRE(mid > 0.0F);
    REQUIRE(mid < 0.5F); // ease-in 在中点落后于线性。

    const float out = animation::ease(animation::Easing::ease_out, 0.5F);
    REQUIRE(out > 0.5F); // ease-out 在中点超前于线性。

    const float in_out = animation::ease(animation::Easing::ease_in_out, 0.5F);
    REQUIRE(in_out == Catch::Approx(0.5F)); // ease-in-out 中点正好一半。
}

TEST_CASE("tween advances from start to end with easing", "[animation][tween]") {
    animation::Tween<float> tween;
    tween.start(0.0F, 100.0F, 1.0F, animation::Easing::linear);

    REQUIRE_FALSE(tween.is_finished());
    REQUIRE(tween.value() == Catch::Approx(0.0F));

    REQUIRE(tween.tick(0.25F) == Catch::Approx(25.0F));
    REQUIRE(tween.tick(0.25F) == Catch::Approx(50.0F));
    REQUIRE(tween.tick(0.5F) == Catch::Approx(100.0F));
    REQUIRE(tween.is_finished());
    REQUIRE(tween.progress() == Catch::Approx(1.0F));
}

TEST_CASE("tween zero duration finishes immediately at target", "[animation][tween]") {
    animation::Tween<float> tween;
    tween.start(3.0F, 9.0F, 0.0F);
    REQUIRE(tween.is_finished());
    REQUIRE(tween.value() == Catch::Approx(9.0F));
}

TEST_CASE("tween finish and reset jump without animation", "[animation][tween]") {
    animation::Tween<float> tween(0.0F);
    tween.start(0.0F, 10.0F, 1.0F);
    (void)tween.tick(0.2F);
    REQUIRE_FALSE(tween.is_finished());

    tween.finish();
    REQUIRE(tween.is_finished());
    REQUIRE(tween.value() == Catch::Approx(10.0F));

    tween.reset(42.0F);
    REQUIRE(tween.is_finished());
    REQUIRE(tween.value() == Catch::Approx(42.0F));
}

TEST_CASE("tween clamps dt overshoot and reuses target", "[animation][tween]") {
    animation::Tween<float> tween;
    tween.start(0.0F, 4.0F, 1.0F, animation::Easing::linear);
    (void)tween.tick(5.0F); // 远超时长
    REQUIRE(tween.is_finished());
    REQUIRE(tween.value() == Catch::Approx(4.0F));
}

TEST_CASE("tween ignores negative and NaN dt", "[animation][tween]") {
    animation::Tween<float> tween;
    tween.start(0.0F, 10.0F, 1.0F, animation::Easing::linear);

    REQUIRE(tween.tick(-0.5F) == Catch::Approx(0.0F));
    REQUIRE(tween.tick(std::numeric_limits<float>::quiet_NaN()) == Catch::Approx(0.0F));
    REQUIRE_FALSE(tween.is_finished());
}

TEST_CASE("color tween interpolates OKLCH hue over the shortest arc", "[animation][color]") {
    const auto from = foundation::NanColor::from_oklch(0.4F, 0.1F, 350.0F, 0.2F);
    const auto to = foundation::NanColor::from_oklch(0.8F, 0.3F, 10.0F, 1.0F);
    animation::Tween<foundation::NanColor> tween;
    tween.start(from, to, 1.0F, animation::Easing::linear);

    const auto mid = tween.tick(0.5F).oklch();
    REQUIRE(mid.light == Catch::Approx(0.6F));
    REQUIRE(mid.chroma == Catch::Approx(0.2F));
    REQUIRE(mid.hue == Catch::Approx(0.0F).margin(0.001F));
    REQUIRE(mid.alpha == Catch::Approx(0.6F));
}

TEST_CASE(
    "animated property jumps until an enabled behavior is installed",
    "[animation][property]"
) {
    animation::AnimatedProperty<float> property(2.0F);
    property.set_target(8.0F);
    REQUIRE(property.target() == Catch::Approx(8.0F));
    REQUIRE(property.value() == Catch::Approx(8.0F));
    REQUIRE_FALSE(property.is_animating());

    property.set_behavior(animation::Behavior<float>(1.0F).set_enabled(false));
    property.set_target(12.0F);
    REQUIRE(property.value() == Catch::Approx(12.0F));

    property.set_behavior(animation::Behavior<float>(0.0F));
    property.set_target(16.0F);
    REQUIRE(property.value() == Catch::Approx(16.0F));
}

TEST_CASE("animated property separates target and presentation value", "[animation][property]") {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    property.set_target(10.0F);

    REQUIRE(property.target() == Catch::Approx(10.0F));
    REQUIRE(property.value() == Catch::Approx(0.0F));
    REQUIRE(property.is_animating());
    REQUIRE(property.tick(0.25F) == Catch::Approx(2.5F));
    REQUIRE(property.progress() == Catch::Approx(0.25F));
}

TEST_CASE(
    "animated property retargets continuously from its current value",
    "[animation][property]"
) {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    property.set_target(10.0F);
    REQUIRE(property.tick(0.5F) == Catch::Approx(5.0F));

    property.set_target(15.0F);
    REQUIRE(property.value() == Catch::Approx(5.0F));
    REQUIRE(property.target() == Catch::Approx(15.0F));
    REQUIRE(property.tick(0.5F) == Catch::Approx(10.0F));
    REQUIRE(property.tick(5.0F) == Catch::Approx(15.0F));
    REQUIRE_FALSE(property.is_animating());
}

TEST_CASE("writing the same target does not restart an active property", "[animation][property]") {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    property.set_target(10.0F);
    REQUIRE(property.tick(0.25F) == Catch::Approx(2.5F));

    property.set_target(10.0F);
    REQUIRE(property.tick(0.25F) == Catch::Approx(5.0F));
}

TEST_CASE("clearing behavior finishes the current property transition", "[animation][property]") {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    property.set_target(10.0F);
    (void)property.tick(0.25F);

    property.clear_behavior();
    REQUIRE(property.value() == Catch::Approx(10.0F));
    REQUIRE_FALSE(property.is_animating());
    REQUIRE_FALSE(property.behavior().has_value());
}

TEST_CASE("disabling behavior finishes an active property transition", "[animation][property]") {
    animation::AnimatedProperty<float> property(0.0F);
    property.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    property.set_target(10.0F);
    (void)property.tick(0.25F);

    property.set_behavior(animation::Behavior<float>(1.0F).set_enabled(false));
    REQUIRE(property.value() == Catch::Approx(10.0F));
    REQUIRE_FALSE(property.is_animating());
}

TEST_CASE("behavior rejects invalid durations", "[animation][behavior]") {
    REQUIRE_THROWS_AS(animation::Behavior<float>(-0.1F), std::invalid_argument);
    REQUIRE_THROWS_AS(
        animation::Behavior<float>(std::numeric_limits<float>::infinity()),
        std::invalid_argument
    );
    REQUIRE_THROWS_AS(
        animation::Behavior<float>(std::numeric_limits<float>::quiet_NaN()),
        std::invalid_argument
    );
}

TEST_CASE("animation host advances only active properties with manual dt", "[animation][host]") {
    scene::NanSceneTree tree;
    auto probe = std::make_shared<AnimatedProbe>();
    tree.set_root(probe);
    probe->paint_value.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    probe->clear_dirty(all_dirty_flags);

    tree.animation_host().set_target(*probe, probe->paint_value, 10.0F, scene::DirtyFlags::paint);
    REQUIRE(tree.animation_host().active_count() == 1);
    REQUIRE(probe->paint_value.target() == Catch::Approx(10.0F));
    REQUIRE(probe->paint_value.value() == Catch::Approx(0.0F));
    REQUIRE_FALSE(probe->is_dirty(scene::DirtyFlags::paint));

    advance(tree, 0.25F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(2.5F));
    REQUIRE(probe->is_dirty(scene::DirtyFlags::paint));
    REQUIRE_FALSE(probe->is_dirty(scene::layout_dirty_flags));
    REQUIRE_FALSE(probe->is_dirty(scene::DirtyFlags::semantics));

    probe->clear_dirty(all_dirty_flags);
    advance(tree, 5.0F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(10.0F));
    REQUIRE(tree.animation_host().active_count() == 0);
    REQUIRE(probe->is_dirty(scene::DirtyFlags::paint));

    probe->clear_dirty(all_dirty_flags);
    advance(tree, 0.5F);
    REQUIRE_FALSE(probe->is_dirty(scene::DirtyFlags::paint));
}

TEST_CASE("animation host retargets one property without duplicate tracks", "[animation][host]") {
    scene::NanSceneTree tree;
    auto probe = std::make_shared<AnimatedProbe>();
    tree.set_root(probe);
    probe->paint_value.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));

    tree.animation_host().set_target(*probe, probe->paint_value, 10.0F, scene::DirtyFlags::paint);
    advance(tree, 0.5F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(5.0F));

    tree.animation_host().set_target(*probe, probe->paint_value, 20.0F, scene::DirtyFlags::paint);
    REQUIRE(tree.animation_host().active_count() == 1);
    REQUIRE(probe->paint_value.value() == Catch::Approx(5.0F));
    advance(tree, 0.5F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(12.5F));
}

TEST_CASE("animation host applies immediate targets and exact dirty flags", "[animation][host]") {
    scene::NanSceneTree tree;
    auto root = std::make_shared<scene::NanControl>();
    auto probe = std::make_shared<AnimatedProbe>();
    root->add_child(probe);
    tree.set_root(root);
    (void)tree.update_semantics();
    REQUIRE_FALSE(tree.semantics_dirty());
    root->clear_dirty(all_dirty_flags);
    probe->clear_dirty(all_dirty_flags);

    tree.animation_host().set_target(
        *probe,
        probe->paint_value,
        4.0F,
        scene::DirtyFlags::paint | scene::DirtyFlags::semantics
    );
    REQUIRE(probe->paint_value.value() == Catch::Approx(4.0F));
    REQUIRE(tree.animation_host().active_count() == 0);
    REQUIRE(probe->is_dirty(scene::DirtyFlags::paint));
    REQUIRE(probe->is_dirty(scene::DirtyFlags::semantics));
    REQUIRE_FALSE(probe->is_dirty(scene::layout_dirty_flags));
    REQUIRE_FALSE(root->is_dirty(scene::layout_dirty_flags));
    REQUIRE(tree.semantics_dirty());

    probe->layout_value.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    root->clear_dirty(all_dirty_flags);
    probe->clear_dirty(all_dirty_flags);
    tree.animation_host().set_target(
        *probe,
        probe->layout_value,
        8.0F,
        scene::layout_dirty_flags | scene::DirtyFlags::paint
    );
    advance(tree, 0.25F);
    REQUIRE(probe->is_dirty(scene::DirtyFlags::measure));
    REQUIRE(probe->is_dirty(scene::DirtyFlags::layout));
    REQUIRE(probe->is_dirty(scene::DirtyFlags::paint));
    REQUIRE_FALSE(probe->is_dirty(scene::DirtyFlags::semantics));
    REQUIRE(root->is_dirty(scene::DirtyFlags::measure));
    REQUIRE(root->is_dirty(scene::DirtyFlags::layout));
}

TEST_CASE("animation host cancels tracks when an owner exits the tree", "[animation][host]") {
    scene::NanSceneTree tree;
    auto root = std::make_shared<scene::NanControl>();
    auto probe = std::make_shared<AnimatedProbe>();
    root->add_child(probe);
    tree.set_root(root);
    probe->paint_value.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    tree.animation_host().set_target(*probe, probe->paint_value, 10.0F, scene::DirtyFlags::paint);
    advance(tree, 0.25F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(2.5F));
    REQUIRE(tree.animation_host().active_count() == 1);

    auto detached = root->remove_child(*probe);
    REQUIRE(detached == probe);
    REQUIRE_FALSE(probe->is_inside_tree());
    REQUIRE(tree.animation_host().active_count() == 0);
    REQUIRE_FALSE(probe->paint_value.is_animating());
    REQUIRE(probe->paint_value.value() == Catch::Approx(10.0F));

    advance(tree, 1.0F);
    REQUIRE(probe->paint_value.value() == Catch::Approx(10.0F));
}

TEST_CASE("animation host rejects owners from another scene tree", "[animation][host]") {
    scene::NanSceneTree first;
    scene::NanSceneTree second;
    auto probe = std::make_shared<AnimatedProbe>();
    first.set_root(probe);

    REQUIRE_THROWS_AS(
        second.animation_host()
            .set_target(*probe, probe->paint_value, 1.0F, scene::DirtyFlags::paint),
        std::invalid_argument
    );
}

TEST_CASE(
    "animation phase defers tree mutation and host clear finishes tracks",
    "[animation][host]"
) {
    scene::NanSceneTree tree;
    auto probe = std::make_shared<AnimatedProbe>();
    tree.set_root(probe);
    probe->paint_value.set_behavior(animation::Behavior<float>(1.0F, animation::Easing::linear));
    tree.animation_host().set_target(*probe, probe->paint_value, 10.0F, scene::DirtyFlags::paint);
    advance(tree, 0.25F);
    probe->clear_dirty(all_dirty_flags);

    {
        auto phase = tree.enter_phase(scene::FramePhase::animation);
        REQUIRE(tree.defers_tree_mutation());
        tree.animation_host().clear();
    }

    REQUIRE(tree.animation_host().active_count() == 0);
    REQUIRE_FALSE(probe->paint_value.is_animating());
    REQUIRE(probe->paint_value.value() == Catch::Approx(10.0F));
    REQUIRE(probe->is_dirty(scene::DirtyFlags::paint));
}

TEST_CASE(
    "builder bindings and behaviors share scene-owned property endpoints",
    "[animation][authoring][endpoint]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope {graph};
    theme::ThemeManager themes;
    widget::BuildContext ui {graph, scope, themes};

    const auto initial_color = foundation::NanColor::from_oklch(0.2F, 0.1F, 40.0F);
    const auto target_color = foundation::NanColor::from_oklch(0.8F, 0.1F, 40.0F);
    reactive::Signal<foundation::NanColor> color {graph, initial_color};
    reactive::Signal<float> radius {graph, 4.0F};

    auto label = ui.make<widget::Label>("Animated label")
                     .bind(widget::visual::label.color, color)
                     .behavior(
                         widget::visual::label.color,
                         animation::Behavior<foundation::NanColor>(1.0F, animation::Easing::linear)
                     )
                     .build();
    auto button = ui.make<widget::Button>("Animated button")
                      .behavior(
                          widget::visual::container.radius,
                          animation::Behavior<float>(1.0F, animation::Easing::linear)
                      )
                      .bind(widget::visual::container.radius, radius)
                      .build();

    auto root = std::make_shared<scene::NanControl>();
    root->add_child(label);
    root->add_child(button);
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(root);

    const auto initial_label_endpoint = label->property(widget::visual::color_t {}).value();
    REQUIRE(initial_label_endpoint != nullptr);
    REQUIRE(initial_label_endpoint->approx_equals(initial_color));
    REQUIRE(button->resolved_style().container.radius == Catch::Approx(4.0F));
    REQUIRE(tree.animation_host().active_count() == 0);

    color.set(target_color);
    radius.set(20.0F);
    REQUIRE(tree.animation_host().active_count() == 2);
    REQUIRE(label->color().approx_equals(target_color));
    REQUIRE(
        button->visual_part(widget::visual::container_t {})
            .property(widget::visual::radius_t {})
            .target()
        != nullptr
    );
    REQUIRE(
        *button->visual_part(widget::visual::container_t {})
             .property(widget::visual::radius_t {})
             .target()
        == Catch::Approx(20.0F)
    );

    advance(tree, 0.5F);
    const auto midpoint = label->property(widget::visual::color_t {}).value()->oklch();
    REQUIRE(midpoint.light == Catch::Approx(0.5F));
    REQUIRE(button->resolved_style().container.radius == Catch::Approx(12.0F));

    label->set_color(initial_color);
    widget::property::write(*button, widget::visual::container.radius, 8.0F);
    REQUIRE(tree.animation_host().active_count() == 2);
    REQUIRE(label->color().approx_equals(initial_color));
    REQUIRE(
        *button->visual_part(widget::visual::container_t {})
             .property(widget::visual::radius_t {})
             .target()
        == Catch::Approx(8.0F)
    );

    advance(tree, 1.0F);
    REQUIRE(label->property(widget::visual::color_t {}).value()->approx_equals(initial_color));
    REQUIRE(button->resolved_style().container.radius == Catch::Approx(8.0F));
    REQUIRE(tree.animation_host().active_count() == 0);

    const auto ignored_color = foundation::NanColor::from_oklch(0.6F, 0.2F, 180.0F);
    scope.clear();
    color.set(ignored_color);
    radius.set(30.0F);
    REQUIRE(label->color().approx_equals(initial_color));
    REQUIRE(
        *button->visual_part(widget::visual::container_t {})
             .property(widget::visual::radius_t {})
             .target()
        == Catch::Approx(8.0F)
    );
}

TEST_CASE(
    "property endpoints reconcile behavior changes and cleared overrides",
    "[animation][endpoint][lifecycle]"
) {
    reactive::Graph graph;
    auto label = widget::Label::create(graph, "Label");
    auto button = widget::Button::create("Button");
    button->set_font_size(12.0F);
    widget::property::set_behavior(
        *label,
        widget::visual::label.color,
        animation::Behavior<foundation::NanColor>(1.0F, animation::Easing::linear)
    );
    widget::property::set_behavior(
        *button,
        widget::visual::label.font_size,
        animation::Behavior<float>(1.0F, animation::Easing::linear)
    );

    auto root = std::make_shared<scene::NanControl>();
    root->add_child(label);
    root->add_child(button);
    scene::NanSceneTree tree;
    tree.set_root(root);

    const auto target_color = foundation::NanColor::from_oklch(0.7F, 0.2F, 250.0F);
    label->set_color(target_color);
    button->set_font_size(24.0F);
    REQUIRE(tree.animation_host().active_count() == 2);
    advance(tree, 0.25F);

    widget::property::set_behavior(
        *label,
        widget::visual::label.color,
        animation::Behavior<foundation::NanColor>(1.0F).set_enabled(false)
    );
    REQUIRE(label->property(widget::visual::color_t {}).value()->approx_equals(target_color));
    REQUIRE(tree.animation_host().active_count() == 1);

    button->clear_font_size();
    REQUIRE(tree.animation_host().active_count() == 0);
    advance(tree, 1.0F);
    REQUIRE(tree.animation_host().active_count() == 0);
}
