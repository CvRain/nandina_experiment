//
// Nandina — standard single-page application example.
//

#include "app/nan_application.hpp"
#include "app/nan_page.hpp"
#include "app/nan_window.hpp"
#include "foundation/geometry.hpp"
#include "foundation/nan_logger.hpp"
#include "reactive/computed.hpp"
#include "reactive/scope.hpp"
#include "reactive/signal.hpp"
#include "scene/control.hpp"
#include "semantics/semantics.hpp"
#include "theme/nan_style.hpp"
#include "theme/theme.hpp"
#include "theme/theme_manager.hpp"
#include "widget/button.hpp"
#include "widget/declarative.hpp"
#include "widget/label.hpp"
#include "widget/layout.hpp"
#include "widget/scroll_view.hpp"
#include "widget/text_field.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace nandina;

struct TodoItem {
    std::uint64_t id = 0;
    std::string title;
    bool completed = false;

    auto operator==(const TodoItem&) const -> bool = default;
};

class TodoRow final: public widget::Row {
public:
    using Action = std::function<void(std::uint64_t)>;

    TodoRow(reactive::Graph& graph, theme::NanTheme theme, Action toggle, Action remove):
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

        auto remove_button = widget::Button::create("删除", theme);
        remove_button->set_tone(theme::ButtonTone::secondary);
        remove_button->set_treatment(theme::ButtonTreatment::outlined);
        remove_button->set_on_click([this] { remove_(id_); });

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
            .add(remove_button);
    }

    void update(const TodoItem& item, const theme::NanTheme& theme) {
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

    void on_theme_changed(const theme::ThemeManager& manager) override {
        Row::on_theme_changed(manager);
        label_->set_color(
            completed_ ? manager.theme().palette.on_surface_variant
                       : manager.theme().palette.on_surface
        );
    }

private:
    Action toggle_;
    Action remove_;
    std::shared_ptr<widget::Label> label_;
    std::shared_ptr<widget::Button> toggle_button_;
    std::uint64_t id_ = 0;
    bool completed_ = false;
};

