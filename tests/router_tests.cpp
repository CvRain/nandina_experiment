//
// Router + Store tests.
//

#include "app/nan_router.hpp"
#include "app/nan_store.hpp"
#include "foundation/geometry.hpp"
#include "reactive/effect.hpp"
#include "reactive/signal.hpp"
#include "scene/control.hpp"

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
} // namespace

TEST_CASE("router pushes keep-alive pages and toggles top visibility", "[app][router]") {
    reactive::Graph graph;
    TestStore store {graph};
    app::NanRouter router {graph};
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
    app::NanRouter router {graph, &store, app::nan_type_key<TestStore>()};

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

        app::NanRouter router {graph, &store, app::nan_type_key<TestStore>()};
        router.push<HomePage>(HomeParams {.user_id = 1});
        REQUIRE(observed == 1);

        router.push<DetailPage>(DetailParams {.blog_id = 9});
        REQUIRE(observed == 9);

        store.count.set(12);
        REQUIRE(observed == 12);
    }
}
