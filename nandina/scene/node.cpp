//
// Created by cvrain on 2026/6/30.
//

#include "node.hpp"
#include "node2d.hpp"
#include "scene_tree.hpp"

#include <algorithm>
#include <stdexcept>

namespace nandina::scene
{

NanNode::NanNode() = default;

NanNode::~NanNode() {
    // Remove from tree before destruction (calls on_exit_tree on children).
    if (tree_) {
        _propagate_exit_tree();
    }
    // Children are automatically destroyed by unique_ptr.
}

auto NanNode::parent() const -> NanNode* {
    return parent_.lock().get();
}

auto NanNode::child_count() const -> size_t {
    return children_.size();
}

auto NanNode::get_child(const size_t index) const -> NanNode* {
    if (index < children_.size()) {
        return children_[index].get();
    }
    return nullptr;
}

auto NanNode::is_ancestor_of(const NanNode& other) const -> bool {
    for (auto* p = other.parent(); p != nullptr; p = p->parent()) {
        if (p == this) {
            return true;
        }
    }
    return false;
}

auto NanNode::is_inside_tree() const -> bool {
    return tree_ != nullptr;
}

auto NanNode::get_tree() const -> NanSceneTree* {
    return tree_;
}

auto NanNode::add_child(std::shared_ptr<NanNode> child) -> NanNode& {
    if (!child) {
        throw std::runtime_error("NanNode::add_child: child is null");
    }
    if (!child->parent_.expired()) {
        throw std::runtime_error("NanNode::add_child: child already has a parent");
    }
    if (child->tree_ != nullptr) {
        throw std::runtime_error("NanNode::add_child: child is already in a tree");
    }

    const auto* parent_2d = as_node2d();
    const auto* child_2d = child->as_node2d();
    if ((parent_2d != nullptr) != (child_2d != nullptr)) {
        throw std::runtime_error(
            "NanNode::add_child: cannot mix NanNode and NanNode2D on the same edge"
        );
    }

    child->parent_ = weak_from_this();
    auto* raw = child.get();
    children_.push_back(std::move(child));

    // If this node is already in a tree, propagate enter_tree to the new subtree.
    if (tree_) {
        raw->_propagate_enter_tree(tree_);
        raw->_propagate_ready();
    }

    return *raw;
}

auto NanNode::remove_child(NanNode& child) -> std::shared_ptr<NanNode> {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() == &child) {
            // Exit tree before removing.
            if (tree_) {
                child._propagate_exit_tree();
            }
            child.parent_.reset();
            auto result = std::move(*it);
            children_.erase(it);
            return result;
        }
    }
    return nullptr;
}

void NanNode::remove_and_delete(NanNode& child) {
    remove_child(child);  // unique_ptr goes out of scope, child is destroyed.
}

auto NanNode::name() const -> std::string_view {
    return name_;
}

void NanNode::set_name(std::string name) {
    name_ = std::move(name);
}

void NanNode::on_enter_tree() {}
void NanNode::on_ready() {}
void NanNode::on_exit_tree() {}

auto NanNode::on_input(InputEvent& /*event*/) -> bool { return false; }

void NanNode::on_process(float /*dt*/) {}
void NanNode::on_draw() {}

auto NanNode::z_index_hint() const -> int { return 0; }

auto NanNode::is_focusable() const -> bool { return false; }

auto NanNode::is_visible_in_tree() const -> bool {
    auto* p = parent();
    return p == nullptr || p->is_visible_in_tree();
}

void NanNode::_set_tree(NanSceneTree* tree) {
    tree_ = tree;
}

void NanNode::_propagate_enter_tree(NanSceneTree* tree) {
    // Idempotent: if a user calls add_child() from inside on_enter_tree(),
    // add_child already propagated enter_tree into the new subtree (because
    // tree_ was set). The parent's own children loop must not enter it again.
    if (tree_ == tree) {
        return;
    }
    tree_ = tree;
    on_enter_tree();
    for (auto& child : children_) {
        child->_propagate_enter_tree(tree);
    }
}

void NanNode::_propagate_ready() {
    // Children first (bottom-up).
    for (auto& child : children_) {
        child->_propagate_ready();
    }
    // Idempotent: a node added during on_enter_tree() is readied immediately by
    // add_child; the later top-level ready sweep must not fire on_ready twice.
    if (!ready_notified_) {
        ready_notified_ = true;
        on_ready();
    }
}

void NanNode::_propagate_exit_tree() {
    on_exit_tree();
    for (auto& child : children_) {
        child->_propagate_exit_tree();
    }
    tree_ = nullptr;
    ready_notified_ = false;  // Allow re-entry (remove + re-add) to fire ready again.
}

void NanNode::_propagate_process(const float dt) {
    on_process(dt);
    for (auto& child : children_) {
        child->_propagate_process(dt);
    }
}

void NanNode::_propagate_draw() {
    if (!is_visible_in_tree()) {
        return;
    }

    on_draw();

    if (children_.empty()) return;

    std::vector<size_t> indices(children_.size());
    for (size_t i = 0; i < children_.size(); ++i) indices[i] = i;
    std::stable_sort(indices.begin(), indices.end(),
        [this](size_t a, size_t b) {
            const int za = children_[a]->z_index_hint();
            const int zb = children_[b]->z_index_hint();
            return za < zb;
        });

    for (size_t idx : indices) {
        children_[idx]->_propagate_draw();
    }
}

} // namespace nandina::scene
