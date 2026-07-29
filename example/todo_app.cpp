//
// Todo example application components and paired page authoring forms.
//

#include "todo_app.hpp"

#include "app/nan_router.hpp"
#include "foundation/geometry.hpp"
#include "foundation/nan_logger.hpp"
#include "scene/scene_tree.hpp"
#include "semantics/semantics.hpp"
#include "widget/authoring.hpp"

#include <algorithm>
#include <cctype>
#include <ranges>
#include <utility>

namespace nandina::examples::todo
{
    namespace
    {
        [[nodiscard]] auto has_non_space(const std::string_view text) -> bool {
            return std::ranges::any_of(text, [](const char ch) {
                return std::isspace(static_cast<unsigned char>(ch)) == 0;
            });
        }

        [[nodiscard]] auto status_for(TodoStore& store) -> std::string {
            const auto& items = store.items.get();
            const auto completed =
                static_cast<std::size_t>(std::ranges::count(items, true, &TodoItem::completed));
            return std::to_string(items.size()) + " tasks, " + std::to_string(completed)
                + " completed / 已完成";
        }

        [[nodiscard]] auto
        parameter_summary(const TodoPageParams& params, const std::uint64_t visit) -> std::string {
            return "页面参数：来自 " + params.source + " · 第 " + std::to_string(visit) + " 次访问";
        }

        struct PageParts {
            std::shared_ptr<TodoHeader> header;
            std::shared_ptr<TodoComposer> composer;
            std::shared_ptr<TodoList> list;
        };

        template<typename Navigate>
        [[nodiscard]] auto make_page_parts(
            app::PageContext& context,
            const TodoPageParams& params,
            std::string authoring_label,
            std::string navigation_label,
            Navigate&& navigate
        ) -> PageParts {
            auto& store = context.store<TodoStore>();
            auto& status = context.scope().computed([&store] { return status_for(store); });
            auto& empty = context.scope().computed([&store] { return store.items.get().empty(); });
            auto& visit_text = context.scope().computed([&store, &params] {
                return parameter_summary(params, store.visits.get());
            });
            auto list = std::make_shared<TodoList>(context.graph(), empty, store, context.theme());
            list->on_toggle([&store](const std::uint64_t id) { store.toggle(id); });
            list->on_remove([&store](const std::uint64_t id) { store.remove(id); });
            auto composer = std::make_shared<TodoComposer>(
                context.theme(),
                [&store, weak = std::weak_ptr<TodoList>(list)](const std::string_view title) {
                    if (!store.add(title)) {
                        return false;
                    }
                    if (const auto current = weak.lock()) {
                        current->request_scroll_to_end();
                    }
                    return true;
                }
            );
            auto header = std::make_shared<TodoHeader>(
                context.graph(),
                status,
                visit_text,
                std::move(authoring_label),
                std::move(navigation_label),
                context.theme_manager(),
                context.dispatcher(),
                std::forward<Navigate>(navigate)
            );
            return {
                .header = std::move(header),
                .composer = std::move(composer),
                .list = std::move(list)
            };
        }
    } // namespace

    TodoStore::TodoStore(reactive::Graph& graph):
        items(
            graph,
            {
                {.id = 1, .title = "完成统一文本渲染管线", .completed = true},
                {.id = 2, .title = "验证中文输入、鼠标和窗口缩放"},
                {.id = 3, .title = "检查动态任务列表的滚动效果"},
            }
        ),
        visits(graph, 0) {}

    auto TodoStore::add(const std::string_view title) -> bool {
        if (!has_non_space(title)) {
            return false;
        }
        items.update([&](auto& values) {
            values.push_back(TodoItem {.id = next_id_++, .title = std::string(title)});
        });
        return true;
    }

    void TodoStore::toggle(const std::uint64_t id) {
        items.update([id](auto& values) {
            const auto item = std::ranges::find(values, id, &TodoItem::id);
            if (item != values.end()) {
                item->completed = !item->completed;
            }
        });
    }

    void TodoStore::remove(const std::uint64_t id) {
        items.update([id](auto& values) {
            std::erase_if(values, [id](const auto& item) { return item.id == id; });
        });
    }

    void TodoStore::bump_visit() {
        visits.set(visits.get() + 1);
    }

