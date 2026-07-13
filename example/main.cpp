//
// Nandina — standard single-page application example.
//

#include "app/nan_application.hpp"
#include "app/nan_page.hpp"
#include "app/nan_window.hpp"
#include "foundation/geometry.hpp"
#include "reactive/computed.hpp"
#include "reactive/scope.hpp"
#include "reactive/signal.hpp"
#include "scene/control.hpp"
#include "text/font_face.hpp"
#include "text/glyph_atlas.hpp"
#include "text/glyph_run_renderer.hpp"
#include "text/harfbuzz_text_backend.hpp"
#include "theme/theme.hpp"
#include "widget/button.hpp"
#include "widget/label.hpp"
#include "widget/layout.hpp"
#include "widget/scroll_view.hpp"
#include "widget/text_field.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace nandina;

struct TodoPageParams {
    widget::primitives::TextPipeline text_pipeline;
};

struct TodoItem {
    std::uint64_t id = 0;
    std::string title;
    bool completed = false;

    auto operator==(const TodoItem&) const -> bool = default;
};

class TodoPageRoot final: public scene::NanControl {
public:
    TodoPageRoot(
        reactive::Graph& graph,
        reactive::Signal<std::vector<TodoItem>>& todos,
        theme::NanTheme theme,
        widget::primitives::TextPipeline pipeline
    ):
        graph_(&graph), todos_(&todos), theme_(theme), pipeline_(pipeline) {
        set_background(theme_.palette.surface);

        title_ = widget::Label::create(graph, "Todo workspace", theme_);
        title_->set_font_size(24.0F);
        title_->set_text_pipeline(pipeline_);

        status_ = widget::Label::create(graph, "", theme_);
        status_->set_color(theme_.palette.on_surface_variant);
        status_->set_text_pipeline(pipeline_);

        input_ = std::make_shared<widget::TextField>("", "Add a task", theme_);
        input_->set_text_pipeline(pipeline_);
        input_->set_on_submit([this](std::string_view value) { submit(value); });

        add_button_ = widget::Button::create("Add", theme_);
        add_button_->set_tone(theme::ButtonTone::secondary);
        add_button_->set_text_pipeline(pipeline_);
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
        list_view_->set_child(widget::Column::create());

        const auto list_expanded = widget::Expanded::create();
        list_expanded->set_child(list_view_);
        const auto content = widget::Column::create();
        content->set_gap(10.0F)
            .set_cross_alignment(widget::LayoutAlignment::stretch)
            .add(title_)
            .add(status_)
            .add(input_row)
            .add(list_expanded);

        const auto padding = widget::Padding::create(foundation::NanInsets::all(16.0F));
        padding->set_child(content);
        add_child(padding);
    }

    void stage_items(std::vector<TodoItem> items) {
        pending_items_ = std::move(items);
        rebuild_pending_ = true;
    }

    [[nodiscard]] auto input_field() const -> widget::TextField* { return input_.get(); }
    [[nodiscard]] auto list_view() const -> widget::ScrollView* { return list_view_.get(); }
    [[nodiscard]] auto rendered_item_count() const -> std::size_t { return rendered_item_count_; }

    void on_process(float dt) override {
        scene::NanControl::on_process(dt);
        if (rebuild_pending_) {
            rebuild_pending_ = false;
            rebuild_list();
        } else {
            apply_deferred_scroll();
        }
    }

    void on_ready() override {
        scene::NanControl::on_ready();
        get_tree()->set_focus(input_.get());
    }

    void apply_deferred_scroll() {
        if (!scroll_after_layout_pending_) { return; }
        scroll_after_layout_pending_ = false;
        list_view_->set_scroll_offset(list_view_->maximum_scroll_offset());
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
        scroll_to_end_pending_ = true;
    }

    void toggle(std::uint64_t id) {
        todos_->update([id](auto& items) {
            const auto item = std::ranges::find(items, id, &TodoItem::id);
            if (item != items.end()) { item->completed = !item->completed; }
        });
    }

    void remove(std::uint64_t id) {
        todos_->update([id](auto& items) {
            std::erase_if(items, [id](const auto& item) { return item.id == id; });
        });
    }

