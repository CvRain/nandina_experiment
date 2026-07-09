//
// Router + Store tests.
//

#include "app/nan_router.hpp"
#include "app/nan_store.hpp"
#include "foundation/geometry.hpp"
#include "reactive/effect.hpp"
#include "reactive/signal.hpp"
#include "scene/control.hpp"
#include "theme/theme.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string_view>

namespace
{
    using namespace nandina;

    struct TestStore final: app::NanStore {
        explicit TestStore(reactive::Graph& graph): count(graph, 0) {}
        reactive::Signal<int> count;
    };

    struct HomeParams {
        int user_id = 0;
    };

    struct DetailParams {
        int blog_id = 0;
    };

    class HomePage final: public app::NanPageT<HomeParams> {
    public:
        explicit HomePage(HomeParams params): NanPageT(params) {}

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "home";
        }

        [[nodiscard]] auto build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> override {
            auto root = std::make_shared<scene::NanControl>(foundation::NanSize(320, 200));
            root->set_name("home-root");
            root->set_position(foundation::NanPoint(static_cast<float>(params().user_id), 0.0F));
            context.store<TestStore>().count.set(1);
            return root;
        }
    };

    class DetailPage final: public app::NanPageT<DetailParams> {
    public:
        explicit DetailPage(DetailParams params): NanPageT(params) {}

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "detail";
        }

        [[nodiscard]] auto build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> override {
            auto root = std::make_shared<scene::NanControl>(foundation::NanSize(200, 100));
            root->set_name("detail-root");
            context.store<TestStore>().count.set(params().blog_id);
            return root;
        }
    };

    class PlainPage final: public app::NanPageT<app::NoParams> {
    public:
        PlainPage() = default;

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "plain";
        }

        [[nodiscard]] auto build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> override {
            auto root = std::make_shared<scene::NanControl>(foundation::NanSize(100, 50));
            root->set_background(context.theme().palette.primary);
            return root;
        }
    };

    struct ScopedObserverLog {
        int observed = 0;
        int runs = 0;
    };

    struct ScopedObserverParams {
        ScopedObserverLog* log = nullptr;
    };

    class ScopedObserverPage final: public app::NanPageT<ScopedObserverParams> {
    public:
        explicit ScopedObserverPage(ScopedObserverParams params): NanPageT(params) {}

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "scoped-observer";
        }

        [[nodiscard]] auto build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> override {
            auto& store = context.store<TestStore>();
            auto* log = params().log;
            context.scope().effect([&store, log] {
                log->observed = store.count.get();
                ++log->runs;
            });
            return std::make_shared<scene::NanControl>(foundation::NanSize(80, 40));
        }
    };
} // namespace

TEST_CASE("router pushes keep-alive pages and toggles top visibility", "[app][router]") {
    reactive::Graph graph;
    TestStore store {graph};
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme};
    router.set_store(store);

    auto& home = router.push<HomePage>(HomeParams {.user_id = 42});
    REQUIRE(home.params().user_id == 42);
    REQUIRE(router.depth() == 1);
    REQUIRE(router.current_key() == "home");
    REQUIRE(router.host()->child_count() == 1);

    auto* home_root = router.host()->get_child(0)->as_node2d();
    REQUIRE(home_root != nullptr);
    REQUIRE(home_root->visible());
    REQUIRE(store.count.peek() == 1);

    router.push<DetailPage>(DetailParams {.blog_id = 7});
    REQUIRE(router.depth() == 2);
    REQUIRE(router.current_key() == "detail");
    REQUIRE(router.host()->child_count() == 2);
    REQUIRE_FALSE(home_root->visible());
    auto* detail_root = router.host()->get_child(1)->as_node2d();
    REQUIRE(detail_root != nullptr);
    REQUIRE(detail_root->visible());
    REQUIRE(store.count.peek() == 7);

    REQUIRE(router.pop());
    REQUIRE(router.depth() == 1);
    REQUIRE(router.current_key() == "home");
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE(home_root->visible());
    REQUIRE_FALSE(router.pop());
}

TEST_CASE("router pop_to keeps target frame and removes newer frames", "[app][router]") {
    reactive::Graph graph;
    TestStore store {graph};
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme, &store, app::nan_type_key<TestStore>()};

    router.push<HomePage>(HomeParams {.user_id = 1});
    router.push<DetailPage>(DetailParams {.blog_id = 2});
    router.push<DetailPage>(DetailParams {.blog_id = 3});

    REQUIRE(router.depth() == 3);
    REQUIRE(router.pop_to("detail"));
    REQUIRE(router.depth() == 3);
    REQUIRE(router.pop_to("home"));
    REQUIRE(router.depth() == 1);
    REQUIRE(router.current_key() == "home");
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE_FALSE(router.pop_to("missing"));
}

TEST_CASE("store updates propagate to keep-alive page effects", "[app][router][store]") {
    reactive::Graph graph;
    TestStore store {graph};
    int observed = 0;

    {
        reactive::EffectScope scope {graph};
        scope.add([&] { observed = store.count.get(); });

        const auto theme = theme::default_theme();
        app::NanRouter router {graph, theme, &store, app::nan_type_key<TestStore>()};
        router.push<HomePage>(HomeParams {.user_id = 1});
        REQUIRE(observed == 1);

        router.push<DetailPage>(DetailParams {.blog_id = 9});
        REQUIRE(observed == 9);

        store.count.set(12);
        REQUIRE(observed == 12);
    }
}

TEST_CASE("router supports no-params pages and passes theme through context", "[app][router][theme]") {
    reactive::Graph graph;
    auto app_theme = theme::default_theme();
    app_theme.palette.primary = theme::nan_color(0.72F, 0.12F, 120.0F);
    app::NanRouter router {graph, app_theme};

    auto& page = router.push<PlainPage>();
    REQUIRE(page.params_type_key() == app::nan_type_key<app::NoParams>());
    REQUIRE(router.depth() == 1);
    REQUIRE(router.current_key() == "plain");

    auto* root = router.host()->get_child(0)->as_node2d();
    REQUIRE(root != nullptr);
    auto* control = static_cast<scene::NanControl*>(root);
    REQUIRE(control->background().has_value());
    REQUIRE(control->background()->alpha() == app_theme.palette.primary.alpha());
}

TEST_CASE("router clears page reactive scope when a frame is popped", "[app][router][scope]") {
    reactive::Graph graph;
    TestStore store {graph};
    ScopedObserverLog log;
    const auto theme = theme::default_theme();
    app::NanRouter router {graph, theme, &store, app::nan_type_key<TestStore>()};

    router.push<HomePage>(HomeParams {.user_id = 1});
    router.push<ScopedObserverPage>(ScopedObserverParams {.log = &log});

    REQUIRE(log.observed == 1);
    REQUIRE(log.runs == 1);

    store.count.set(5);
    REQUIRE(log.observed == 5);
    REQUIRE(log.runs == 2);

    REQUIRE(router.pop());
    store.count.set(9);
    REQUIRE(log.observed == 5);
    REQUIRE(log.runs == 2);
}
