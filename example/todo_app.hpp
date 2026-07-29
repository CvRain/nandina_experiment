//
// Todo example application components and paired page authoring forms.
//

#ifndef NANDINA_EXPERIMENT_EXAMPLE_TODO_APP_HPP
#define NANDINA_EXPERIMENT_EXAMPLE_TODO_APP_HPP

#include "app/nan_page.hpp"
#include "app/nan_store.hpp"
#include "reactive/computed.hpp"
#include "reactive/event.hpp"
#include "reactive/signal.hpp"
#include "theme/theme_manager.hpp"
#include "widget/button.hpp"
#include "widget/declarative.hpp"
#include "widget/label.hpp"
#include "widget/layout.hpp"
#include "widget/list_view.hpp"
#include "widget/scroll_view.hpp"
#include "widget/text_field.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::examples::todo
{
    struct TodoItem {
        std::uint64_t id = 0;
        std::string title;
        bool completed = false;

        auto operator==(const TodoItem&) const -> bool = default;
    };

    class TodoStore final: public app::NanStore {
    public:
        explicit TodoStore(reactive::Graph& graph);

        [[nodiscard]] auto add(std::string_view title) -> bool;
        void toggle(std::uint64_t id);
        void remove(std::uint64_t id);
        void bump_visit();

        reactive::Signal<std::vector<TodoItem>> items;
        reactive::Signal<std::uint64_t> visits;

    private:
        std::uint64_t next_id_ = 4;
    };

    struct TodoPageParams {
        std::string source = "应用启动";
    };

    class TodoRow final: public widget::Row {
    public:
        using Action = std::function<void(std::uint64_t)>;

        TodoRow(reactive::Graph& graph, theme::NanTheme theme, Action toggle, Action remove);
        void update(const TodoItem& item, const theme::NanTheme& theme);
        [[nodiscard]] auto toggle_button() -> widget::Button&;
        [[nodiscard]] auto remove_button() -> widget::Button&;
        void on_theme_changed(const theme::ThemeManager& manager) override;

    private:
        Action toggle_;
        Action remove_;
        std::shared_ptr<widget::Label> label_;
        std::shared_ptr<widget::Button> toggle_button_;
        std::shared_ptr<widget::Button> remove_button_;
        std::uint64_t id_ = 0;
        bool completed_ = false;
    };

    class TodoEmptyState final: public widget::Label {
    public:
        TodoEmptyState(reactive::Graph& graph, theme::NanTheme theme);
        void on_theme_changed(const theme::ThemeManager& manager) override;
    };

    class TodoHeader final: public widget::Column {
    public:
        TodoHeader(
            reactive::Graph& graph,
            reactive::Computed<std::string>& status,
            reactive::Computed<std::string>& visit_text,
            std::string authoring_label,
            std::string navigation_label,
            theme::ThemeManager& themes,
            app::UiDispatcher& dispatcher,
            std::function<void()> navigate
        );

        [[nodiscard]] auto navigation_button() -> widget::Button&;
        [[nodiscard]] auto parameter_text() const -> std::string_view;
        void on_theme_changed(const theme::ThemeManager& manager) override;

    private:
        [[nodiscard]] static auto next_preference(theme::ThemePreference preference)
            -> theme::ThemePreference;
        [[nodiscard]] static auto preference_label(theme::ThemePreference preference)
            -> std::string;

        theme::ThemeManager* themes_;
        std::shared_ptr<widget::Label> status_;
        std::shared_ptr<widget::Label> parameters_;
        std::shared_ptr<widget::Button> navigation_button_;
        std::shared_ptr<widget::Button> theme_button_;
    };

    class TodoComposer final: public widget::Row {
    public:
        using Submit = std::function<bool(std::string_view)>;

        TodoComposer(theme::NanTheme theme, Submit submit);
        [[nodiscard]] auto input() -> widget::TextField&;
        [[nodiscard]] auto add_button() -> widget::Button&;
        [[nodiscard]] auto submit() -> bool;

    private:
        [[nodiscard]] auto submit_value(std::string_view value) -> bool;

        Submit submit_;
        std::shared_ptr<widget::TextField> input_;
        std::shared_ptr<widget::Button> add_button_;
    };

    class TodoTasks final: public widget::Expanded {
    public:
        using Action = std::function<void(std::uint64_t)>;

        TodoTasks(
            reactive::Graph& graph,
            reactive::Computed<bool>& empty,
            TodoStore& store,
            theme::NanTheme theme
        );

        void on_toggle(Action action);
        void on_remove(Action action);
        void request_scroll_to_end();
        [[nodiscard]] auto row_count() const -> std::size_t;
        [[nodiscard]] auto row(std::uint64_t id) -> TodoRow*;
        [[nodiscard]] auto scroll_view() -> widget::ScrollView&;
        void on_theme_changed(const theme::ThemeManager& manager) override;

    private:
        using Rows = widget::ListView<TodoItem, std::uint64_t, TodoRow>;

        theme::NanTheme theme_;
        reactive::Event<std::uint64_t> toggle_requested_;
        reactive::Event<std::uint64_t> remove_requested_;
        reactive::Subscription toggle_subscription_;
        reactive::Subscription remove_subscription_;
        std::shared_ptr<widget::ScrollView> list_view_;
        std::shared_ptr<Rows> rows_;
    };

    using TodoList = TodoTasks;

    class TodoWorkspace final: public scene::NanControl {
    public:
        TodoWorkspace(
            theme::ThemeManager& themes,
            std::shared_ptr<TodoHeader> header,
            std::shared_ptr<TodoComposer> composer,
            std::shared_ptr<TodoList> list
        );

        void set_content(std::shared_ptr<scene::NanControl> content);
        [[nodiscard]] auto header() -> TodoHeader&;
        [[nodiscard]] auto composer() -> TodoComposer&;
        [[nodiscard]] auto list() -> TodoList&;
        void on_ready() override;
        void on_theme_changed(const theme::ThemeManager& manager) override;

    private:
        std::shared_ptr<TodoHeader> header_;
        std::shared_ptr<TodoComposer> composer_;
        std::shared_ptr<TodoList> list_;
    };

    class DslTodoPage;

    class ImperativeTodoPage final: public app::NanPageT<TodoPageParams> {
    public:
        explicit ImperativeTodoPage(TodoPageParams params);
        [[nodiscard]] auto route_key() const -> std::string_view override;
        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override;
        void on_activate(app::PageContext& context) override;
        [[nodiscard]] auto workspace() const -> const std::shared_ptr<TodoWorkspace>&;

    private:
        std::shared_ptr<TodoWorkspace> workspace_;
    };

    class DslTodoPage final: public app::NanPageT<TodoPageParams> {
    public:
        explicit DslTodoPage(TodoPageParams params);
        [[nodiscard]] auto route_key() const -> std::string_view override;
        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override;
        void on_activate(app::PageContext& context) override;
        [[nodiscard]] auto workspace() const -> const std::shared_ptr<TodoWorkspace>&;

    private:
        std::shared_ptr<TodoWorkspace> workspace_;
    };
} // namespace nandina::examples::todo

#endif // NANDINA_EXPERIMENT_EXAMPLE_TODO_APP_HPP
