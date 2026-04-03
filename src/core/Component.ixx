module;

#include <memory>

export module Nandina.Core.Component;

import Nandina.Core.Color;
import Nandina.Core.Widget;
import Nandina.Reactive;

export namespace Nandina {

    // ── Component ─────────────────────────────────────────────────────────────
    // Business-logic base class. Adds an EffectScope so reactive Effects
    // registered inside a component are automatically cleaned up on destruction.
    class Component : public Widget {
    public:
        using Ptr = std::unique_ptr<Component>;
        ~Component() override = default;

    protected:
        EffectScope scope_;
    };

    // ── RectangleComponent ────────────────────────────────────────────────────
    // A plain filled rectangle with optional rounded corners.
    // Used as a layered building block inside composite components.
    class RectangleComponent final : public Component {
    public:
        static auto Create() -> std::unique_ptr<RectangleComponent> {
            return std::make_unique<RectangleComponent>();
        }

        auto color(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                   std::uint8_t a = 255) -> RectangleComponent& {
            set_background(r, g, b, a);
            return *this;
        }

        auto radius(float r) -> RectangleComponent& {
            set_border_radius(r);
            return *this;
        }
    };

    // ── FocusComponent ────────────────────────────────────────────────────────
    // Renders an optional focus-ring overlay around its parent bounds.
    class FocusComponent final : public Component {
    public:
        static auto Create() -> std::unique_ptr<FocusComponent> {
            return std::make_unique<FocusComponent>();
        }

        auto focused(bool value) -> FocusComponent& {
            focused_ = value;
            if (focused_) {
                set_background(59, 130, 246, 90);
            } else {
                set_background(0, 0, 0, 0);
            }
            return *this;
        }

    private:
        bool focused_ = false;
    };

} // export namespace Nandina
