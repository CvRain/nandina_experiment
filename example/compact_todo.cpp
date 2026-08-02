//
// Compact Todo — UI composition and component logic only.
//

#include "compact_todo.hpp"

#include "foundation/geometry.hpp"
#include "semantics/semantics.hpp"
#include "widget/build_context.hpp"
#include "widget/layout.hpp"
#include "widget/primitives/text.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <ranges>
#include <utility>

namespace nandina::examples::compact_todo
{
    namespace
    {
        [[nodiscard]] auto has_text(const std::string_view value) -> bool {
            return std::ranges::any_of(value, [](const char character) {
                return std::isspace(static_cast<unsigned char>(character)) == 0;
            });
        }

        [[nodiscard]] auto summary(Store& store) -> std::string {
            const auto& tasks = store.tasks.get();
            const auto completed = std::ranges::count(tasks, true, &Task::completed);
            return std::to_string(tasks.size()) + " tasks · " + std::to_string(completed)
                + " completed";
        }

        class TaskRow final: public widget::Row {
        public:
            using Action = std::function<void(std::uint64_t)>;

            TaskRow(widget::BuildContext ui, Action toggle, Action remove):
                toggle_(std::move(toggle)),
                remove_(std::move(remove)) {
                label_ = ui.label().color_token(theme::ColorToken::on_surface).build();
                label_->set_overflow(widget::primitives::TextOverflow::clip);
                toggle_button_ = ui.button("Done")
                                     .treatment(theme::ButtonTreatment::outlined)
                                     .on_click([this] { toggle_(id_); })
                                     .build();
                auto remove_button = ui.button("Delete")
                                         .tone(theme::ButtonTone::secondary)
                                         .treatment(theme::ButtonTreatment::outlined)
                                         .on_click([this] { remove_(id_); })
                                         .build();

                auto title = ui.expanded().child(label_).build();
                set_gap(8.0F)
                    .set_cross_alignment(widget::LayoutAlignment::stretch)
                    .add(title)
                    .add(toggle_button_)
                    .add(remove_button);
            }

            void update(const Task& task) {
                id_ = task.id;
                label_->set_text((task.completed ? "[Done] " : "[Todo] ") + task.title);
                label_->set_color_token(
                    task.completed ? theme::ColorToken::on_surface_variant
                                   : theme::ColorToken::on_surface
                );
                toggle_button_->set_text(task.completed ? "Undo" : "Done");
                set_semantics_override(
                    semantics::Properties {
                        .role = semantics::Role::list_item,
                        .label = task.title,
                        .value = task.completed ? "completed" : "pending",
                    }
                );
            }

        private:
            Action toggle_;
            Action remove_;
            std::shared_ptr<widget::Label> label_;
            std::shared_ptr<widget::Button> toggle_button_;
            std::uint64_t id_ = 0;
        };
    } // namespace

    Store::Store(reactive::Graph& graph):
        tasks(
            graph,
            {
                {.id = 1, .title = "Build the compact Todo"},
                {.id = 2, .title = "Keep component logic readable"},
            }
        ) {}

    auto Store::add(const std::string_view title) -> bool {
        if (!has_text(title)) {
            return false;
        }
        tasks.update([&](auto& values) {
            values.push_back(Task {.id = next_id_++, .title = std::string(title)});
        });
        return true;
    }

    void Store::toggle(const std::uint64_t id) {
        tasks.update([id](auto& values) {
            if (const auto task = std::ranges::find(values, id, &Task::id); task != values.end()) {
                task->completed = !task->completed;
            }
        });
    }

    void Store::remove(const std::uint64_t id) {
        tasks.update([id](auto& values) {
            std::erase_if(values, [id](const Task& task) { return task.id == id; });
        });
    }

    auto Page::route_key() const -> std::string_view {
        return "compact-todo";
    }

    auto Page::build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> {
        auto ui = context.ui();
        auto& store = context.store<Store>();
        auto& draft = ui.signal<std::string>();
        auto& status = ui.computed([&store] { return summary(store); });
        auto& empty = ui.computed([&store] { return store.tasks.get().empty(); });
        auto submit = [&store, &draft] {
            if (store.add(draft.peek())) {
                draft.set("");
            }
        };

        auto input = ui.text_field(draft, "Add a task")
                         .autofocus()
                         .on_submit([submit](std::string_view) { submit(); })
                         .build();
        auto add = ui.button("Add").tone(theme::ButtonTone::primary).on_click(submit).build();
        auto composer = ui.row()
                            .gap(8.0F)
                            .cross_alignment(widget::LayoutAlignment::stretch)
                            .children(ui.expanded().child(input), add)
                            .build();

        auto empty_state = ui.when(empty, [](widget::BuildContext branch) {
            return branch.label("No tasks").color_token(theme::ColorToken::on_surface_variant);
        });
        auto rows = ui.for_each(
            store.tasks,
            &Task::id,
            [&store](widget::BuildContext item, const Task&) {
                return item.make<TaskRow>(
                    [&store](const std::uint64_t id) { store.toggle(id); },
                    [&store](const std::uint64_t id) { store.remove(id); }
                );
            },
            [](TaskRow& row, const Task& task) { row.update(task); }
        );
        rows.gap(8.0F).cross_alignment(widget::LayoutAlignment::stretch);
        auto list = ui.column()
                        .cross_alignment(widget::LayoutAlignment::stretch)
                        .children(empty_state, rows)
                        .build();
        auto scroll = ui.scroll_view().wheel_step(36.0F).child(list);

        auto content = ui.column()
                           .gap(12.0F)
                           .cross_alignment(widget::LayoutAlignment::stretch)
                           .children(
                               ui.label("Todo").font_size(26.0F),
                               ui.label(status).color_token(theme::ColorToken::on_surface_variant),
                               composer,
                               ui.expanded().child(scroll)
                           );
        return ui.padding(foundation::NanInsets::all(16.0F)).child(content).build();
    }
} // namespace nandina::examples::compact_todo
