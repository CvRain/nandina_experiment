//
// Animation easing + tween tests.
//

#include "animation/animated_property.hpp"
#include "animation/behavior.hpp"
#include "animation/easing.hpp"
#include "animation/tween.hpp"
#include "foundation/nandina_color.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <stdexcept>

using namespace nandina;

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
