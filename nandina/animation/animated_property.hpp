//
// animation/animated_property - logical target plus current presentation value.
//

#ifndef NANDINA_EXPERIMENT_ANIMATION_ANIMATED_PROPERTY_HPP
#define NANDINA_EXPERIMENT_ANIMATION_ANIMATED_PROPERTY_HPP

#include "behavior.hpp"
#include "tween.hpp"

#include <concepts>
#include <optional>
#include <utility>

namespace nandina::animation
{
    template<typename T>
        requires std::copyable<T> && std::equality_comparable<T>
    class AnimatedProperty {
    public:
        explicit AnimatedProperty(T initial = {}): target_(initial), tween_(std::move(initial)) {}

        void set_target(T target) {
            if (target_ == target) {
                return;
            }
            target_ = std::move(target);
            if (!behavior_ || !behavior_->enabled() || behavior_->duration() == 0.0F) {
                tween_.reset(target_);
                return;
            }
            tween_.start(tween_.value(), target_, behavior_->duration(), behavior_->easing());
        }

        [[nodiscard]] auto target() const noexcept -> const T& {
            return target_;
        }

        [[nodiscard]] auto value() const noexcept -> const T& {
            return tween_.value();
        }

        void set_behavior(Behavior<T> behavior) {
            behavior_ = std::move(behavior);
            if (!is_animating()) {
                return;
            }
            if (!behavior_->enabled() || behavior_->duration() == 0.0F) {
                tween_.reset(target_);
                return;
            }
            tween_.start(tween_.value(), target_, behavior_->duration(), behavior_->easing());
        }

        void clear_behavior() {
            behavior_.reset();
            tween_.reset(target_);
        }

        [[nodiscard]] auto behavior() const noexcept -> const std::optional<Behavior<T>>& {
            return behavior_;
        }

        [[nodiscard]] auto tick(const float dt) -> const T& {
            return tween_.tick(dt);
        }

        void finish() {
            tween_.finish();
        }

        [[nodiscard]] auto is_animating() const noexcept -> bool {
            return !tween_.is_finished();
        }

        [[nodiscard]] auto progress() const noexcept -> float {
            return tween_.progress();
        }

    private:
        T target_;
        Tween<T> tween_;
        std::optional<Behavior<T>> behavior_;
    };
} // namespace nandina::animation

#endif // NANDINA_EXPERIMENT_ANIMATION_ANIMATED_PROPERTY_HPP
