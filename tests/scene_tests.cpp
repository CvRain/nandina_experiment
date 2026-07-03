//
// Scene-layer unit tests (Catch2 v3).
// Headless: scene/ never calls raylib, so these run without a window.
//

#include <catch2/catch_test_macros.hpp>

#include "scene/input_event.hpp"
#include "scene/node2d.hpp"
#include "scene/scene_tree.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{

/// Records lifecycle + input callbacks into a shared trace for order assertions.
class ProbeNode : public scene::NanNode2D {
public:
    ProbeNode(std::vector<std::string>* trace, std::string tag, bool focusable = false)
        : trace_(trace), tag_(std::move(tag)), focusable_(focusable) {}

    [[nodiscard]] auto contains_point(foundation::NanPoint local) const -> bool override {
        // Unit box centered at origin (half-extent 10) for simple hit tests.
        return local.get_x() >= -10 && local.get_x() <= 10
            && local.get_y() >= -10 && local.get_y() <= 10;
    }

    [[nodiscard]] auto global_bounds() const -> foundation::NanRect override {
        const auto p = global_position();
        return foundation::NanRect::from_xywh(p.get_x() - 10, p.get_y() - 10, 20, 20);
    }

    [[nodiscard]] auto is_focusable() const -> bool override { return focusable_; }

    void on_enter_tree() override { NanNode2D::on_enter_tree(); log("enter"); }
    void on_ready() override { log("ready"); }
    void on_exit_tree() override { NanNode2D::on_exit_tree(); log("exit"); }

    auto on_input(scene::InputEvent& e) -> bool override {
        switch (e.type()) {
        case scene::EventType::mouse_enter: log("hover_enter"); break;
        case scene::EventType::mouse_leave: log("hover_leave"); break;
        case scene::EventType::focus_enter: log("focus_enter"); break;
        case scene::EventType::focus_leave: log("focus_leave"); break;
        case scene::EventType::mouse_wheel: {
            const auto& w = static_cast<const scene::MouseWheelEvent&>(e);
            log("wheel:" + std::to_string(static_cast<int>(w.delta().get_y())));
            break;
        }
        case scene::EventType::text_input: {
            const auto& t = static_cast<const scene::TextInputEvent&>(e);
            log("text:" + std::string(t.text()));
            break;
        }
        default: break;
        }
        return false;
    }

    std::function<void(ProbeNode&)> on_enter_hook;

protected:
    void log(const std::string& ev) {
        if (trace_) { trace_->push_back(tag_ + ":" + ev); }
    }

private:
    std::vector<std::string>* trace_;
    std::string tag_;
    bool focusable_;
};

/// ProbeNode variant that runs a hook during on_enter_tree (P0 regression).
class HookNode : public ProbeNode {
public:
    using ProbeNode::ProbeNode;
    void on_enter_tree() override {
        ProbeNode::on_enter_tree();
        if (on_enter_hook) { on_enter_hook(*this); }
    }
};

auto count(const std::vector<std::string>& trace, const std::string& what) -> int {
    int n = 0;
    for (const auto& s : trace) { if (s == what) { ++n; } }
    return n;
}

auto index_of(const std::vector<std::string>& trace, const std::string& what) -> long {
    for (size_t i = 0; i < trace.size(); ++i) { if (trace[i] == what) { return static_cast<long>(i); } }
    return -1;
}

} // namespace

TEST_CASE("lifecycle callbacks fire in the correct order", "[scene][lifecycle]") {
    std::vector<std::string> trace;

    auto root = std::make_shared<ProbeNode>(&trace, "root");
    auto child = std::make_shared<ProbeNode>(&trace, "child");
    auto grandchild = std::make_shared<ProbeNode>(&trace, "gc");
    child->add_child(grandchild);
    root->add_child(child);

    scene::NanSceneTree tree;
    tree.set_root(root);

    SECTION("enter_tree is top-down") {
        REQUIRE(index_of(trace, "root:enter") < index_of(trace, "child:enter"));
        REQUIRE(index_of(trace, "child:enter") < index_of(trace, "gc:enter"));
    }
    SECTION("ready is bottom-up") {
        REQUIRE(index_of(trace, "gc:ready") < index_of(trace, "child:ready"));
        REQUIRE(index_of(trace, "child:ready") < index_of(trace, "root:ready"));
    }
    SECTION("each callback fires exactly once") {
        REQUIRE(count(trace, "root:ready") == 1);
        REQUIRE(count(trace, "gc:enter") == 1);
    }
}