    TodoRow::TodoRow(reactive::Graph& graph, theme::NanTheme theme, Action toggle, Action remove):
        toggle_(std::move(toggle)),
        remove_(std::move(remove)) {
        label_ = widget::Label::create(graph, "", theme);
        label_->set_overflow(widget::primitives::TextOverflow::clip);

        toggle_button_ = widget::Button::create("完成", theme);
        toggle_button_->set_treatment(theme::ButtonTreatment::outlined);
        toggle_button_->set_on_click([this] {
            log::get("todo.row").debug("toggle task {}", id_);
            toggle_(id_);
        });

        remove_button_ = widget::Button::create("删除", theme);
        remove_button_->set_tone(theme::ButtonTone::secondary);
        remove_button_->set_treatment(theme::ButtonTreatment::outlined);
        remove_button_->set_on_click([this] { remove_(id_); });

        auto label_item = widget::FlexItem::create(
            scene::LayoutFlexPolicy {
                .grow = 1.0F,
                .shrink = 1.0F,
                .limits = {.min_width = 80.0F},
            }
        );
        label_item->set_child(label_);
        set_gap(8.0F)
            .set_cross_alignment(widget::LayoutAlignment::stretch)
            .add(label_item)
            .add(toggle_button_)
            .add(remove_button_);
    }

    void TodoRow::update(const TodoItem& item, const theme::NanTheme& theme) {
        id_ = item.id;
        completed_ = item.completed;
        label_->set_text((item.completed ? "[已完成] " : "[待办] ") + item.title);
        label_->set_color(
            item.completed ? theme.palette.on_surface_variant : theme.palette.on_surface
        );
        toggle_button_->set_text(item.completed ? "撤销" : "完成");
        set_semantics_override(
            semantics::Properties {
                .role = semantics::Role::list_item,
                .label = item.title,
                .value = item.completed ? "已完成" : "未完成",
            }
        );
    }

    auto TodoRow::toggle_button() -> widget::Button& {
        return *toggle_button_;
    }

    auto TodoRow::remove_button() -> widget::Button& {
        return *remove_button_;
    }

    void TodoRow::on_theme_changed(const theme::ThemeManager& manager) {
        Row::on_theme_changed(manager);
        label_->set_color(
            completed_ ? manager.theme().palette.on_surface_variant
                       : manager.theme().palette.on_surface
        );
    }

    TodoEmptyState::TodoEmptyState(reactive::Graph& graph, theme::NanTheme theme):
        Label(graph, "暂无任务", theme) {
        set_color(theme.palette.on_surface_variant);
    }

    void TodoEmptyState::on_theme_changed(const theme::ThemeManager& manager) {
        Label::on_theme_changed(manager);
        set_color(manager.theme().palette.on_surface_variant);
    }

    TodoHeader::TodoHeader(
        reactive::Graph& graph,
        reactive::Computed<std::string>& status,
        reactive::Computed<std::string>& visit_text,
        std::string authoring_label,
        std::string navigation_label,
        theme::ThemeManager& themes,
        app::UiDispatcher& dispatcher,
        std::function<void()> navigate
    ):
        themes_(&themes) {
        auto title = widget::Label::create(
            graph,
            "待办事项 / Todo · " + std::move(authoring_label),
            themes.theme()
        );
        title->set_font_size(24.0F);
        auto title_expanded = widget::Expanded::create();
        title_expanded->set_child(title);

        navigation_button_ = widget::Button::create(std::move(navigation_label), themes.theme());
        navigation_button_->set_tone(theme::ButtonTone::secondary);
        navigation_button_->set_on_click([dispatcher = &dispatcher,
                                          navigate = std::move(navigate)] {
            // 页面切换会转移节点所有权，必须离开当前输入/遍历回调后再执行。
            (void)dispatcher->post(navigate);
        });

        theme_button_ =
            widget::Button::create(preference_label(themes.preference()), themes.theme());
        theme_button_->set_treatment(theme::ButtonTreatment::outlined);
        theme_button_->set_on_click([this] {
            const auto next = next_preference(themes_->preference());
            themes_->set_preference(next);
            theme_button_->set_text(preference_label(next));
        });

        auto actions = widget::Row::create();
        actions->set_gap(8.0F)
            .set_cross_alignment(widget::LayoutAlignment::center)
            .add(navigation_button_)
            .add(theme_button_);
        auto title_row = widget::Row::create();
        title_row->set_gap(8.0F)
            .set_cross_alignment(widget::LayoutAlignment::center)
            .add(title_expanded)
            .add(actions);

        parameters_ = widget::Label::create(graph, "", themes.theme());
        parameters_->set_color(themes.theme().palette.on_surface_variant);
        parameters_->bind_text(visit_text);
        status_ = widget::Label::create(graph, "", themes.theme());
        status_->set_color(themes.theme().palette.on_surface_variant);
        status_->bind_text(status);

        set_gap(6.0F)
            .set_cross_alignment(widget::LayoutAlignment::stretch)
            .add(title_row)
            .add(parameters_)
            .add(status_);
    }