class TodoPageRoot final: public scene::NanControl {
public:
    TodoPageRoot(
        reactive::Graph& graph,
        reactive::Signal<std::vector<TodoItem>>& todos,
        reactive::Computed<std::string>& status,
        reactive::Computed<bool>& empty,
        theme::ThemeManager& themes
    ):
        graph_(&graph),
        todos_(&todos),
        themes_(&themes),
        theme_(themes.theme()) {
        set_background(theme_.palette.surface);

        title_ = widget::Label::create(graph, "待办事项 / Todo workspace", theme_);
        title_->set_font_size(24.0F);

        theme_button_ = widget::Button::create("切换浅色主题", theme_);
        theme_button_->set_treatment(theme::ButtonTreatment::outlined);
        theme_button_->set_on_click([this] {
            const auto target = themes_->active_name() == "dark" ? "light" : "dark";
            (void)themes_->activate(target);
        });

        const auto title_expanded = widget::Expanded::create();
        title_expanded->set_child(title_);
        const auto title_row = widget::Row::create();
        title_row->set_cross_alignment(widget::LayoutAlignment::center)
            .add(title_expanded)
            .add(theme_button_);

        status_ = widget::Label::create(graph, "", theme_);
        status_->set_color(theme_.palette.on_surface_variant);
        status_->bind_text(status);

        input_ = std::make_shared<widget::TextField>("", "添加一个任务", theme_);
        input_->set_on_submit([this](std::string_view value) { submit(value); });

        add_button_ = widget::Button::create("添加", theme_);
        add_button_->set_tone(theme::ButtonTone::secondary);
        add_button_->set_on_click([this] { submit(input_->value()); });

        const auto input_expanded = widget::Expanded::create();
        input_expanded->set_child(input_);
        const auto input_row = widget::Row::create();
        input_row->set_gap(10.0F)
            .set_cross_alignment(widget::LayoutAlignment::stretch)
            .add(input_expanded)
            .add(add_button_);

        list_view_ = widget::ScrollView::create(widget::ScrollAxis::vertical);
        list_view_->set_wheel_step(36.0F);
        list_view_->set_semantics_override(
            semantics::Properties {
                .role = semantics::Role::list,
                .label = "待办事项",
            }
        );

        auto empty_region =
            widget::IfRegion<widget::Label>::create(graph, [this](reactive::ReactiveScope&) {
                auto label = widget::Label::create(*graph_, "暂无任务", theme_);
                label->set_color(theme_.palette.on_surface_variant);
                return label;
            });
        empty_region->bind(empty);

        auto rows = widget::ForEach<TodoItem, std::uint64_t, TodoRow>::create(
            graph,
            [](const TodoItem& item) { return item.id; },
            [this](reactive::ReactiveScope&, const TodoItem&) {
                return std::make_shared<TodoRow>(
                    *graph_,
                    theme_,
                    [this](const std::uint64_t id) { toggle(id); },
                    [this](const std::uint64_t id) { remove(id); }
                );
            },
            [this](TodoRow& row, const TodoItem& item) { row.update(item, theme_); }
        );
        rows->set_gap(8.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
        rows->bind(todos);

        auto list_content = widget::Column::create();
        list_content->set_cross_alignment(widget::LayoutAlignment::stretch)
            .add(empty_region)
            .add(rows);
        list_view_->set_child(list_content);

        const auto list_expanded = widget::Expanded::create();
        list_expanded->set_child(list_view_);
        const auto content = widget::Column::create();
        content->set_gap(10.0F)
            .set_cross_alignment(widget::LayoutAlignment::stretch)
            .add(title_row)
            .add(status_)
            .add(input_row)
            .add(list_expanded);

        const auto padding = widget::Padding::create(foundation::NanInsets::all(16.0F));
        padding->set_child(content);
        add_child(padding);
    }

    void on_ready() override {
        scene::NanControl::on_ready();
        get_tree()->set_focus(input_.get());
    }

    void on_theme_changed(const theme::ThemeManager& manager) override {
        scene::NanControl::on_theme_changed(manager);
        theme_ = manager.theme();
        set_background(theme_.palette.surface);
        status_->set_color(theme_.palette.on_surface_variant);
        theme_button_->set_text(
            manager.active_name() == "dark" ? "切换浅色主题" : "切换深色主题"
        );
    }

private:
    [[nodiscard]] static auto has_non_space(std::string_view text) -> bool {
        return std::ranges::any_of(text, [](const char ch) {
            return std::isspace(static_cast<unsigned char>(ch)) == 0;
        });
    }

    void submit(std::string_view value) {
        const std::string title(value);
        if (!has_non_space(title)) {
            input_->set_invalid(true);
            return;
        }
        input_->set_invalid(false);
        todos_->update([&](auto& items) {
            items.push_back(TodoItem {.id = next_id_++, .title = title});
        });
        input_->set_value("");
        get_tree()->post_layout([weak = std::weak_ptr<widget::ScrollView>(list_view_)] {
            if (auto list = weak.lock()) {
                list->set_scroll_offset(list->maximum_scroll_offset());
            }
        });
    }

    void toggle(std::uint64_t id) {
        todos_->update([id](auto& items) {
            const auto item = std::ranges::find(items, id, &TodoItem::id);
            if (item != items.end()) {
                item->completed = !item->completed;
            }
        });
    }

    void remove(std::uint64_t id) {
        todos_->update([id](auto& items) {
            std::erase_if(items, [id](const auto& item) { return item.id == id; });
        });
    }

    reactive::Graph* graph_;
    reactive::Signal<std::vector<TodoItem>>* todos_;
    theme::ThemeManager* themes_;
    theme::NanTheme theme_;
    std::shared_ptr<widget::Label> title_;
    std::shared_ptr<widget::Label> status_;
    std::shared_ptr<widget::TextField> input_;
    std::shared_ptr<widget::Button> add_button_;
    std::shared_ptr<widget::Button> theme_button_;
    std::shared_ptr<widget::ScrollView> list_view_;
    std::uint64_t next_id_ = 4;
};

class TodoPage final: public app::NanPageT<app::NoParams> {
public:
    TodoPage() = default;