TEST_CASE("adding a child during on_enter_tree does not double-fire (P0)", "[scene][lifecycle][regression]") {
    std::vector<std::string> trace;

    auto root = std::make_shared<HookNode>(&trace, "root");
    root->on_enter_hook = [&trace](ProbeNode& self) {
        self.add_child(std::make_shared<ProbeNode>(&trace, "late"));
    };

    scene::NanSceneTree tree;
    tree.set_root(root);

    REQUIRE(count(trace, "late:enter") == 1);
    REQUIRE(count(trace, "late:ready") == 1);
}

TEST_CASE("hover tracking sends one enter and one leave", "[scene][hover]") {
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto a = std::make_shared<ProbeNode>(nullptr, "a");
    std::vector<std::string> trace;
    auto b = std::make_shared<ProbeNode>(&trace, "b");
    a->set_position(foundation::NanPoint(50, 50));
    b->set_position(foundation::NanPoint(200, 200));
    root->add_child(a);
    root->add_child(b);

    scene::NanSceneTree tree;
    tree.set_root(root);

    tree.dispatch_mouse_move(scene::MouseMoveEvent(foundation::NanPoint(200, 200), foundation::NanPoint(0, 0)));
    REQUIRE(tree.hovered_node() == b.get());
    REQUIRE(count(trace, "b:hover_enter") == 1);

    tree.dispatch_mouse_move(scene::MouseMoveEvent(foundation::NanPoint(500, 500), foundation::NanPoint(0, 0)));
    REQUIRE(tree.hovered_node() == nullptr);
    REQUIRE(count(trace, "b:hover_leave") == 1);
}

TEST_CASE("focus traversal skips non-focusable and wraps", "[scene][focus]") {
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto f1 = std::make_shared<ProbeNode>(nullptr, "f1", true);
    auto f2 = std::make_shared<ProbeNode>(nullptr, "f2", true);
    auto plain = std::make_shared<ProbeNode>(nullptr, "plain", false);
    root->add_child(f1);
    root->add_child(plain);
    root->add_child(f2);

    scene::NanSceneTree tree;
    tree.set_root(root);

    tree.focus_next();
    REQUIRE(tree.focused_node() == f1.get());
    tree.focus_next();
    REQUIRE(tree.focused_node() == f2.get());  // skips non-focusable plain
    tree.focus_next();
    REQUIRE(tree.focused_node() == f1.get());  // wraps
    tree.focus_previous();
    REQUIRE(tree.focused_node() == f2.get());  // wraps back
}

TEST_CASE("queue_delete dedups descendants and never dangles", "[scene][delete]") {
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    std::weak_ptr<scene::NanNode2D> parentw;
    std::weak_ptr<scene::NanNode2D> childw;
    {
        auto parent = std::make_shared<ProbeNode>(nullptr, "parent");
        auto child = std::make_shared<ProbeNode>(nullptr, "child");
        parentw = parent;
        childw = child;
        parent->add_child(child);
        root->add_child(parent);  // root becomes the sole strong owner
    }

    scene::NanSceneTree tree;
    tree.set_root(root);

    SECTION("ancestor request absorbs descendant request") {
        tree.queue_delete(*parentw.lock());
        tree.queue_delete(*childw.lock());  // deduped under parent

        REQUIRE_FALSE(childw.expired());
        tree.process(0.016F);  // flush
        REQUIRE(childw.expired());
        REQUIRE(parentw.expired());
        REQUIRE(root->child_count() == 0);
    }

    SECTION("node destroyed by another path before flush is skipped safely") {
        auto lone = std::make_shared<ProbeNode>(nullptr, "lone");
        root->add_child(lone);
        tree.queue_delete(*lone);
        root->remove_and_delete(*lone);  // gone before flush
        REQUIRE_NOTHROW(tree.process(0.016F));  // expired weak_ptr must not crash
        REQUIRE(root->child_count() == 1);  // only 'parent' subtree remains
    }
}

