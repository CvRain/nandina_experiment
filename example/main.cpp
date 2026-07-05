//
// Nandina — App/Window demo
//
// 展示新的应用入口: 开发者不再手写 raylib 主循环 / 输入翻译 / 颜色转换。
// 只需构造 NanApplication + NanWindow, set_content 挂载一个 NanControl 树,
// 然后 app.run(window)。整个 raylib 样板与 utils.hpp 已被 app 层吸收。
//

#include "app/nan_application.hpp"
#include "app/nan_window.hpp"
#include "foundation/geometry.hpp"
#include "foundation/nandina_color.hpp"
#include "scene/control.hpp"

#include <memory>
#include <ranges>

using namespace nandina;

namespace
{

    auto oklch(const float light, const float chroma, const float hue) -> foundation::NanColor {
        return foundation::NanColor::from(
            foundation::NanOklch {.light = light, .chroma = chroma, .hue = hue, .alpha = 1.0F}
        );
    }

    /// 构建一个简单的静态页面: 一个面板 + 三张彩色卡片, 全部用 NanControl。
    auto build_demo_root() -> std::shared_ptr<scene::NanControl> {
        // 根: 占满窗口的透明容器 (无背景, 由窗口清屏色打底)。
        auto root = std::make_shared<scene::NanControl>(foundation::NanSize(1100, 700));

        // 面板。
        auto panel = std::make_shared<scene::NanControl>(foundation::NanSize(360, 220));
        panel->set_position(foundation::NanPoint(60, 60));
        panel->set_background(oklch(0.28F, 0.03F, 260.0F));
        root->add_child(panel);

        const auto card_colors = std::array<foundation::NanColor, 3> {
            oklch(0.62F, 0.18F, 250.0F),
            oklch(0.68F, 0.16F, 150.0F),
            oklch(0.70F, 0.15F, 60.0F),
        };

        for (size_t i = 0; i < card_colors.size(); i++) {
            auto card = std::make_shared<scene::NanControl>(foundation::NanSize(90, 120));
            card->set_position(foundation::NanPoint(20.0F + static_cast<float>(i) * 110.0F, 50.0F));
            card->set_background(card_colors.at(i));
            panel->add_child(card);
        }

        // 右侧一个大的强调块。
        const auto accent = std::make_shared<scene::NanControl>(foundation::NanSize(420, 500));
        accent->set_position(foundation::NanPoint(620, 60));
        accent->set_background(oklch(0.39F, 0.03F, 276.0F));
        root->add_child(accent);

        return root;
    }

} // namespace

auto main() -> int {
    app::NanApplication application;

    app::NanWindow window {
        application,
        app::WindowConfig {
            .title = "Nandina — App/Window Demo",
            .width = 1100,
            .height = 700,
            .background = oklch(0.33F, 0.03F, 275.0F),
        }
    };

    window.set_content(build_demo_root());

    return application.run(window);
}
