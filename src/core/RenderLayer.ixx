module;
#include <memory>
#include <utility>

export module Nandina.Core.RenderLayer;

import Nandina.Core.Widget;

export namespace Nandina {

// Single rendering layer: an independent Widget-tree root with its own
// rendering and event context.
//
// Corresponds to Godot's CanvasLayer:
//   - Higher index layers render on top of lower index layers.
//   - Events are dispatched from the highest layer down to the lowest.
//   - modal=true blocks all event dispatch to layers below this one.
struct RenderLayer {
    int  index   = 0;
    bool modal   = false;   // true: this layer and above handle events normally;
                            //       layers below are blocked.
    bool visible = true;

    // Root widget for this layer (usually fills the window).
    std::unique_ptr<Widget> root;

    [[nodiscard]] auto has_content() const noexcept -> bool {
        return root != nullptr;
    }

    // Take ownership of a widget and set it as (or add it as a child of) the root.
    // Returns a raw pointer for subsequent operations.
    template<typename T>
    auto add(std::unique_ptr<T> widget) -> T* {
        auto* ptr = widget.get();
        if (!root) {
            root = std::move(widget);
        } else {
            root->add_child(std::move(widget));
        }
        return ptr;
    }
};

} // namespace Nandina
