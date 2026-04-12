module;

#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

export module DemoPage;

import Nandina.Core;
import Nandina.Window;
import Nandina.Layout;
import Nandina.Components.Label;

namespace demo_detail {

class NumberListView final : public Nandina::CompositeComponent {
public:
    static auto Create(Nandina::StateList<int>& items,
                       Nandina::ReadState<std::optional<int>> selected_value,
                       std::function<void(int)> on_select) -> std::unique_ptr<NumberListView> {
        auto self = std::unique_ptr<NumberListView>(
            new NumberListView(items, selected_value, std::move(on_select)));
        self->initialize();
        return self;
    }

    auto build() -> Nandina::WidgetPtr override {
        auto shell = Nandina::Column::Create();
        shell->set_background(36, 40, 58, 255);
        shell->set_border_radius(12.0f);
        shell->padding(12.0f).gap(8.0f);

        auto title = Nandina::Label::Create(Nandina::Prop<std::string>{std::string{"ListView"}});
        title->set_bounds(0.0f, 0.0f, 240.0f, 20.0f);
        title->font_size(13.0f).text_color(156, 196, 255);
        shell->add(std::move(title));

        auto hint = Nandina::Label::Create(Nandina::Prop<std::string>{std::string{"Click an item to select it."}});
        hint->set_bounds(0.0f, 0.0f, 240.0f, 18.0f);
        hint->font_size(11.0f).text_color(166, 174, 194);
        shell->add(std::move(hint));

        auto items_host = Nandina::Column::Create();
        items_host->set_background(25, 28, 41, 255);
        items_host->set_border_radius(10.0f);
        items_host->padding(8.0f).gap(6.0f);
        items_host->set_bounds(0.0f, 0.0f, 240.0f, 220.0f);
        items_host_ = items_host.get();
        shell->add(std::move(items_host));

        rebuild_rows();
        list_watch_ = Nandina::ScopedConnection{items_.watch([this] { rebuild_rows(); })};
        effect([this] {
            (void)selected_value_();
            refresh_row_styles();
        });

        return shell;
    }

private:
    NumberListView(Nandina::StateList<int>& items,
                   Nandina::ReadState<std::optional<int>> selected_value,
                   std::function<void(int)> on_select)
        : items_(items), selected_value_(selected_value), on_select_(std::move(on_select)) {}

    auto rebuild_rows() -> void {
        if (!items_host_) {
            return;
        }

        items_host_->clear_children_ref();
        row_buttons_.clear();

        const auto& items = items_.items();
        row_buttons_.reserve(items.size());

        for (int value : items) {
            auto row = Nandina::Button::Create();
            row->set_bounds(0.0f, 0.0f, 224.0f, 34.0f);
            row->text(std::format("item {}", value));
            row->on_click([this, value] {
                on_select_(value);
            });

            row_buttons_.push_back(std::move(row));
            items_host_->add_child_ref(row_buttons_.back().get());
        }

        refresh_row_styles();
        items_host_->layout();
        items_host_->mark_dirty();
    }

    auto refresh_row_styles() -> void {
        const auto selected = selected_value_();

        for (auto& row : row_buttons_) {
            const bool is_selected = selected.has_value() && row->get_text() == std::format("item {}", *selected);
            if (is_selected) {
                row->set_background(85, 132, 255);
            } else {
                row->set_background(68, 78, 111);
            }
        }

        if (items_host_) {
            items_host_->mark_dirty();
        }
    }

    Nandina::StateList<int>& items_;
    Nandina::ReadState<std::optional<int>> selected_value_;
    std::function<void(int)> on_select_;
    Nandina::Column* items_host_ = nullptr;
    std::vector<std::unique_ptr<Nandina::Button>> row_buttons_;
    Nandina::ScopedConnection list_watch_;
};

} // namespace demo_detail

export class DemoPage : public Nandina::Page {
public:
    static auto Create() -> std::unique_ptr<DemoPage>;

    auto build() -> Nandina::WidgetPtr override;

private:
    auto select_value(int value) -> void;

    Nandina::StateList<int> items_;
    Nandina::State<std::optional<int>> selected_value_{std::nullopt};
    Nandina::State<int> next_value_{4};
    Nandina::State<std::string> status_text_{"Press Add item to append data, then click a row to inspect it."};

    Nandina::Computed<std::function<std::string()>> selected_text_{
        std::function<std::string()>{
            [this] {
                const auto selected = selected_value_();
                if (!selected.has_value()) {
                    return std::string{"Selected item: none"};
                }

                const auto& items = items_.tracked_items();
                for (int value : items) {
                    if (value == *selected) {
                        return std::format("Selected item: {}", value);
                    }
                }

                return std::string{"Selected item: none"};
            }
        }
    };

    Nandina::Computed<std::function<std::string()>> summary_text_{
        std::function<std::string()>{
            [this] {
                const auto count = items_.tracked_size();
                if (count == 0) {
                    return std::string{"List summary: empty"};
                }

                const auto& values = items_.tracked_items();
                return std::format("List summary: {} items, newest={}, next={}",
                                   count,
                                   values.back(),
                                   next_value_());
            }
        }
    };
};

