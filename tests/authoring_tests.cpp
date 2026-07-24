#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "reactive/graph.hpp"
#include "reactive/signal.hpp"
#include "scene/canvas_layer.hpp"
#include "scene/scene_tree.hpp"
#include "semantics/semantics.hpp"
#include "widget/authoring.hpp"
#include "widget/button.hpp"
#include "widget/declarative.hpp"
#include "widget/label.hpp"
#include "widget/layout.hpp"

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using namespace nandina;

namespace
{
    struct Item {
        int key = 0;
        std::string value;

        auto operator==(const Item&) const -> bool = default;
    };

    class ItemControl final: public scene::NanControl {
    public:
        explicit ItemControl(const int key): key(key) {}

        int key;
        std::string value;
    };
} // namespace

TEST_CASE("authoring builders preserve concrete widget identity", "[authoring][widget]") {
    reactive::Graph graph;
    std::shared_ptr<widget::Label> title;
    std::shared_ptr<widget::Button> action;

    auto root = widget::authoring::make<widget::Column>()
                    .configure([](widget::Column& column) {
                        column.set_gap(12.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
                    })
                    .children(
                        widget::authoring::make<widget::Label>(graph, "Tasks").expose(title),
                        widget::authoring::make<widget::Button>("Add")
                            .configure([](widget::Button& button) {
                                button.set_treatment(theme::ButtonTreatment::outlined);
                            })
                            .expose(action)
                    )
                    .build();

    static_assert(std::same_as<decltype(root), std::shared_ptr<widget::Column>>);
    REQUIRE(root->child_count() == 2);
    REQUIRE(root->get_child(0) == title.get());
    REQUIRE(root->get_child(1) == action.get());
    REQUIRE(title->text() == "Tasks");
    REQUIRE(action->treatment() == theme::ButtonTreatment::outlined);
    REQUIRE(root->gap() == Catch::Approx(12.0F));
}

TEST_CASE(
    "imperative and authored trees resolve equivalent layout and style",
    "[authoring][layout][style]"
) {
    auto imperative_button = widget::Button::create("Continue");
    imperative_button->set_tone(theme::ButtonTone::secondary);
    imperative_button->set_treatment(theme::ButtonTreatment::outlined);
    auto imperative = widget::Column::create();
    imperative->set_gap(7.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
    imperative->add(imperative_button);

    std::shared_ptr<widget::Button> authored_button;
    auto authored =
        widget::authoring::make<widget::Column>()
            .configure([](widget::Column& value) {
                value.set_gap(7.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
            })
            .children(
                widget::authoring::make<widget::Button>("Continue")
                    .configure([](widget::Button& value) {
                        value.set_tone(theme::ButtonTone::secondary);
                        value.set_treatment(theme::ButtonTreatment::outlined);
                    })
                    .expose(authored_button)
            )
            .build();

    scene::NanSceneTree imperative_tree;
    imperative_tree.set_root(imperative);
    scene::NanSceneTree authored_tree;
    authored_tree.set_root(authored);
    REQUIRE(imperative_tree.layout_root(foundation::NanSize(240.0F, 100.0F)) == 1);
    REQUIRE(authored_tree.layout_root(foundation::NanSize(240.0F, 100.0F)) == 1);

    REQUIRE(authored_button->global_bounds() == imperative_button->global_bounds());
    REQUIRE(
        authored_button->resolved_style().border_width
        == Catch::Approx(imperative_button->resolved_style().border_width)
    );
    REQUIRE(authored_button->tone() == imperative_button->tone());
    REQUIRE(authored_button->treatment() == imperative_button->treatment());
}

TEST_CASE("authoring child and expose use the original concrete nodes", "[authoring][widget]") {
    std::shared_ptr<widget::Button> button;
    auto padding = widget::authoring::make<widget::Padding>(foundation::NanInsets::all(8.0F))
                       .child(widget::authoring::make<widget::Button>("Save").expose(button))
                       .build();

    REQUIRE(padding->child_count() == 1);
    REQUIRE(padding->get_child(0) == button.get());
    REQUIRE(button->parent() == padding.get());
}

TEST_CASE("authored bindings keep the concrete widget lifecycle", "[authoring][reactive]") {
    reactive::Graph graph;
    reactive::Signal<std::string> text {graph, "first"};
    auto label = widget::authoring::make<widget::Label>(graph)
                     .configure([&](widget::Label& value) { value.bind_text(text); })
                     .build();

    scene::NanSceneTree tree;
    tree.set_root(label);
    REQUIRE(label->text() == "first");
    text.set("second");
    REQUIRE(label->text() == "second");

    tree.set_root({});
    text.set("after detach");
    REQUIRE(label->text() == "second");
}

TEST_CASE("authored buttons retain input and semantics behavior", "[authoring][semantics]") {
    int activations = 0;
    auto button =
        widget::authoring::make<widget::Button>("Run")
            .configure([&](widget::Button& value) { value.set_on_click([&] { ++activations; }); })
            .build();
    scene::NanSceneTree tree;
    tree.set_root(button);
    REQUIRE(tree.layout_root(foundation::NanSize(160.0F, 60.0F)) == 1);
    REQUIRE(tree.update_semantics());

    REQUIRE(tree.perform_semantics_action(
        button->semantics_id(),
        {.action = semantics::Action::activate}
    ));
    REQUIRE(activations == 1);
}

TEST_CASE("authoring wraps keyed regions without changing reuse", "[authoring][declarative]") {
    reactive::Graph graph;
    reactive::Signal<std::vector<Item>> items {
        graph,
        {{.key = 1, .value = "one"}, {.key = 2, .value = "two"}},
    };
    using Region = widget::ForEach<Item, int, ItemControl>;
    auto region = widget::authoring::from(
                      Region::create(
                          graph,
                          [](const Item& item) { return item.key; },
                          [](reactive::ReactiveScope&, const Item& item) {
                              return std::make_shared<ItemControl>(item.key);
                          },
                          [](ItemControl& control, const Item& item) { control.value = item.value; }
                      )
    ).configure([&](Region& value) {
         value.bind(items);
     }).build();
    scene::NanSceneTree tree;
    tree.set_root(region);

    auto* first = region->node_for(1);
    items.set({{.key = 2, .value = "updated"}, {.key = 1, .value = "retained"}});
    REQUIRE(region->node_for(1) == first);
    REQUIRE(first->value == "retained");
    REQUIRE(region->get_child(1) == first);
}

TEST_CASE("authoring configures existing canvas factories", "[authoring][canvas]") {
    auto layer = widget::authoring::from(scene::CanvasLayer::create(scene::CanvasSpace::world, 2))
                     .configure([](scene::CanvasLayer& value) {
                         value.set_input_mode(scene::LayerInputMode::block_below);
                     })
                     .build();
    auto stack = widget::authoring::from(scene::LayerStack::create())
                     .configure([&](scene::LayerStack& value) { value.add_layer(layer); })
                     .build();

    static_assert(std::same_as<decltype(layer), std::shared_ptr<scene::CanvasLayer>>);
    REQUIRE(stack->layer_at(0) == layer.get());
    REQUIRE(layer->order() == 2);
    REQUIRE(layer->input_mode() == scene::LayerInputMode::block_below);
}

TEST_CASE("authoring rejects null factory results", "[authoring]") {
    REQUIRE_THROWS_AS(
        widget::authoring::from(std::shared_ptr<widget::Button> {}),
        std::invalid_argument
    );
}
