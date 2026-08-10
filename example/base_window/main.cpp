//
// base_window 示例子项目 - 最小占位实现。
// 展示Nandina基础窗口
//

#include "base_window.hpp"

#include "app/nan_application.hpp"

using namespace nandina;

auto main() -> int {
    return app::run<example::base_window::MainPage>(
        app::RunConfig {
            .id = "org.nandina.example.base_window",
            .window =
                app::WindowConfig {
                    .title = "nandina::example base window",
                    .width = 640,
                    .height = 480,
                },
        }
    );
}
