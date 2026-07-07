//
// app/nan_page — typed page and page context contracts.
//
// Route params are downward data (Angular route params / Svelte page data style).
// Shared app state lives in a developer-defined NanStore and is accessed through
// PageContext, so deep pages can update the store and keep-alive ancestor pages
// react through Signal/Effect without reverse route plumbing.
//

#ifndef NANDINA_EXPERIMENT_APP_NAN_PAGE_HPP
#define NANDINA_EXPERIMENT_APP_NAN_PAGE_HPP

#include "../reactive/graph.hpp"
#include "../scene/node2d.hpp"
#include "../theme/theme.hpp"
#include "nan_store.hpp"

#include <concepts>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

namespace nandina::app
{

    using NanTypeKey = const void*;

    struct NoParams {};

    template<typename T>
    [[nodiscard]] auto nan_type_key() -> NanTypeKey {
        static const int token = 0;
        return &token;
    }

    class NanRouter;

    class PageContext {
    public:
        PageContext(
            NanRouter& router,
            reactive::Graph& graph,
            const theme::NanTheme& theme,
            NanStore* store,
            NanTypeKey store_key
        ):
            router_(&router),
            graph_(&graph),
            theme_(&theme),
            store_(store),
            store_key_(store_key) {}

        [[nodiscard]] auto router() -> NanRouter& {
            return *router_;
        }

        [[nodiscard]] auto graph() -> reactive::Graph& {
            return *graph_;
        }

        [[nodiscard]] auto theme() const -> const theme::NanTheme& {
            return *theme_;
        }

        [[nodiscard]] auto has_store() const -> bool {
            return store_ != nullptr;
        }

        template<typename StoreT>
            requires std::derived_from<StoreT, NanStore>
        [[nodiscard]] auto store() -> StoreT& {
            if (store_ == nullptr || store_key_ != nan_type_key<StoreT>()) {
                throw std::runtime_error(
                    "PageContext::store: requested store type is not installed"
                );
            }
            return static_cast<StoreT&>(*store_);
        }

    private:
        NanRouter* router_;
        reactive::Graph* graph_;
        const theme::NanTheme* theme_;
        NanStore* store_;
        NanTypeKey store_key_ = nullptr;
    };

    class NanPage {
    public:
        virtual ~NanPage() = default;

        NanPage(const NanPage&) = delete;
        auto operator=(const NanPage&) -> NanPage& = delete;
        NanPage(NanPage&&) = delete;
        auto operator=(NanPage&&) -> NanPage& = delete;

        [[nodiscard]] virtual auto route_key() const -> std::string_view = 0;
        [[nodiscard]] virtual auto params_type_key() const -> NanTypeKey = 0;
        [[nodiscard]] virtual auto build(PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> = 0;

    protected:
        NanPage() = default;
    };

    template<typename ParamsT>
    class NanPageT: public NanPage {
    public:
        using Params = ParamsT;

        NanPageT()
            requires std::default_initializable<Params>
        = default;
        explicit NanPageT(Params params): params_(std::move(params)) {}

        [[nodiscard]] auto params() const -> const Params& {
            return params_;
        }

        [[nodiscard]] auto params() -> Params& {
            return params_;
        }

        [[nodiscard]] auto params_type_key() const -> NanTypeKey override {
            return nan_type_key<Params>();
        }

    private:
        Params params_;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_PAGE_HPP
