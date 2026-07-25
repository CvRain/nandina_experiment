#include "todo_app.hpp"

#include "app/nan_router.hpp"
#include "app/ui_dispatcher.hpp"
#include "foundation/geometry.hpp"
#include "scene/scene_tree.hpp"
#include "semantics/semantics.hpp"
#include "theme/theme_manager.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string_view>
#include <type_traits>

using namespace nandina;
using namespace nandina::examples::todo;

namespace
{
    void
    activate(scene::NanSceneTree& tree, app::UiDispatcher& dispatcher, widget::Button& button) {
        REQUIRE(tree.update_semantics());
        {
            auto phase = tree.enter_phase(scene::FramePhase::input);
            REQUIRE(tree.perform_semantics_action(
                button.semantics_id(),
                {.action = semantics::Action::activate}
            ));
        }
        REQUIRE(dispatcher.pending_count() == 1);
    }

    void drain_tasks(scene::NanSceneTree& tree, app::UiDispatcher& dispatcher) {
        auto phase = tree.enter_phase(scene::FramePhase::tasks);
        REQUIRE(dispatcher.drain() == 1);
    }
} // namespace

TEST_CASE("imperative and DSL Todo pages share components and state", "[todo][authoring][router]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    TodoStore store {graph};
    app::UiDispatcher dispatcher;
    app::NanRouter router {
        graph,
        themes,
        &store,
        app::nan_type_key<TodoStore>(),
        nullptr,
        nullptr,
        nullptr,
        &dispatcher
    };
    auto& imperative =
        router.push<ImperativeTodoPage>(TodoPageParams {.source = "测试入口", .visit = 1});
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(router.host());
    REQUIRE(tree.layout_root(foundation::NanSize(760.0F, 420.0F)) == 1);

    static_assert(
        std::same_as<decltype(imperative.workspace()), const std::shared_ptr<TodoWorkspace>&>
    );
    REQUIRE(imperative.workspace()->name() == "todo-imperative-root");
    REQUIRE(imperative.workspace()->header().parameter_text().contains("测试入口"));
    REQUIRE(imperative.workspace()->header().parameter_text().contains("第 1 次"));
    REQUIRE(imperative.workspace()->list().row_count() == 3);

    imperative.workspace()->composer().input().set_value("命令式页面新增");
    REQUIRE(imperative.workspace()->composer().submit());
    REQUIRE(store.items.peek().size() == 4);
    REQUIRE(imperative.workspace()->list().row_count() == 4);

    activate(tree, dispatcher, imperative.workspace()->header().navigation_button());
    REQUIRE(router.depth() == 1);
    drain_tasks(tree, dispatcher);
    REQUIRE(router.depth() == 2);
    REQUIRE(router.current_key() == "todo-dsl");
    REQUIRE(router.host()->child_count() == 2);

    auto* dsl = static_cast<TodoWorkspace*>(router.host()->get_child(1));
    REQUIRE(dsl->name() == "todo-dsl-root");
    REQUIRE(dsl->header().parameter_text().contains("命令式页面"));
    REQUIRE(dsl->header().parameter_text().contains("第 2 次"));
    REQUIRE(dsl->list().row_count() == 4);

    dsl->composer().input().set_value("DSL 页面新增");
    REQUIRE(dsl->composer().submit());
    REQUIRE(store.items.peek().size() == 5);
    REQUIRE(dsl->list().row_count() == 5);
    REQUIRE(imperative.workspace()->list().row_count() == 5);

    activate(tree, dispatcher, dsl->header().navigation_button());
    REQUIRE(router.depth() == 2);
    drain_tasks(tree, dispatcher);
    REQUIRE(router.depth() == 1);
    REQUIRE(router.current_key() == "todo-imperative");
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE(imperative.workspace()->visible());
    REQUIRE(imperative.workspace()->list().row_count() == 5);

    // keep-alive 页面恢复后，其导航回调仍可再次投递。
    activate(tree, dispatcher, imperative.workspace()->header().navigation_button());
    drain_tasks(tree, dispatcher);
    REQUIRE(router.depth() == 2);
    REQUIRE(router.current_key() == "todo-dsl");
}

TEST_CASE("DSL Todo page can pass params to a new imperative page", "[todo][params][router]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    TodoStore store {graph};
    app::UiDispatcher dispatcher;
    app::NanRouter router {
        graph,
        themes,
        &store,
        app::nan_type_key<TodoStore>(),
        nullptr,
        nullptr,
        nullptr,
        &dispatcher
    };
    auto& dsl = router.push<DslTodoPage>(TodoPageParams {.source = "直接入口", .visit = 1});
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(router.host());
    REQUIRE(tree.layout_root(foundation::NanSize(760.0F, 420.0F)) == 1);

    REQUIRE(dsl.workspace()->header().parameter_text().contains("直接入口"));
    activate(tree, dispatcher, dsl.workspace()->header().navigation_button());
    REQUIRE(router.depth() == 1);
    drain_tasks(tree, dispatcher);
    REQUIRE(router.depth() == 2);
    REQUIRE(router.current_key() == "todo-imperative");

    auto* imperative = static_cast<TodoWorkspace*>(router.host()->get_child(1));
    REQUIRE(imperative->header().parameter_text().contains("DSL 页面"));
    REQUIRE(imperative->header().parameter_text().contains("第 2 次"));
}
