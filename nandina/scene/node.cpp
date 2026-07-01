//
// Created by cvrain on 2026/6/30.
//

#include "node.hpp"
#include "scene_tree.hpp"

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
    return parent_;
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
    for (auto* p = other.parent_; p != nullptr; p = p->parent_) {
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

auto NanNode::add_child(std::unique_ptr<NanNode> child) -> NanNode& {
    if (!child) {
        throw std::runtime_error("NanNode::add_child: child is null");
    }
    if (child->parent_ != nullptr) {
        throw std::runtime_error("NanNode::add_child: child already has a parent");
    }
    if (child->tree_ != nullptr) {
        throw std::runtime_error("NanNode::add_child: child is already in a tree");
    }

    child->parent_ = this;
    auto* raw = child.get();
    children_.push_back(std::move(child));

    // If this node is already in a tree, propagate enter_tree to the new subtree.
    if (tree_) {
        raw->_propagate_enter_tree(tree_);
    }

    return *raw;
}

auto NanNode::remove_child(NanNode& child) -> std::unique_ptr<NanNode> {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (it->get() == &child) {
            // Exit tree before removing.
            if (tree_) {
                child._propagate_exit_tree();
            }
            child.parent_ = nullptr;
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
void NanNode::on_process(float /*dt*/) {}
void NanNode::on_draw() {}

void NanNode::_set_tree(NanSceneTree* tree) {
    tree_ = tree;
}

void NanNode::_propagate_enter_tree(NanSceneTree* tree) {
    tree_ = tree;
    on_enter_tree();
    for (auto& child : children_) {
        child->_propagate_enter_tree(tree);
    }
    // After all descendants have entered, call ready bottom-up.
    _propagate_ready();
}

void NanNode::_propagate_ready() {
    // Children first (bottom-up).
    for (auto& child : children_) {
        child->_propagate_ready();
    }
    on_ready();
}

void NanNode::_propagate_exit_tree() {
    on_exit_tree();
    for (auto& child : children_) {
        child->_propagate_exit_tree();
    }
    tree_ = nullptr;
}

void NanNode::_propagate_process(const float dt) {
    on_process(dt);
    for (auto& child : children_) {
        child->_propagate_process(dt);
    }
}

void NanNode::_propagate_draw() {
    on_draw();
    for (auto& child : children_) {
        child->_propagate_draw();
    }
}

} // namespace nandina::scene
