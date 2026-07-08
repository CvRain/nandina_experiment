//
// Created by cvrain on 2026/7/4.
//
// app/nan_application — 应用主入口。
//
// NanApplication 是进程级的编排者:
//   - 初始化日志 (spdlog);
//   - 持有响应式 Graph —— 它必须比任何 window / widget 活得久, 因为 widget 卸载时
//     会回访 graph (dispose effect / detach source)。App 持有它 = 最长生命周期;
//   - run(window): 进入阻塞主循环, 每帧驱动 window tick, 直到窗口请求关闭。
//
// 用法:
//   NanApplication app;
//   MyWindow window{app, {.title = "Demo"}};
//   return app.run(window);
//

#ifndef NANDINA_EXPERIMENT_APP_NAN_APPLICATION_HPP
#define NANDINA_EXPERIMENT_APP_NAN_APPLICATION_HPP

#include "../reactive/graph.hpp"
#include "../theme/theme.hpp"
#include "nan_page.hpp"
#include "nan_store.hpp"

#include <concepts>
#include <memory>
#include <stdexcept>
#include <utility>

namespace nandina::app
{

    class NanWindow;

    class NanApplication {
    public:
        NanApplication();
        ~NanApplication();

        NanApplication(const NanApplication&) = delete;
        auto operator=(const NanApplication&) -> NanApplication& = delete;
        NanApplication(NanApplication&&) = delete;
        auto operator=(NanApplication&&) -> NanApplication& = delete;

        /// 响应式调度图。所有 signal / effect 依附其上; 生命周期覆盖整个 app。
        [[nodiscard]] auto graph() -> reactive::Graph&;

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme() const -> const theme::NanTheme&;

        template<typename StoreT, typename... Args>
            requires std::derived_from<StoreT, NanStore>
        auto use_store(Args&&... args) -> StoreT& {
            store_ = std::make_unique<StoreT>(graph_, std::forward<Args>(args)...);
            store_key_ = nan_type_key<StoreT>();
            return static_cast<StoreT&>(*store_);
        }

        template<typename StoreT>
            requires std::derived_from<StoreT, NanStore>
        [[nodiscard]] auto store() -> StoreT& {
            if (store_ == nullptr || store_key_ != nan_type_key<StoreT>()) {
                throw std::runtime_error(
                    "NanApplication::store: requested store type is not installed"
                );
            }
            return static_cast<StoreT&>(*store_);
        }

        [[nodiscard]] auto store_base() -> NanStore*;
        [[nodiscard]] auto store_type_key() const -> NanTypeKey;

        /// 进入阻塞主循环: 打开窗口, 每帧 tick, 直到窗口关闭。返回进程退出码。
        auto run(NanWindow& window) -> int;

    private:
        reactive::Graph graph_;
        theme::NanTheme theme_;
        std::unique_ptr<NanStore> store_;
        NanTypeKey store_key_ = nullptr;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_APPLICATION_HPP
