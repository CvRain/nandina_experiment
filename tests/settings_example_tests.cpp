#include "settings_example.hpp"

#include "app/nan_router.hpp"
#include "app/root_view.hpp"
#include "foundation/geometry.hpp"
#include "scene/input_event.hpp"
#include "scene/scene_tree.hpp"
#include "semantics/semantics.hpp"
#include "theme/theme_manager.hpp"
#include "widget/button.hpp"
#include "widget/checkbox.hpp"
#include "widget/label.hpp"
#include "widget/slider.hpp"
#include "widget/text_field.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string_view>

using namespace nandina;

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

    [[nodiscard]] auto checkbox_named(scene::NanNode& root, const std::string_view label)
        -> widget::Checkbox* {
        return find_node<widget::Checkbox>(root, [label](const widget::Checkbox& checkbox) {
            return checkbox.label() == label;
        });
    }

    [[nodiscard]] auto button_named(scene::NanNode& root, const std::string_view text)
        -> widget::Button* {
        return find_node<widget::Button>(root, [text](const widget::Button& button) {
            return button.text() == text;
        });
    }
} // namespace

TEST_CASE("settings example exercises stateful controls through semantics", "[example][settings]") {
    reactive::Graph graph;
    theme::ThemeManager themes;
    app::NanRouter router {graph, themes};
    (void)router.push<app::detail::RootViewPage>(
        app::detail::make_root_view_params(examples::settings::build)
    );
    scene::NanSceneTree tree;
    tree.set_theme_manager(themes);
    tree.set_root(router.host());
    REQUIRE(tree.layout_root(foundation::NanSize(720.0F, 520.0F)) >= 1);

    auto* diagnostics = checkbox_named(*router.host(), "Send anonymous diagnostics");
    auto* notifications = checkbox_named(*router.host(), "Desktop notifications");
    auto* input = find_node<widget::TextField>(*router.host(), [](const auto&) { return true; });
    auto* save = button_named(*router.host(), "Save preferences");
    auto* reset = button_named(*router.host(), "Reset");
    auto* scale = find_node<widget::Slider>(*router.host(), [](const auto&) { return true; });
    REQUIRE(diagnostics != nullptr);
    REQUIRE(notifications != nullptr);
    REQUIRE(input != nullptr);
    REQUIRE(save != nullptr);
    REQUIRE(reset != nullptr);
    REQUIRE(scale != nullptr);
    REQUIRE(tree.focused_node() == input);
    REQUIRE(notifications->checked());
    REQUIRE_FALSE(diagnostics->checked());
    REQUIRE(scale->value() == 1.0F);

    REQUIRE(tree.update_semantics());
    REQUIRE(tree.perform_semantics_action(
        diagnostics->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(diagnostics->checked());
    REQUIRE(tree.perform_semantics_action(
        scale->semantics_id(),
        {.action = semantics::Action::set_value, .value = "1.25"}
    ));
    REQUIRE(scale->value() == 1.25F);
    REQUIRE(
        find_node<widget::Label>(
            *router.host(),
            [](const widget::Label& label) { return label.text().contains("diagnostics help"); }
        )
        != nullptr
    );

    tree.dispatch_key(scene::KeyEvent(65, scene::KeyEvent::Action::press, {.ctrl = true}));
    tree.dispatch_text_input(scene::TextInputEvent("Review profile"));
    REQUIRE(tree.update_semantics());
    REQUIRE(
        tree.perform_semantics_action(save->semantics_id(), {.action = semantics::Action::activate})
    );
    REQUIRE(
        find_node<widget::Label>(
            *router.host(),
            [](const widget::Label& label) {
                return label.text() == "Preferences saved for Review profile";
            }
        )
        != nullptr
    );

    REQUIRE(tree.update_semantics());
    REQUIRE(tree.perform_semantics_action(
        reset->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(input->value() == "Nandina developer");
    REQUIRE(notifications->checked());
    REQUIRE_FALSE(diagnostics->checked());
    REQUIRE(scale->value() == 1.0F);
}
