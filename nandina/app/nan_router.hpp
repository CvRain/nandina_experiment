//
// app/nan_router — keep-alive page stack router.
//
// The router owns a stack of page frames. Each frame keeps both the page object
// and its built root node alive. Only the top frame is visible; lower frames stay
// mounted and keep their reactive bindings active, which makes shared Store
// updates immediately visible when popping back.
//

#ifndef NANDINA_EXPERIMENT_APP_NAN_ROUTER_HPP
#define NANDINA_EXPERIMENT_APP_NAN_ROUTER_HPP

#include "../reactive/graph.hpp"
#include "../scene/control.hpp"
#include "nan_page.hpp"
#include "nan_store.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace nandina::app
{

    class NanRouter {
    public:
        explicit NanRouter(
            reactive::Graph& graph,
            const theme::NanTheme& theme,
            NanStore* store = nullptr,
            NanTypeKey store_key = nullptr
        );
        ~NanRouter() = default;

        NanRouter(const NanRouter&) = delete;
        auto operator=(const NanRouter&) -> NanRouter& = delete;
        NanRouter(NanRouter&&) = delete;
        auto operator=(NanRouter&&) -> NanRouter& = delete;

        [[nodiscard]] auto host() -> std::shared_ptr<scene::NanControl>;
        [[nodiscard]] auto graph() -> reactive::Graph&;
        [[nodiscard]] auto theme() const -> const theme::NanTheme&;
        [[nodiscard]] auto store_base() -> NanStore*;
        [[nodiscard]] auto depth() const -> std::size_t;
        [[nodiscard]] auto empty() const -> bool;
        [[nodiscard]] auto current_key() const -> std::string_view;
        [[nodiscard]] auto can_pop() const -> bool;

        template<typename StoreT>
            requires std::derived_from<StoreT, NanStore>
        void set_store(StoreT& store) {
            store_ = &store;
            store_key_ = nan_type_key<StoreT>();
        }

        void clear_store();

        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, NanPageT<ParamsT>>
        auto push(ParamsT params) -> PageT& {
            auto page = std::make_unique<PageT>(std::move(params));
            auto* raw = page.get();
            push_page(std::move(page));
            return *raw;
        }

        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        auto push() -> PageT& {
            auto page = std::make_unique<PageT>();
            auto* raw = page.get();
            push_page(std::move(page));
            return *raw;
        }

        template<typename PageT, typename ParamsT>
            requires std::derived_from<PageT, NanPageT<ParamsT>>
        auto replace(ParamsT params) -> PageT& {
            if (!frames_.empty()) {
                pop();
            }
            return push<PageT>(std::move(params));
        }

        template<typename PageT>
            requires std::derived_from<PageT, NanPageT<typename PageT::Params>>
            && std::default_initializable<PageT>
        auto replace() -> PageT& {
            if (!frames_.empty()) {
                pop();
            }
            return push<PageT>();
        }

        auto pop() -> bool;
        auto pop_to(std::string_view route_key) -> bool;
        void clear();

    private:
        using NodePtr = std::shared_ptr<scene::NanNode>;

        struct Frame {
            std::unique_ptr<NanPage> page;
            std::shared_ptr<scene::NanNode2D> root;
            std::string key;
        };

        void push_page(std::unique_ptr<NanPage> page);
        void sync_visibility();
        void attach_root(const std::shared_ptr<scene::NanNode2D>& root);
        void detach_root(const std::shared_ptr<scene::NanNode2D>& root);

        reactive::Graph* graph_;
        const theme::NanTheme* theme_;
        NanStore* store_ = nullptr;
        NanTypeKey store_key_ = nullptr;
        std::shared_ptr<scene::NanControl> host_;
        std::vector<Frame> frames_;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_ROUTER_HPP
