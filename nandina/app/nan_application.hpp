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
        [[nodiscard]] auto graph() -> reactive::Graph& {
            return graph_;
        }

        /// 进入阻塞主循环: 打开窗口, 每帧 tick, 直到窗口关闭。返回进程退出码。
        auto run(NanWindow& window) -> int;

    private:
        reactive::Graph graph_;
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_NAN_APPLICATION_HPP