    [[nodiscard]] auto route_key() const -> std::string_view override {
        return "todo";
    }

    [[nodiscard]] auto build(app::PageContext& context)
        -> std::shared_ptr<scene::NanNode2D> override {
        auto& todos = context.scope().signal_value(
            std::vector<TodoItem> {
                {.id = 1, .title = "完成统一文本渲染管线", .completed = true},
                {.id = 2, .title = "验证中文输入、鼠标和窗口缩放"},
                {.id = 3, .title = "检查动态任务列表的滚动效果"},
            }
        );
        auto& status = context.scope().computed([&todos] {
            const auto& items = todos.get();
            const auto completed =
                static_cast<std::size_t>(std::ranges::count(items, true, &TodoItem::completed));
            return std::to_string(items.size()) + " tasks, " + std::to_string(completed)
                + " completed / 已完成";
        });
        auto& empty = context.scope().computed([&todos] { return todos.get().empty(); });
        auto root = std::make_shared<TodoPageRoot>(
            context.graph(), todos, status, empty, context.theme_manager()
        );
        return root;
    }
};

class TodoWindow final: public app::NanWindow {
public:
    using NanWindow::NanWindow;

protected:
    void on_setup() override {
        use_router().push<TodoPage>();
    }
};

auto main() -> int {
    app::NanApplication application(app::NanApplicationConfig::for_process("org.nandina.todo"));
    auto chinese_fallback = text::register_optional_font_fallback(
        application.font_families(),
        application.resources(),
        resource::ResourceKey("families/zh-cn"),
        resource::ResourceKey("fonts/fallback/zh-cn")
    );
    if (!chinese_fallback) {
        return 2;
    }
    auto dark_theme = theme::default_theme();
    auto light_theme = theme::default_theme();
    light_theme.palette.primary = theme::nan_color(0.56F, 0.18F, 250.0F);
    light_theme.palette.on_primary = theme::nan_color(0.98F, 0.01F, 250.0F);
    light_theme.palette.secondary = theme::nan_color(0.62F, 0.13F, 150.0F);
    light_theme.palette.on_secondary = theme::nan_color(0.16F, 0.02F, 150.0F);
    light_theme.palette.surface = theme::nan_color(0.97F, 0.01F, 270.0F);
    light_theme.palette.on_surface = theme::nan_color(0.22F, 0.02F, 275.0F);
    light_theme.palette.surface_variant = theme::nan_color(0.91F, 0.02F, 275.0F);
    light_theme.palette.on_surface_variant = theme::nan_color(0.43F, 0.03F, 275.0F);
    light_theme.palette.outline = theme::nan_color(0.58F, 0.02F, 275.0F);
    light_theme.palette.outline_variant = theme::nan_color(0.78F, 0.02F, 275.0F);

    auto style = std::make_shared<theme::NanStyle>();
    theme::ButtonStyleRule buttons;
    buttons.radius = theme::ThemeScalar::literal(7.0F);
    style->add_button_rule(std::move(buttons));
    theme::TextFieldStyleRule focused_field;
    focused_field.state = theme::TextFieldVisualState::focused;
    focused_field.border_color = theme::ThemeColor::token(theme::ColorToken::primary);
    focused_field.focus_ring_color = theme::ThemeColor::token(theme::ColorToken::primary);
    style->add_text_field_rule(std::move(focused_field));

    auto& themes = application.theme_manager();
    (void)themes.register_theme("dark", dark_theme);
    (void)themes.register_theme("light", light_theme);
    themes.set_style(std::move(style));
    (void)themes.activate("dark");

    TodoWindow window {
        application,
        app::WindowConfig {
            .title = "Nandina - Todo Text Demo",
            .width = 640,
            .height = 360,
            .target_fps = 120,
            .decorated = true,
            .resizable = true,
            .msaa = true,
            .vsync = false,
            .background = application.theme().palette.surface,
        },
    };

    return application.run(window);
}
