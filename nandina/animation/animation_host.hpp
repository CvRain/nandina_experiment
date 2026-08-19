//
// animation/animation_host - scene-owned scheduler for active animated properties.
//

#ifndef NANDINA_EXPERIMENT_ANIMATION_ANIMATION_HOST_HPP
#define NANDINA_EXPERIMENT_ANIMATION_ANIMATION_HOST_HPP

#include "../scene/control.hpp"
#include "animated_property.hpp"

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace nandina::scene
{
    class NanSceneTree;
}

namespace nandina::animation
{
    class AnimationHost {
    public:
        explicit AnimationHost(scene::NanSceneTree& tree) noexcept;

        AnimationHost(const AnimationHost&) = delete;
        auto operator=(const AnimationHost&) -> AnimationHost& = delete;

        template<typename T>
            requires std::copyable<T> && std::equality_comparable<T>
        void set_target(
            scene::NanControl& owner,
            AnimatedProperty<T>& property,
            T target,
            scene::DirtyFlags dirty_flags
        ) {
            if (owner.get_tree() != tree_) {
                throw std::invalid_argument("animation owner must belong to the host scene tree");
            }
            auto weak_owner = owner.weak_from_this();
            if (weak_owner.expired()) {
                throw std::logic_error("animation owner must be managed by shared_ptr");
            }

            const T previous = property.value();
            property.set_target(std::move(target));
            if (!(previous == property.value())) {
                owner.mark_dirty(dirty_flags);
            }

            const auto* identity = static_cast<const void*>(std::addressof(property));
            if (!property.is_animating()) {
                cancel_property(identity);
                return;
            }

            upsert(
                std::move(weak_owner),
                identity,
                [&property](const float dt) {
                    const T before = property.value();
                    (void)property.tick(dt);
                    return TickResult {
                        .changed = !(before == property.value()),
                        .active = property.is_animating(),
                    };
                },
                [&property] { property.finish(); },
                dirty_flags
            );
        }

        void advance(float dt);
        void cancel_owner(const scene::NanNode& owner);
        void clear();
        [[nodiscard]] auto active_count() const noexcept -> std::size_t;

    private:
        struct TickResult {
            bool changed = false;
            bool active = false;
        };

        struct Track {
            std::weak_ptr<scene::NanNode> owner;
            const void* identity = nullptr;
            std::function<TickResult(float)> tick;
            std::function<void()> finish;
            scene::DirtyFlags dirty_flags = scene::DirtyFlags::none;
        };

        void upsert(
            std::weak_ptr<scene::NanNode> owner,
            const void* identity,
            std::function<TickResult(float)> tick,
            std::function<void()> finish,
            scene::DirtyFlags dirty_flags
        );
        void cancel_property(const void* identity) noexcept;

        scene::NanSceneTree* tree_;
        std::vector<Track> tracks_;
    };
} // namespace nandina::animation

#endif // NANDINA_EXPERIMENT_ANIMATION_ANIMATION_HOST_HPP