auto DemoPage::Create() -> std::unique_ptr<DemoPage> {
    auto self = std::make_unique<DemoPage>();
    self->initialize();
    return self;
}

auto DemoPage::select_value(int value) -> void {
    selected_value_.set(value);
    status_text_.set(std::format("You selected item {} from the list.", value));
}

auto DemoPage::build() -> Nandina::WidgetPtr {
    items_.assign_all({1, 2, 3});

    auto root = Nandina::Column::Create();
    root->set_bounds(0.0f, 0.0f, 640.0f, 480.0f);
    root->set_background(25, 28, 40, 255);
    root->padding(24.0f).gap(14.0f);

    auto title = Nandina::Label::Create(Nandina::Prop<std::string>{std::string{"Reactive List Selection Demo"}});
    title->set_bounds(0.0f, 0.0f, 592.0f, 32.0f);
    title->font_size(24.0f).text_color(218, 228, 255);
    root->add(std::move(title));

    auto intro = Nandina::Label::Create(Nandina::Prop<std::string>{std::string{"This page validates StateList, tracked_* bridge APIs, and batched State updates together."}});
    intro->set_bounds(0.0f, 0.0f, 592.0f, 20.0f);
    intro->font_size(12.0f).text_color(168, 176, 198);
    root->add(std::move(intro));

    auto body = Nandina::Row::Create();
    body->set_bounds(0.0f, 0.0f, 592.0f, 260.0f);
    body->gap(16.0f);
    body->set_background(0, 0, 0, 0);

    auto list_view = demo_detail::NumberListView::Create(items_, selected_value_.as_read_only(), [this](int value) {
        select_value(value);
    });
    list_view->set_bounds(0.0f, 0.0f, 260.0f, 260.0f);
    body->add(std::move(list_view));

    auto detail_panel = Nandina::Column::Create();
    detail_panel->set_bounds(0.0f, 0.0f, 316.0f, 260.0f);
    detail_panel->set_background(36, 40, 58, 255);
    detail_panel->set_border_radius(12.0f);
    detail_panel->padding(16.0f).gap(12.0f);

    auto detail_title = Nandina::Label::Create(Nandina::Prop<std::string>{std::string{"Selection detail"}});
    detail_title->set_bounds(0.0f, 0.0f, 284.0f, 22.0f);
    detail_title->font_size(16.0f).text_color(196, 225, 255);
    detail_panel->add(std::move(detail_title));

    auto selected_label = Nandina::Label::Create();
    selected_label->set_bounds(0.0f, 0.0f, 284.0f, 30.0f);
    selected_label->font_size(22.0f).text_color(255, 236, 170);
    auto* selected_label_ptr = selected_label.get();
    detail_panel->add(std::move(selected_label));

    effect([this, selected_label_ptr] {
        selected_label_ptr->text(selected_text_());
    });

    auto summary_label = Nandina::Label::Create();
    summary_label->set_bounds(0.0f, 0.0f, 284.0f, 18.0f);
    summary_label->font_size(12.0f).text_color(160, 208, 185);
    auto* summary_label_ptr = summary_label.get();
    detail_panel->add(std::move(summary_label));

    effect([this, summary_label_ptr] {
        summary_label_ptr->text(summary_text_());
    });

    auto status_label = Nandina::Label::Create(Nandina::Prop<std::string>{status_text_.as_read_only()});
    status_label->set_bounds(0.0f, 0.0f, 284.0f, 54.0f);
    status_label->font_size(12.0f).text_color(182, 190, 208);
    detail_panel->add(std::move(status_label));

    auto action_row = Nandina::Row::Create();
    action_row->set_bounds(0.0f, 0.0f, 284.0f, 36.0f);
    action_row->gap(10.0f);
    action_row->set_background(0, 0, 0, 0);

    auto add_button = Nandina::Button::Create();
    add_button->set_bounds(0.0f, 0.0f, 132.0f, 36.0f);
    add_button->text("Add item");
    add_button->set_background(79, 119, 234);
    add_button->on_click([this] {
        const int value = next_value_.get();
        Nandina::batch([this, value] {
            items_.push_back(value);
            selected_value_.set(value);
            next_value_.set(value + 1);
            status_text_.set(std::format("Added item {} and selected it in one batched update.", value));
        });
    });
    action_row->add(std::move(add_button));

    auto clear_button = Nandina::Button::Create();
    clear_button->set_bounds(0.0f, 0.0f, 132.0f, 36.0f);
    clear_button->text("Clear selection");
    clear_button->set_background(104, 112, 135);
    clear_button->on_click([this] {
        selected_value_.set(std::nullopt);
        status_text_.set("Selection cleared. The list stays intact.");
    });
    action_row->add(std::move(clear_button));
    detail_panel->add(std::move(action_row));

    auto note = Nandina::Label::Create(Nandina::Prop<std::string>{std::string{"Rows come from StateList; detail text comes from tracked_items() + Computed."}});
    note->set_bounds(0.0f, 0.0f, 284.0f, 34.0f);
    note->font_size(11.0f).text_color(148, 156, 176);
    detail_panel->add(std::move(note));

    body->add(std::move(detail_panel));
    root->add(std::move(body));

    return root;
}
