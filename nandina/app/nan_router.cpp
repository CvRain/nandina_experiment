//
// app/nan_router — keep-alive page stack router implementation.
//

#include "nan_router.hpp"

namespace nandina::app
{
    namespace
    {
        class PageHost final: public scene::NanControl {
        protected:
            [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
                -> foundation::NanSize override {
                return constraints.constrain(size());
            }

            auto on_layout() -> void override {
                for (std::size_t i = 0; i < child_count(); ++i) {
                    auto* child = get_child(i) != nullptr ? get_child(i)->as_control() : nullptr;
                    if (!child || !child->visible()) {
                        continue;
                    }
                    (void)child->measure_layout(scene::LayoutConstraints::tight(size()));
                    child->layout_to(local_rect());
                }
            }
        };
    } // namespace

    NanRouter::NanRouter(
        reactive::Graph& graph,
        const theme::NanTheme& theme,
        NanStore* store,
        NanTypeKey store_key,
        resource::ResourceManager* resources,
        text::FontLoader* font_loader,
        text::FontFamilyRegistry* font_families,
        UiDispatcher* dispatcher,
        BackgroundExecutor* background_executor
    ):
        graph_(&graph),
        theme_(&theme),
        store_(store),
        store_key_(store_key),
        resources_(resources),
        font_loader_(font_loader),
        font_families_(font_families),
        dispatcher_(dispatcher),
        background_executor_(background_executor),
        host_(std::make_shared<PageHost>()) {}

    NanRouter::NanRouter(
        reactive::Graph& graph,
        theme::ThemeManager& theme_manager,
        NanStore* store,
        NanTypeKey store_key,
        resource::ResourceManager* resources,
        text::FontLoader* font_loader,
        text::FontFamilyRegistry* font_families,
        UiDispatcher* dispatcher,
        BackgroundExecutor* background_executor
    ):
        NanRouter(
            graph,
            theme_manager.theme(),
            store,
            store_key,
            resources,
            font_loader,
            font_families,
            dispatcher,
            background_executor
        ) {
        theme_manager_ = &theme_manager;
    }

    auto NanRouter::host() -> std::shared_ptr<scene::NanControl> {
        return host_;
    }

    auto NanRouter::graph() -> reactive::Graph& {
        return *graph_;
    }

    auto NanRouter::theme() const -> const theme::NanTheme& {
        return theme_manager_ != nullptr ? theme_manager_->theme() : *theme_;
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

        drop_frame(frames_.back());
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
            drop_frame(frames_.back());
            frames_.pop_back();
        }
        sync_visibility();
        return true;
    }

    void NanRouter::clear() {
        while (!frames_.empty()) {
            drop_frame(frames_.back());
            frames_.pop_back();
        }
    }

    auto NanRouter::request_pop() -> bool {
        return post_command([this] { (void)pop(); });
    }

    auto NanRouter::request_pop_to(std::string route_key) -> bool {
        return post_command([this, route_key = std::move(route_key)] { (void)pop_to(route_key); });
    }

    auto NanRouter::request_clear() -> bool {
        return post_command([this] { clear(); });
    }

    void NanRouter::push_page(std::unique_ptr<NanPage> page) {
        if (!page) {
            throw std::runtime_error("NanRouter::push_page: page is null");
        }

        auto scope = std::make_unique<reactive::ReactiveScope>(*graph_);
        auto async_scope = dispatcher_ && background_executor_
            ? std::make_unique<AsyncScope>(*dispatcher_, *background_executor_)
            : nullptr;
        PageContext context {
            *this,
            *graph_,
            *scope,
            theme(),
            store_,
            store_key_,
            resources_,
            font_loader_,
            font_families_,
            async_scope.get(),
            theme_manager_,
            dispatcher_
        };
        auto root = page->build(context);
        if (!root) {
            throw std::runtime_error("NanRouter::push_page: page build returned null root");
        }

        root->set_visible(false);
        attach_root(root);
        frames_.push_back(
            Frame {
                .page = std::move(page),
                .root = std::move(root),
                .scope = std::move(scope),
                .async_scope = std::move(async_scope),
                .key = {},
                .active = false,
            }
        );
        frames_.back().key = std::string(frames_.back().page->route_key());
        sync_visibility();
    }

    void NanRouter::sync_visibility() {
        // Find the previously active frame (if any) and the new top frame.
        Frame* deactivating = nullptr;
        Frame* activating = nullptr;

        for (auto& frame : frames_) {
            if (frame.active) {
                deactivating = &frame;
                break;
            }
        }

        // Set all frames invisible, mark top as visible.
        for (std::size_t i = 0; i < frames_.size(); ++i) {
            frames_[i].root->set_visible(i + 1 == frames_.size());
        }

        if (!frames_.empty()) {
            activating = &frames_.back();
        }

        // Deactivate the previously active page before activating the new one.
        if (deactivating != nullptr && deactivating != activating) {
            deactivating->active = false;
            auto ctx = make_context_for(*deactivating);
            deactivating->page->on_deactivate(ctx);
        }

        // Activate the new top page.
        if (activating != nullptr && !activating->active) {
            activating->active = true;
            auto ctx = make_context_for(*activating);
            activating->page->on_activate(ctx);
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

    void NanRouter::drop_frame(Frame& frame) {
        if (frame.active) {
            frame.active = false;
            auto ctx = make_context_for(frame);
            frame.page->on_deactivate(ctx);
        }
        detach_root(frame.root);
        if (frame.async_scope != nullptr) {
            frame.async_scope->clear();
        }
        if (frame.scope != nullptr) {
            frame.scope->clear();
        }
    }

    auto NanRouter::post_command(std::move_only_function<void()> command) -> bool {
        if (dispatcher_ == nullptr) {
            return false;
        }
        auto lifetime = std::weak_ptr<void>(command_lifetime_);
        return dispatcher_->post([lifetime = std::move(lifetime),
                                  command = std::move(command)]() mutable {
            if (const auto alive = lifetime.lock()) {
                command();
            }
        });
    }

    auto NanRouter::make_context_for(Frame& frame) -> PageContext {
        return PageContext {
            *this,
            *graph_,
            *frame.scope,
            theme(),
            store_,
            store_key_,
            resources_,
            font_loader_,
            font_families_,
            frame.async_scope.get(),
            theme_manager_,
            dispatcher_
        };
    }

} // namespace nandina::app