    void rebuild_list() {
        auto column = widget::Column::create();
        column->set_gap(8.0F).set_cross_alignment(widget::LayoutAlignment::stretch);
        rendered_item_count_ = pending_items_.size();
        const auto completed = static_cast<std::size_t>(std::ranges::count(
            pending_items_, true, &TodoItem::completed
        ));
        status_->set_text(
            std::to_string(pending_items_.size()) + " tasks, " + std::to_string(completed)
            + " completed"
        );

        if (pending_items_.empty()) {
            auto empty = widget::Label::create(*graph_, "No tasks yet", theme_);
            empty->set_color(theme_.palette.on_surface_variant);
            empty->set_text_pipeline(pipeline_);
            column->add(empty);
        }
        for (const auto& item: pending_items_) {
            auto label = widget::Label::create(
                *graph_, (item.completed ? "[done] " : "[todo] ") + item.title, theme_
            );
            label->set_color(item.completed
                ? theme_.palette.on_surface_variant
                : theme_.palette.on_surface);
            label->set_overflow(widget::primitives::TextOverflow::clip);
            label->set_text_pipeline(pipeline_);

            auto toggle_button = widget::Button::create(item.completed ? "Undo" : "Done", theme_);
            toggle_button->set_treatment(theme::ButtonTreatment::outlined);
            toggle_button->set_text_pipeline(pipeline_);
            toggle_button->set_on_click([this, id = item.id] { toggle(id); });

            auto remove_button = widget::Button::create("Remove", theme_);
            remove_button->set_tone(theme::ButtonTone::secondary);
            remove_button->set_treatment(theme::ButtonTreatment::outlined);
            remove_button->set_text_pipeline(pipeline_);
            remove_button->set_on_click([this, id = item.id] { remove(id); });

            auto label_item = widget::FlexItem::create(scene::LayoutFlexPolicy {
                .grow = 1.0F,
                .shrink = 1.0F,
                .limits = {.min_width = 80.0F},
            });
            label_item->set_child(label);
            auto row = widget::Row::create();
            row->set_gap(8.0F)
                .set_cross_alignment(widget::LayoutAlignment::stretch)
                .add(label_item)
                .add(toggle_button)
                .add(remove_button);
            column->add(row);
        }
        list_view_->set_child(column);
        mark_layout_dirty();
        if (scroll_to_end_pending_) {
            scroll_to_end_pending_ = false;
            scroll_after_layout_pending_ = true;
        }
    }

    reactive::Graph* graph_;
    reactive::Signal<std::vector<TodoItem>>* todos_;
    theme::NanTheme theme_;
    widget::primitives::TextPipeline pipeline_;
    std::shared_ptr<widget::Label> title_;
    std::shared_ptr<widget::Label> status_;
    std::shared_ptr<widget::TextField> input_;
    std::shared_ptr<widget::Button> add_button_;
    std::shared_ptr<widget::ScrollView> list_view_;
    std::vector<TodoItem> pending_items_;
    std::uint64_t next_id_ = 4;
    std::size_t rendered_item_count_ = 0;
    bool rebuild_pending_ = false;
    bool scroll_to_end_pending_ = false;
    bool scroll_after_layout_pending_ = false;
};

class TodoPage final: public app::NanPageT<TodoPageParams> {
public:
    explicit TodoPage(TodoPageParams params): NanPageT(std::move(params)) {}

    [[nodiscard]] auto route_key() const -> std::string_view override {
        return "todo";
    }

    [[nodiscard]] auto build(app::PageContext& context)
        -> std::shared_ptr<scene::NanNode2D> override {
        auto& todos = context.scope().signal_value(std::vector<TodoItem> {
            {.id = 1, .title = "Build the unified text pipeline", .completed = true},
            {.id = 2, .title = "Exercise mouse, keyboard, resize, and bidi editing"},
            {.id = 3, .title = "Validate scrolling with a dynamic task list"},
        });
        auto root = std::make_shared<TodoPageRoot>(
            context.graph(), todos, context.theme(), params().text_pipeline
        );
        context.scope().effect([weak = std::weak_ptr<TodoPageRoot>(root), &todos] {
            if (auto current = weak.lock()) { current->stage_items(todos.get()); }
        });
        return root;
    }
};

class TodoWindow final: public app::NanWindow {
public:
    using NanWindow::NanWindow;

protected:
    void on_setup() override {
        auto pipeline = widget::primitives::TextPipeline {};
#ifdef NANDINA_EXAMPLE_FONT_PATH
        auto* device = render_device();
        if (device == nullptr) {
            throw std::runtime_error("TodoWindow requires an active render device");
        }

        font_face_ = std::make_shared<text::FreeTypeFontFace>(NANDINA_EXAMPLE_FONT_PATH);
        text_backend_ = std::make_unique<text::HarfBuzzTextLayoutBackend>(font_face_);
        glyph_atlas_ = std::make_unique<text::GlyphAtlas>(font_face_, 1024, 1024);
        atlas_texture_ = std::make_unique<text::GlyphAtlasTexture>(*device, *glyph_atlas_);
        text_renderer_ = std::make_unique<text::GlyphRunRenderer>(*glyph_atlas_, *atlas_texture_);
        pipeline = {
            .backend = text_backend_.get(),
            .renderer = text_renderer_.get(),
        };
#endif

        use_router().push<TodoPage>(TodoPageParams {
            .text_pipeline = pipeline,
        });
    }

    void on_teardown() override {
        if (auto* active_router = router(); active_router != nullptr) {
            active_router->clear();
        }
        text_renderer_.reset();
        atlas_texture_.reset();
        glyph_atlas_.reset();
        text_backend_.reset();
        font_face_.reset();
    }

private:
    std::shared_ptr<text::FreeTypeFontFace> font_face_;
    std::unique_ptr<text::HarfBuzzTextLayoutBackend> text_backend_;
    std::unique_ptr<text::GlyphAtlas> glyph_atlas_;
    std::unique_ptr<text::GlyphAtlasTexture> atlas_texture_;
    std::unique_ptr<text::GlyphRunRenderer> text_renderer_;
};

auto main() -> int {
    app::NanApplication application;
    application.set_theme(theme::default_theme());

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