    auto TodoHeader::navigation_button() -> widget::Button& {
        return *navigation_button_;
    }

    auto TodoHeader::parameter_text() const -> std::string_view {
        return parameters_->text();
    }

    void TodoHeader::on_theme_changed(const theme::ThemeManager& manager) {
        Column::on_theme_changed(manager);
        parameters_->set_color(manager.theme().palette.on_surface_variant);
        status_->set_color(manager.theme().palette.on_surface_variant);
        theme_button_->set_text(preference_label(manager.preference()));
    }

    auto TodoHeader::next_preference(const theme::ThemePreference preference)
        -> theme::ThemePreference {
        switch (preference) {
            case theme::ThemePreference::system:
                return theme::ThemePreference::light;
            case theme::ThemePreference::light:
                return theme::ThemePreference::dark;
            case theme::ThemePreference::dark:
                return theme::ThemePreference::system;
        }
        return theme::ThemePreference::system;
    }

    auto TodoHeader::preference_label(const theme::ThemePreference preference) -> std::string {
        switch (preference) {
            case theme::ThemePreference::system:
                return "外观：跟随系统";
            case theme::ThemePreference::light:
                return "外观：浅色";
            case theme::ThemePreference::dark:
                return "外观：深色";
        }
        return "外观";
    }

    TodoComposer::TodoComposer(theme::NanTheme theme, Submit submit): submit_(std::move(submit)) {
        input_ = widget::TextField::create("", "添加一个任务", theme);
        input_->set_on_submit([this](const std::string_view value) { (void)submit_value(value); });
        add_button_ = widget::Button::create("添加", theme);
        add_button_->set_tone(theme::ButtonTone::secondary);
        add_button_->set_on_click([this] { (void)this->submit(); });

        auto input_expanded = widget::Expanded::create();
        input_expanded->set_child(input_);
        set_gap(10.0F)
            .set_cross_alignment(widget::LayoutAlignment::stretch)
            .add(input_expanded)
            .add(add_button_);
    }

    auto TodoComposer::input() -> widget::TextField& {
        return *input_;
    }

    auto TodoComposer::add_button() -> widget::Button& {
        return *add_button_;
    }

    auto TodoComposer::submit() -> bool {
        return submit_value(input_->value());
    }

    auto TodoComposer::submit_value(const std::string_view value) -> bool {
        if (!submit_(value)) {
            input_->set_invalid(true);
            return false;
        }
        input_->set_invalid(false);
        input_->set_value("");
        return true;
    }

