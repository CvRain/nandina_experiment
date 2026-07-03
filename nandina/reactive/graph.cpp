//
// Created by cvrain on 2026/7/3.
//

#include "graph.hpp"

#include <algorithm>
#include <cassert>

namespace nandina::reactive
{

Graph::~Graph() {
    // 整体析构: 各 reactor 的 teardown 跳过跨节点解绑 (都将被释放, 解绑既无必要
    // 又有 use-after-free 风险)。unique_ptr 逐个释放。
    tearing_down_ = true;
    reactors_.clear();
}

auto Graph::next_id() -> std::uint64_t {
    return next_id_++;
}

auto Graph::adopt(std::unique_ptr<Reactor> reactor) -> Reactor* {
    auto* raw = reactor.get();
    reactors_.push_back(std::move(reactor));
    return raw;
}

void Graph::dispose_reactor(Reactor* reactor) {
    if (reactor == nullptr || reactor->disposed) {
        return;
    }
    reactor->disposed = true;
    clear_reactor_deps(*reactor);
    // 从 pending 移除, 避免 flush 时执行已释放对象。
    std::erase(pending_, reactor);
    // 从持有列表移除 (触发析构)。
    std::erase_if(reactors_, [reactor](const std::unique_ptr<Reactor>& r) {
        return r.get() == reactor;
    });
}

// ── 依赖追踪 ────────────────────────────────────────────────────────────────

void Graph::track(Source& source) {
    auto* reader = current_reader_;
    if (reader == nullptr || reader->disposed) {
        return;
    }
    // 去重: 同一 reader 一次执行内重复读同一 source 只建一条边。
    for (auto* s : reader->sources) {
        if (s == &source) {
            return;
        }
    }
    reader->sources.push_back(&source);
    source.subs.push_back(reader);
}

auto Graph::begin_read(Reactor* reactor) -> Reactor* {
    auto* prev = current_reader_;
    current_reader_ = reactor;
    return prev;
}

void Graph::end_read(Reactor* previous) {
    current_reader_ = previous;
}

void Graph::clear_reactor_deps(Reactor& reactor) {
    for (auto* source : reactor.sources) {
        std::erase(source->subs, &reactor);
    }
    reactor.sources.clear();
}

void Graph::detach_source(Source& source) {
    for (auto* reactor : source.subs) {
        std::erase(reactor->sources, &source);
    }
    source.subs.clear();
}

// ── 失效与调度 ──────────────────────────────────────────────────────────────

void Graph::notify_source(Source& source) {
    ++source.version;
    // 订阅者在传播中可能修改 source.subs (动态依赖), 先按当前快照逐个失效。
    // invalidate 不会向 source.subs 追加元素, 故索引遍历安全; 但可能移除,
    // 因此拷贝一份快照避免迭代器失效。
    const auto snapshot = source.subs;
    for (auto* reactor : snapshot) {
        invalidate_reactor(*reactor);
    }
    if (batch_depth_ == 0 && !flushing_) {
        flush();
    }
}

void Graph::invalidate_reactor(Reactor& reactor) {
    if (reactor.disposed) {
        return;
    }
    // 已 dirty 的节点无需重复传播 (computed 传播幂等的关键)。
    if (reactor.state == ReactorState::dirty) {
        return;
    }
    reactor.state = ReactorState::dirty;
    reactor.on_invalidate(*this);
}

void Graph::enqueue(Reactor& reactor) {
    if (reactor.in_queue || reactor.disposed) {
        return;
    }
    pending_.push_back(&reactor);
    reactor.in_queue = true;
}

void Graph::flush() {
    if (flushing_) {
        return;
    }
    flushing_ = true;

    // 执行过程中可能有新 reactor 入队, 故用索引遍历。
    std::size_t idx = 0;
    while (idx < pending_.size()) {
        auto* reactor = pending_[idx];
        ++idx;
        reactor->in_queue = false;
        if (reactor->disposed) {
            continue;
        }
        reactor->run(*this);
    }
    pending_.clear();
    flushing_ = false;
}

// ── batch ───────────────────────────────────────────────────────────────────

void Graph::begin_batch() {
    ++batch_depth_;
}

void Graph::end_batch() {
    assert(batch_depth_ > 0);
    --batch_depth_;
    if (batch_depth_ == 0) {
        flush();
    }
}

} // namespace nandina::reactive
