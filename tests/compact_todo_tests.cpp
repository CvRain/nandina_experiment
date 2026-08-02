#include "compact_todo.hpp"

#include "app/nan_router.hpp"
#include "app/ui_dispatcher.hpp"
#include "foundation/geometry.hpp"
#include "scene/input_event.hpp"
#include "scene/scene_tree.hpp"
#include "semantics/semantics.hpp"
#include "theme/theme_manager.hpp"
#include "widget/button.hpp"
#include "widget/text_field.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string_view>

using namespace nandina;
using namespace nandina::examples::compact_todo;

namespace
{
    template<typename Node, typename Predicate>
    [[nodiscard]] auto find_node(scene::NanNode& root, Predicate&& predicate) -> Node* {
        if (auto* node = dynamic_cast<Node*>(&root); node != nullptr && predicate(*node)) {
            return node;
        }
        for (std::size_t index = 0; index < root.child_count(); ++index) {
            if (auto* found = find_node<Node>(*root.get_child(index), predicate); found != nullptr)
            {
                return found;
            }
        }
        return nullptr;
    }

    [[nodiscard]] auto button_named(scene::NanNode& root, const std::string_view text)
        -> widget::Button* {
        return find_node<widget::Button>(root, [text](const widget::Button& button) {
            return button.text() == text;
        });
    }
} // namespace

TEST_CASE("compact Todo keeps application code free of lifecycle plumbing", "[todo][compact]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    Store store {graph};
    app::UiDispatcher dispatcher;
    app::NanRouter router {
        graph,
        themes,
        &store,
        app::nan_type_key<Store>(),
        nullptr,
        nullptr,
        nullptr,
        &dispatcher
    };
    (void)router.push<Page>();
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(router.host());
    REQUIRE(tree.layout_root(foundation::NanSize(720.0F, 420.0F)) >= 1);

    auto* input = find_node<widget::TextField>(*router.host(), [](const auto&) { return true; });
    auto* add = button_named(*router.host(), "Add");
    REQUIRE(input != nullptr);
    REQUIRE(add != nullptr);
    REQUIRE(tree.focused_node() == input);
    tree.dispatch_text_input(scene::TextInputEvent("Test compact authoring"));
    REQUIRE(tree.update_semantics());
    REQUIRE(
        tree.perform_semantics_action(add->semantics_id(), {.action = semantics::Action::activate})
    );
    REQUIRE(store.tasks.peek().size() == 3);
    REQUIRE(input->value().empty());

    auto* done = button_named(*router.host(), "Done");
    REQUIRE(done != nullptr);
    REQUIRE(tree.update_semantics());
    REQUIRE(
        tree.perform_semantics_action(done->semantics_id(), {.action = semantics::Action::activate})
    );
    REQUIRE(store.tasks.peek().front().completed);

    auto* remove = button_named(*router.host(), "Delete");
    REQUIRE(remove != nullptr);
    REQUIRE(tree.update_semantics());
    REQUIRE(tree.perform_semantics_action(
        remove->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(store.tasks.peek().size() == 2);

    tree.dispatch_text_input(scene::TextInputEvent("Submit with Enter"));
    tree.dispatch_key(scene::KeyEvent(257, scene::KeyEvent::Action::press));
    REQUIRE(store.tasks.peek().size() == 3);
    REQUIRE(input->value().empty());
}
