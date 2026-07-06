//
// Created by cvrain on 2026/7/4.
//
// app/window_config — NanWindow 的创建配置。
//
// 标准窗口零配置即可; 需要自定义标题栏 / 无边框 / 可调整大小时通过字段声明,
// 具体的自定义标题栏 chrome 由 NanWindow 子类覆写 build_chrome() 提供。
//

#ifndef NANDINA_EXPERIMENT_APP_WINDOW_CONFIG_HPP
#define NANDINA_EXPERIMENT_APP_WINDOW_CONFIG_HPP

#include "../foundation/nandina_color.hpp"

#include <string>

namespace nandina::app
{

    /// 窗口创建参数。所有字段有合理默认值, 标准桌面窗口可直接 `WindowConfig{}`。
    using WindowConfig = struct WindowConfig {
        std::string title = "Nandina";
        int width = 1100;
        int height = 700;
        int target_fps = 60;

        /// 是否显示系统标题栏 / 边框。false = 无边框 (用于自定义标题栏)。
        bool decorated = true;
        /// 是否允许用户拖拽调整窗口大小。
        bool resizable = true;
        /// 是否启用 MSAA 4x 抗锯齿。
        bool msaa = true;
        /// 是否垂直同步 (通常配合 target_fps)。
        bool vsync = true;

        /// 每帧清屏背景色。
        foundation::NanColor background = foundation::NanColor::from(
            foundation::NanHexRgb {.red = 30, .green = 30, .blue = 46, .alpha = 255}
        );
    };

} // namespace nandina::app

#endif // NANDINA_EXPERIMENT_APP_WINDOW_CONFIG_HPP
