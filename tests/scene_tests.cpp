//
// Headless logic tests for the scene layer.
// No third-party framework: a tiny assertion harness keeps the test target
// dependency-free and runnable without a window (scene/ never calls raylib).
//

#include "scene/input_event.hpp"
#include "scene/node2d.hpp"
#include "scene/scene_tree.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{

int g_checks = 0;
int g_failures = 0;

void check(bool cond, const std::string& what) {
    ++g_checks;
    if (!cond) {
        ++g_failures;
        std::cerr << "  FAIL: " << what << "\n";
    }
}

/// Records lifecycle callbacks into a shared trace for order assertions.
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
        if (dynamic_cast<scene::MouseEnterEvent*>(&e)) { log("hover_enter"); }
        else if (dynamic_cast<scene::MouseLeaveEvent*>(&e)) { log("hover_leave"); }
        else if (dynamic_cast<scene::FocusEnterEvent*>(&e)) { log("focus_enter"); }
        else if (dynamic_cast<scene::FocusLeaveEvent*>(&e)) { log("focus_leave"); }
        return false;
    }

    // Optional: run a callback inside on_enter_tree to exercise reentrancy.
    std::function<void(ProbeNode&)> on_enter_hook;

protected:
    void log(const char* ev) {
        if (trace_) { trace_->push_back(tag_ + ":" + ev); }
    }

private:
    std::vector<std::string>* trace_;
    std::string tag_;
    bool focusable_;
};

/// ProbeNode variant that adds children during on_enter_tree (P0 regression).
class HookNode : public ProbeNode {
public:
    using ProbeNode::ProbeNode;
    void on_enter_tree() override {
        ProbeNode::on_enter_tree();
        if (on_enter_hook) { on_enter_hook(*this); }
    }
};

auto count_occurrences(const std::vector<std::string>& trace, const std::string& what) -> int {
    int n = 0;
    for (const auto& s : trace) { if (s == what) { ++n; } }
    return n;
}

auto index_of(const std::vector<std::string>& trace, const std::string& what) -> int {
    for (size_t i = 0; i < trace.size(); ++i) { if (trace[i] == what) { return static_cast<int>(i); } }
    return -1;
}

// ---- tests ----

void test_lifecycle_order() {
    std::vector<std::string> trace;

    auto root = std::make_shared<ProbeNode>(&trace, "root");
    auto child = std::make_shared<ProbeNode>(&trace, "child");
    auto grandchild = std::make_shared<ProbeNode>(&trace, "gc");
    child->add_child(grandchild);
    root->add_child(child);

    scene::NanSceneTree tree;
    tree.set_root(root);

    // enter is top-down: root before child before grandchild.
    check(index_of(trace, "root:enter") < index_of(trace, "child:enter"), "enter top-down root<child");
    check(index_of(trace, "child:enter") < index_of(trace, "gc:enter"), "enter top-down child<gc");

    // ready is bottom-up: grandchild before child before root.
    check(index_of(trace, "gc:ready") < index_of(trace, "child:ready"), "ready bottom-up gc<child");
    check(index_of(trace, "child:ready") < index_of(trace, "root:ready"), "ready bottom-up child<root");

    // each fires exactly once.
    check(count_occurrences(trace, "root:ready") == 1, "root ready once");
    check(count_occurrences(trace, "gc:enter") == 1, "gc enter once");
}

void test_reentrant_add_during_enter() {
    // P0 regression: adding a child inside on_enter_tree must not double-fire
    // enter_tree / ready on the newly added subtree.
    std::vector<std::string> trace;

    auto root = std::make_shared<HookNode>(&trace, "root");
    root->on_enter_hook = [&trace](ProbeNode& self) {
        auto late = std::make_shared<ProbeNode>(&trace, "late");
        self.add_child(late);
    };

    scene::NanSceneTree tree;
    tree.set_root(root);

    check(count_occurrences(trace, "late:enter") == 1, "late enter exactly once (P0)");
    check(count_occurrences(trace, "late:ready") == 1, "late ready exactly once (P0)");
}

