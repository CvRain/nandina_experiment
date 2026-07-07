//
// app/nan_router — keep-alive page stack router implementation.
//

#include "nan_router.hpp"

namespace nandina::app
{

    NanRouter::NanRouter(
        reactive::Graph& graph,
        const theme::NanTheme& theme,
        NanStore* store,
        NanTypeKey store_key
    ):
        graph_(&graph),
        theme_(&theme),
        store_(store),
        store_key_(store_key),
        host_(std::make_shared<scene::NanControl>()) {}

    auto NanRouter::host() -> std::shared_ptr<scene::NanControl> {
        return host_;
    }

    auto NanRouter::graph() -> reactive::Graph& {
        return *graph_;
    }

    auto NanRouter::theme() const -> const theme::NanTheme& {
        return *theme_;
    }

    auto NanRouter::store_base() -> NanStore* {
        return store_;
    }

    auto NanRouter::depth() const -> std::size_t {
        return frames_.size();
    }

    auto NanRouter::empty() const -> bool {
        return frames_.empty();
    }

    auto NanRouter::current_key() const -> std::string_view {
        return frames_.empty() ? std::string_view {} : std::string_view {frames_.back().key};
    }

    auto NanRouter::can_pop() const -> bool {
        return frames_.size() > 1;
    }

    void NanRouter::clear_store() {
        store_ = nullptr;
        store_key_ = nullptr;
    }

    auto NanRouter::pop() -> bool {
        if (frames_.size() <= 1) {
            return false;
        }

        detach_root(frames_.back().root);
        frames_.pop_back();
        sync_visibility();
        return true;
    }

    auto NanRouter::pop_to(std::string_view route_key) -> bool {
        if (frames_.empty()) {
            return false;
        }

        std::size_t target = frames_.size();
        for (std::size_t i = frames_.size(); i > 0; --i) {
            if (frames_[i - 1].key == route_key) {
                target = i - 1;
                break;
            }
        }
        if (target == frames_.size()) {
            return false;
        }

        while (frames_.size() > target + 1) {
            detach_root(frames_.back().root);
            frames_.pop_back();
        }
        sync_visibility();
        return true;
    }

    void NanRouter::clear() {
        while (!frames_.empty()) {
            detach_root(frames_.back().root);
            frames_.pop_back();
        }
    }

    void NanRouter::push_page(std::unique_ptr<NanPage> page) {
        if (!page) {
            throw std::runtime_error("NanRouter::push_page: page is null");
        }

        PageContext context {*this, *graph_, *theme_, store_, store_key_};
        auto root = page->build(context);
        if (!root) {
            throw std::runtime_error("NanRouter::push_page: page build returned null root");
        }

        root->set_visible(false);
        attach_root(root);
        frames_.push_back(Frame {
            .page = std::move(page),
            .root = std::move(root),
            .key = {},
        });
        frames_.back().key = std::string(frames_.back().page->route_key());
        sync_visibility();
    }

    void NanRouter::sync_visibility() {
        for (std::size_t i = 0; i < frames_.size(); ++i) {
            frames_[i].root->set_visible(i + 1 == frames_.size());
        }
    }

    void NanRouter::attach_root(const std::shared_ptr<scene::NanNode2D>& root) {
        host_->add_child(root);
    }

    void NanRouter::detach_root(const std::shared_ptr<scene::NanNode2D>& root) {
        if (!root) {
            return;
        }
        host_->remove_child(*root);
    }

} // namespace nandina::app
