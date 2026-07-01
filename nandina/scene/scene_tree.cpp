//
// Created by cvrain on 2026/6/30.
//

#include "scene_tree.hpp"

#include <algorithm>

namespace nandina::scene
{

NanSceneTree::NanSceneTree() = default;

NanSceneTree::~NanSceneTree() {
    // Flush queued deletes before tearing down the root.
    _flush_deletes();
    if (root_) {
        root_->_propagate_exit_tree();
    }
}

auto NanSceneTree::root() const -> NanNode2D* {
    return root_.get();
}

void NanSceneTree::set_root(std::unique_ptr<NanNode2D> root) {
    if (root_) {
        root_->_propagate_exit_tree();
    }
    delete_queue_.clear();
    root_ = std::move(root);
    if (root_) {
        root_->_propagate_enter_tree(this);
    }
}

void NanSceneTree::process(const float dt) {
    _flush_deletes();
    if (root_) {
        root_->_propagate_process(dt);
    }
}

void NanSceneTree::draw() {
    if (root_) {
        root_->_propagate_draw();
    }
}

// ---- deferred deletion ----

void NanSceneTree::queue_delete(NanNode& node) {
    // Ignore if the node (or an ancestor) is already queued.
    for (auto* queued : delete_queue_) {
        if (queued == &node || queued->is_ancestor_of(node)) {
            return;  // Ancestor already queued — its deletion will cascade.
        }
    }
    // Remove any queued descendants of this node (they will be freed transitively).
    delete_queue_.erase(
        std::remove_if(delete_queue_.begin(), delete_queue_.end(),
            [&node](NanNode* q) { return node.is_ancestor_of(*q); }),
        delete_queue_.end()
    );
    delete_queue_.push_back(&node);
}

void NanSceneTree::_flush_deletes() {
    if (delete_queue_.empty()) return;

    for (auto* node : delete_queue_) {
        if (!node->is_inside_tree()) continue;

        if (auto* parent = node->parent()) {
            parent->remove_and_delete(*node);
        } else if (node == root_.get()) {
            root_->_propagate_exit_tree();
            root_.reset();
        }
    }
    delete_queue_.clear();
}

// ---- hit testing ----

auto NanSceneTree::hit_test(const foundation::NanPoint world_point) const -> NanNode2D* {
    if (!root_) {
        return nullptr;
    }
    return _hit_test_node(root_.get(), world_point);
}

auto NanSceneTree::_hit_test_node(NanNode2D* node, const foundation::NanPoint world_point)
    -> NanNode2D* {
    if (!node->visible()) {
        return nullptr;
    }

    const auto count = node->child_count();
    if (count > 0) {
        std::vector<size_t> indices(count);
        for (size_t i = 0; i < count; ++i) {
            indices[i] = i;
        }

        std::stable_sort(indices.begin(), indices.end(),
            [node](size_t a, size_t b) {
                auto* child_a = static_cast<NanNode2D*>(node->get_child(a));
                auto* child_b = static_cast<NanNode2D*>(node->get_child(b));
                const int za = child_a ? child_a->z_index() : 0;
                const int zb = child_b ? child_b->z_index() : 0;
                if (za != zb) {
                    return za > zb;
                }
                return a > b;
            });

        for (const auto idx : indices) {
            auto* child = static_cast<NanNode2D*>(node->get_child(idx));
            if (!child) {
                continue;
            }
            auto* hit = _hit_test_node(child, world_point);
            if (hit) {
                return hit;
            }
        }
    }

    const auto local_point = node->to_local(world_point);
    if (node->contains_point(local_point)) {
        return node;
    }

    return nullptr;
}

} // namespace nandina::scene
