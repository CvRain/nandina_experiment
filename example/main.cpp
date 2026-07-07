//
// Nandina — Router + Store demo
//
// 展示新的 App 层心智模型:
//   - NanApplication 持有全局 Graph 与开发者 Store;
//   - NanWindow.use_router() 创建 keep-alive Router 并挂载 host;
//   - Page 只负责 typed params 向下传递与 build UI;
//   - 横向 / 向上同步走 Store 里的 Signal, 不靠反向路由参数。
//

#include "app/nan_application.hpp"
#include "app/nan_page.hpp"
#include "app/nan_router.hpp"
#include "app/nan_store.hpp"
#include "app/nan_window.hpp"
#include "foundation/geometry.hpp"
#include "foundation/nandina_color.hpp"
#include "reactive/signal.hpp"
#include "scene/control.hpp"
#include "widget/bindable_rect.hpp"
#include "widget/button.hpp"

#include <memory>
#include <string_view>

using namespace nandina;

namespace
{

    auto oklch(const float light, const float chroma, const float hue) -> foundation::NanColor {
        return foundation::NanColor::from(
            foundation::NanOklch {.light = light, .chroma = chroma, .hue = hue, .alpha = 1.0F}
        );
    }

    struct DemoStore final: app::NanStore {
        explicit DemoStore(reactive::Graph& graph):
            blog_count(graph, 3),
            accent_position(graph, foundation::NanPoint(620.0F, 60.0F)),
            accent_color(graph, oklch(0.39F, 0.03F, 276.0F)) {}

        reactive::Signal<int> blog_count;
        reactive::Signal<foundation::NanPoint> accent_position;
        reactive::Signal<foundation::NanColor> accent_color;
    };

    struct ConsoleParams {
        int user_id = 0;
    };

    struct BlogParams {
        int user_id = 0;
        int blog_id = 0;
    };

    auto add_card(
        scene::NanControl& parent,
        foundation::NanPoint position,
        foundation::NanSize size,
        foundation::NanColor color
    ) -> std::shared_ptr<scene::NanControl> {
        auto card = std::make_shared<scene::NanControl>(size);
        card->set_position(position);
        card->set_background(color);
        parent.add_child(card);
        return card;
    }

    class BlogPage final: public app::NanPageT<BlogParams> {
    public:
        explicit BlogPage(BlogParams params): NanPageT(params) {}

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "blog";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override {
            auto& store = context.store<DemoStore>();

            // 模拟深层页面修改共享状态: 控制台页 keep-alive, pop 回去时已看到新值。
            store.blog_count.update([](int& value) { value += 1; });
            store.accent_position.set(
                foundation::NanPoint(560.0F + static_cast<float>(params().blog_id) * 18.0F, 80.0F)
            );
            store.accent_color.set(
                oklch(0.58F, 0.13F, 35.0F + static_cast<float>(params().blog_id) * 25.0F)
            );

            auto root = std::make_shared<scene::NanControl>(foundation::NanSize(1100, 700));

            auto shell = add_card(
                *root,
                foundation::NanPoint(60.0F, 60.0F),
                foundation::NanSize(460.0F, 300.0F),
                oklch(0.28F, 0.03F, 260.0F)
            );

            add_card(
                *shell,
                foundation::NanPoint(24.0F, 24.0F),
                foundation::NanSize(160.0F, 64.0F),
                oklch(0.65F, 0.17F, 35.0F)
            );
            add_card(
                *shell,
                foundation::NanPoint(24.0F, 112.0F),
                foundation::NanSize(320.0F, 48.0F),
                oklch(0.45F, 0.08F, 260.0F)
            );
            add_card(
                *shell,
                foundation::NanPoint(24.0F, 184.0F),
                foundation::NanSize(260.0F, 48.0F),
                oklch(0.50F, 0.12F, 140.0F)
            );

            auto pop_hint = add_card(
                *root,
                foundation::NanPoint(620.0F, 430.0F),
                foundation::NanSize(360.0F, 70.0F),
                oklch(0.72F, 0.12F, 120.0F)
            );
            pop_hint->set_name("Blog page mutated Store; press Escape after future key binding.");

            return root;
        }
    };

    class ConsolePage final: public app::NanPageT<ConsoleParams> {
    public:
        explicit ConsolePage(ConsoleParams params): NanPageT(params) {}

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "console";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override {
            auto& store = context.store<DemoStore>();

            auto root = std::make_shared<scene::NanControl>(foundation::NanSize(1100, 700));

            auto panel = add_card(
                *root,
                foundation::NanPoint(60.0F, 60.0F),
                foundation::NanSize(420.0F, 260.0F),
                oklch(0.28F, 0.03F, 260.0F)
            );

            add_card(
                *panel,
                foundation::NanPoint(24.0F, 24.0F),
                foundation::NanSize(170.0F, 54.0F),
                oklch(0.62F, 0.18F, 250.0F)
            );
            auto open_blog = std::make_shared<widget::Button>("Open blog");
            open_blog->set_position(foundation::NanPoint(24.0F, 184.0F));
            open_blog->set_tone(theme::ButtonTone::primary);
            open_blog->set_treatment(theme::ButtonTreatment::filled);
            auto& router = context.router();
            open_blog->set_on_click([&router, user_id = params().user_id, &store] {
                router.push<BlogPage>(
                    BlogParams {.user_id = user_id, .blog_id = store.blog_count.peek()}
                );
            });
            panel->add_child(open_blog);

            // 响应式强调块: BlogPage 修改 Store 后, 这个 keep-alive 控件会立即更新。
            auto accent = std::make_shared<widget::BindableRect>(
                context.graph(),
                foundation::NanSize(420.0F, 500.0F)
            );
            accent->bind_position(store.accent_position);
            accent->bind_background(store.accent_color);
            root->add_child(accent);

            // 当前还没有 Button / Label, 暂用色块表示「博客数量」。
            add_card(
                *root,
                foundation::NanPoint(620.0F, 600.0F),
                foundation::NanSize(
                    260.0F + static_cast<float>(store.blog_count.peek()) * 18.0F,
                    44.0F
                ),
                oklch(0.74F, 0.13F, 95.0F)
            );

            return root;
        }
    };

} // namespace

auto main() -> int {
    app::NanApplication application;
    application.use_store<DemoStore>();

    app::NanWindow window {
        application,
        app::WindowConfig {
            .title = "Nandina — Router + Store Demo",
            .width = 1100,
            .height = 700,
            .background = oklch(0.33F, 0.03F, 275.0F),
        }
    };

    auto& router = window.use_router();
    router.push<ConsolePage>(ConsoleParams {.user_id = 42});

    return application.run(window);
}