void test_hover_enter_leave() {
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

    // Move over b.
    tree.dispatch_mouse_move(scene::MouseMoveEvent(foundation::NanPoint(200, 200), foundation::NanPoint(0, 0)));
    check(tree.hovered_node() == b.get(), "hover lands on b");
    check(count_occurrences(trace, "b:hover_enter") == 1, "b got one hover_enter");

    // Move off everything.
    tree.dispatch_mouse_move(scene::MouseMoveEvent(foundation::NanPoint(500, 500), foundation::NanPoint(0, 0)));
    check(tree.hovered_node() == nullptr, "hover cleared off-target");
    check(count_occurrences(trace, "b:hover_leave") == 1, "b got one hover_leave");
}

void test_focus_traversal() {
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
    check(tree.focused_node() == f1.get(), "focus_next -> f1");
    tree.focus_next();
    check(tree.focused_node() == f2.get(), "focus_next -> f2 (skips non-focusable)");
    tree.focus_next();
    check(tree.focused_node() == f1.get(), "focus_next wraps -> f1");
    tree.focus_previous();
    check(tree.focused_node() == f2.get(), "focus_previous wraps -> f2");
}

void test_queue_delete_dedup_and_safety() {
    auto root = std::make_shared<ProbeNode>(nullptr, "root");
    auto parentw = std::weak_ptr<scene::NanNode2D>();
    auto childw = std::weak_ptr<scene::NanNode2D>();
    {
        // Build the subtree and hand full ownership to root; keep only weak
        // observers so the tree is the sole strong owner before flush.
        auto parent = std::make_shared<ProbeNode>(nullptr, "parent");
        auto child = std::make_shared<ProbeNode>(nullptr, "child");
        parentw = parent;
        childw = child;
        parent->add_child(child);

        // Queue both: the ancestor should absorb the descendant request.
        // (queue_delete before set_root would have no tree; do it after.)
        root->add_child(parent);
    }

    scene::NanSceneTree tree;
    tree.set_root(root);

    tree.queue_delete(*parentw.lock());
    tree.queue_delete(*childw.lock());  // deduped under parent

    check(!childw.expired(), "child alive before flush");
    tree.process(0.016F);  // flush
    check(childw.expired(), "child destroyed with parent subtree");
    check(parentw.expired(), "parent destroyed on flush");
    check(root->child_count() == 0, "parent removed from root");

    // Queue a node, then destroy its owner by other means before flush:
    // weak delete_queue must not dangle.
    auto lone = std::make_shared<ProbeNode>(nullptr, "lone");
    root->add_child(lone);
    tree.queue_delete(*lone);
    root->remove_and_delete(*lone);  // gone before flush
    tree.process(0.016F);            // must not crash on expired weak_ptr
    check(root->child_count() == 0, "lone cleaned; no dangling crash");
}

void test_remove_readds_ready() {
    // A removed + re-added node should fire ready again (ready_notified_ reset).
    std::vector<std::string> trace;
    auto root = std::make_shared<ProbeNode>(&trace, "root");
    auto n = std::make_shared<ProbeNode>(&trace, "n");
    root->add_child(n);

    scene::NanSceneTree tree;
    tree.set_root(root);
    check(count_occurrences(trace, "n:ready") == 1, "n ready once initially");

    auto detached = root->remove_child(*n);  // exit_tree fires
    check(count_occurrences(trace, "n:exit") == 1, "n exited on remove");
    root->add_child(detached);  // re-enter + re-ready
    check(count_occurrences(trace, "n:ready") == 2, "n ready fires again after re-add");
}

} // namespace

auto main() -> int {
    struct Case { const char* name; void (*fn)(); };
    const Case cases[] = {
        {"lifecycle_order", test_lifecycle_order},
        {"reentrant_add_during_enter", test_reentrant_add_during_enter},
        {"hover_enter_leave", test_hover_enter_leave},
        {"focus_traversal", test_focus_traversal},
        {"queue_delete_dedup_and_safety", test_queue_delete_dedup_and_safety},
        {"remove_readds_ready", test_remove_readds_ready},
    };

    for (const auto& c : cases) {
        std::cout << "[ run  ] " << c.name << "\n";
        const int before = g_failures;
        c.fn();
        std::cout << (g_failures == before ? "[ ok   ] " : "[ FAIL ] ") << c.name << "\n";
    }

    std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