    TodoTasks::TodoTasks(
        reactive::Graph& graph,
        reactive::Computed<bool>& empty,
        TodoStore& store,
        theme::NanTheme theme
    ):
        theme_(theme) {
        list_view_ = widget::ScrollView::create(widget::ScrollAxis::vertical);
        list_view_->set_wheel_step(36.0F);
        list_view_->set_semantics_override(
            semantics::Properties {.role = semantics::Role::list, .label = "待办事项"}
        );

        auto empty_region = widget::IfRegion<TodoEmptyState>::create(
            graph,
            [&graph, theme](reactive::ReactiveScope&) {
                return std::make_shared<TodoEmptyState>(graph, theme);
            }
        );
        empty_region->bind(empty);

        rows_ = Rows::create(
            graph,
            [](const TodoItem& item) { return item.id; },
            [this, &graph](reactive::ReactiveScope&, const TodoItem&) {
                return std::make_shared<TodoRow>(
                    graph,
                    theme_,
                    [this](const std::uint64_t id) { toggle_requested_.emit(id); },
                    [this](const std::uint64_t id) { remove_requested_.emit(id); }
                );
            },
            [this](TodoRow& row, const TodoItem& item) { row.update(item, theme_); }
        );
        rows_->set_gap(8.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
        rows_->set_model(store.items);

        auto content = widget::Column::create();
        content->set_cross_alignment(widget::LayoutAlignment::stretch).add(empty_region).add(rows_);
        list_view_->set_child(content);
        set_child(list_view_);
    }

    void TodoTasks::on_toggle(Action action) {
        toggle_subscription_ = toggle_requested_.subscribe(std::move(action));
    }

    void TodoTasks::on_remove(Action action) {
        remove_subscription_ = remove_requested_.subscribe(std::move(action));
    }

    void TodoTasks::request_scroll_to_end() {
        if (!is_inside_tree()) {
            return;
        }
        get_tree()->post_layout([weak = std::weak_ptr<widget::ScrollView>(list_view_)] {
            if (const auto list = weak.lock()) {
                list->set_scroll_offset(list->maximum_scroll_offset());
            }
        });
    }

    auto TodoTasks::row_count() const -> std::size_t {
        return rows_->child_count();
    }

    auto TodoTasks::row(const std::uint64_t id) -> TodoRow* {
        return rows_->node_for(id);
    }

    auto TodoTasks::scroll_view() -> widget::ScrollView& {
        return *list_view_;
    }

    void TodoTasks::on_theme_changed(const theme::ThemeManager& manager) {
        Expanded::on_theme_changed(manager);
        theme_ = manager.theme();
    }

    TodoWorkspace::TodoWorkspace(
        theme::ThemeManager& themes,
        std::shared_ptr<TodoHeader> header,
        std::shared_ptr<TodoComposer> composer,
        std::shared_ptr<TodoList> list
    ):
        header_(std::move(header)),
        composer_(std::move(composer)),
        list_(std::move(list)) {
        set_background(themes.theme().palette.background);
    }

    void TodoWorkspace::set_content(std::shared_ptr<scene::NanControl> content) {
        add_child(std::move(content));
    }

    auto TodoWorkspace::header() -> TodoHeader& {
        return *header_;
    }

    auto TodoWorkspace::composer() -> TodoComposer& {
        return *composer_;
    }

    auto TodoWorkspace::list() -> TodoList& {
        return *list_;
    }

    void TodoWorkspace::on_ready() {
        scene::NanControl::on_ready();
        get_tree()->set_focus(&composer_->input());
    }

    void TodoWorkspace::on_theme_changed(const theme::ThemeManager& manager) {
        scene::NanControl::on_theme_changed(manager);
        set_background(manager.theme().palette.background);
    }

    ImperativeTodoPage::ImperativeTodoPage(TodoPageParams params): NanPageT(std::move(params)) {}

    auto ImperativeTodoPage::route_key() const -> std::string_view {
        return "todo-imperative";
    }

    void ImperativeTodoPage::on_activate(app::PageContext& context) {
        context.store<TodoStore>().bump_visit();
    }

    auto ImperativeTodoPage::build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> {
        auto& router = context.router();
        auto parts = make_page_parts(context, params(), "命令式构建", "查看 DSL 版本", [&router] {
            router.push<DslTodoPage>(TodoPageParams {
                .source = "命令式页面",
            });
        });

        auto content = widget::Column::create();
        content->set_gap(10.0F)
            .set_cross_alignment(widget::LayoutAlignment::stretch)
            .add(parts.header)
            .add(parts.composer)
            .add(parts.list);
        auto padding = widget::Padding::create(foundation::NanInsets::all(16.0F));
        padding->set_child(content);
        workspace_ = std::make_shared<TodoWorkspace>(
            context.theme_manager(),
            parts.header,
            parts.composer,
            parts.list
        );
        workspace_->set_name("todo-imperative-root");
        workspace_->set_content(padding);
        return workspace_;
    }

    auto ImperativeTodoPage::workspace() const -> const std::shared_ptr<TodoWorkspace>& {
        return workspace_;
    }

    DslTodoPage::DslTodoPage(TodoPageParams params): NanPageT(std::move(params)) {}

    auto DslTodoPage::route_key() const -> std::string_view {
        return "todo-dsl";
    }

    void DslTodoPage::on_activate(app::PageContext& context) {
        context.store<TodoStore>().bump_visit();
    }

    auto DslTodoPage::build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> {
        using namespace widget::authoring;
        auto& router = context.router();
        auto parts = make_page_parts(context, params(), "DSL 构建", "返回命令式版本", [&router] {
            if (!router.pop_to("todo-imperative")) {
                router.push<ImperativeTodoPage>(TodoPageParams {
                    .source = "DSL 页面",
                });
            }
        });

        auto content =
            column()
                .configure([](widget::Column& c) {
                    c.set_gap(10.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
                })
                .children(parts.header, parts.composer, parts.list)
                .build();
        auto pad = padding(foundation::NanInsets::all(16.0F)).child(content).build();
        workspace_ =
            make<TodoWorkspace>(context.theme_manager(), parts.header, parts.composer, parts.list)
                .configure([&](TodoWorkspace& w) {
                    w.set_name("todo-dsl-root");
                    w.set_content(pad);
                })
                .build();
        return workspace_;
    }

    auto DslTodoPage::workspace() const -> const std::shared_ptr<TodoWorkspace>& {
        return workspace_;
    }
} // namespace nandina::examples::todo
