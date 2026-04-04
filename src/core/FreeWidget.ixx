module;
#include <memory>

export module Nandina.Core.FreeWidget;

import Nandina.Core.Widget;
import Nandina.Types.Position;
import Nandina.Types.Size;

export namespace Nandina {

// FreeWidget: position is owned by the widget itself, not its parent layout.
// Corresponds to Godot's Node2D: can be placed in any RenderLayer, but a
// parent container's layout() will not move it.
class FreeWidget : public Widget {
public:
    // Move to an absolute position (the only valid way to set position).
    auto move_to(float x, float y) noexcept -> FreeWidget& {
        x_ = x; y_ = y; mark_dirty(); return *this;
    }
    auto move_to(const Position& pos) noexcept -> FreeWidget& {
        return move_to(pos.x(), pos.y());
    }
    auto move_by(float dx, float dy) noexcept -> FreeWidget& {
        x_ += dx; y_ += dy; mark_dirty(); return *this;
    }

    // Resize (FreeWidget manages its own size independent of parent).
    auto resize(float w, float h) noexcept -> FreeWidget& {
        width_ = w; height_ = h; mark_dirty(); return *this;
    }
    auto resize(const Size& s) noexcept -> FreeWidget& {
        return resize(s.width(), s.height());
    }

protected:
    // Override: ignore set_bounds() calls from parent layout — position unchanged.
    auto set_bounds(float /*x*/, float /*y*/, float /*w*/, float /*h*/) noexcept -> Widget& override {
        return *this;
    }
};

} // namespace Nandina