TEST_CASE("removed and re-added node readies again", "[scene][lifecycle]") {
    std::vector<std::string> trace;
    auto root = std::make_shared<ProbeNode>(&trace, "root");
    auto n = std::make_shared<ProbeNode>(&trace, "n");
    root->add_child(n);

    scene::NanSceneTree tree;
    tree.set_root(root);
    REQUIRE(count(trace, "n:ready") == 1);

    auto detached = root->remove_child(*n);
    REQUIRE(count(trace, "n:exit") == 1);
    root->add_child(detached);
    REQUIRE(count(trace, "n:ready") == 2);
}

TEST_CASE("mouse wheel dispatches to hovered node and bubbles", "[scene][wheel]") {
    std::vector<std::string> parent_trace;
    std::vector<std::string> child_trace;
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto parent = std::make_shared<ProbeNode>(&parent_trace, "parent");
    auto child = std::make_shared<ProbeNode>(&child_trace, "child");
    parent->set_position(foundation::NanPoint(100, 100));
    // child sits at same world position as parent (local origin), so hit-test
    // lands on child (deepest) and the wheel event bubbles up to parent.
    parent->add_child(child);
    root->add_child(parent);

    scene::NanSceneTree tree;
    tree.set_root(root);

    SECTION("falls back to hit-test when nothing hovered") {
        tree.dispatch_mouse_wheel(scene::MouseWheelEvent(
            foundation::NanPoint(100, 100), foundation::NanPoint(0, 3)));
        REQUIRE(count(child_trace, "child:wheel:3") == 1);
        REQUIRE(count(parent_trace, "parent:wheel:3") == 1);  // bubbled to parent
    }

    SECTION("uses current hover target") {
        tree.dispatch_mouse_move(scene::MouseMoveEvent(
            foundation::NanPoint(100, 100), foundation::NanPoint(0, 0)));
        REQUIRE(tree.hovered_node() == child.get());
        tree.dispatch_mouse_wheel(scene::MouseWheelEvent(
            foundation::NanPoint(100, 100), foundation::NanPoint(0, -2)));
        REQUIRE(count(child_trace, "child:wheel:-2") == 1);
    }
}

TEST_CASE("text input dispatches to the focused node", "[scene][text]") {
    std::vector<std::string> trace;
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto field = std::make_shared<ProbeNode>(&trace, "field", true);
    root->add_child(field);

    scene::NanSceneTree tree;
    tree.set_root(root);

    SECTION("no focus: text input is dropped") {
        tree.dispatch_text_input(scene::TextInputEvent("hi"));
        REQUIRE(count(trace, "field:text:hi") == 0);
    }

    SECTION("with focus: text reaches the focused node") {
        tree.set_focus(field.get());
        tree.dispatch_text_input(scene::TextInputEvent("hi"));
        REQUIRE(count(trace, "field:text:hi") == 1);
    }
}

TEST_CASE("modifiers propagate through button and key events", "[scene][modifiers]") {
    scene::KeyModifiers mods;
    mods.ctrl = true;
    mods.shift = true;

    scene::KeyEvent key(65, scene::KeyEvent::Action::press, mods);
    REQUIRE(key.modifiers().ctrl);
    REQUIRE(key.modifiers().shift);
    REQUIRE_FALSE(key.modifiers().alt);
    REQUIRE(key.modifiers().any());

    scene::MouseButtonEvent btn(scene::MouseButtonEvent::Button::left,
                                scene::MouseButtonEvent::Action::press,
                                foundation::NanPoint(0, 0), mods);
    REQUIRE(btn.modifiers().ctrl);
    REQUIRE(btn.type() == scene::EventType::mouse_button);
}
