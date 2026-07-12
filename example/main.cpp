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

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace nandina;

struct TodoPageParams {
    widget::primitives::ITextLayoutBackend* backend = nullptr;
    widget::primitives::ITextLayoutRenderer* renderer = nullptr;
};

class TodoPage final: public app::NanPageT<TodoPageParams> {
public:
    explicit TodoPage(TodoPageParams params): NanPageT(std::move(params)) {}

    [[nodiscard]] auto route_key() const -> std::string_view override {
        return "todo";
    }

    [[nodiscard]] auto build(app::PageContext& context)
        -> std::shared_ptr<scene::NanNode2D> override {
        auto& graph = context.graph();
        const auto& app_theme = context.theme();

        auto root = std::make_shared<scene::NanControl>(foundation::NanSize(640.0F, 360.0F));
        root->set_background(app_theme.palette.surface);

        const auto todo_label = widget::Label::create(graph, "Current todo", app_theme);
        todo_label->set_font_size(22);
        todo_label->set_overflow(widget::primitives::TextOverflow::clip);

        const auto todo_item_1 =
            widget::Label::create(graph, "todo item 1: Build the text pipeline", app_theme);
        todo_item_1->set_font_size(18);

        const auto todo_item_2 = widget::Label::create(
            graph,
            "todo item 2: Verify ligatures  ->  =>  !=  ===",
            app_theme
        );
        todo_item_2->set_font_size(18);

        const auto todo_item_3 = widget::Label::create(
            graph,
            "todo item 3: if (ready && tested) return success;",
            app_theme
        );
        todo_item_3->set_font_size(18);

        const auto apply_text_pipeline = [this](const std::shared_ptr<widget::Label>& label) {
            if (params().backend != nullptr) {
                label->set_layout_backend(*params().backend);
            }
            label->set_layout_renderer(params().renderer);
        };
        apply_text_pipeline(todo_label);
        apply_text_pipeline(todo_item_1);
        apply_text_pipeline(todo_item_2);
        apply_text_pipeline(todo_item_3);

        const auto todo_item_group = widget::Column::create();
        todo_item_group->set_gap(5).add(todo_item_1).add(todo_item_2).add(todo_item_3);

        const auto remove_item_button = widget::Button::create("remove", app_theme);
        remove_item_button->set_tone(theme::ButtonTone::secondary);
        remove_item_button->set_treatment(theme::ButtonTreatment::outlined);

        const auto add_item_button = widget::Button::create("add", app_theme);
        add_item_button->set_tone(theme::ButtonTone::secondary);
        add_item_button->set_treatment(theme::ButtonTreatment::filled);

        const auto button_group = widget::Row::create();
        button_group->set_cross_alignment(widget::LayoutAlignment::space_between);
        button_group->set_main_alignment(widget::LayoutAlignment::space_between)
            .set_gap(12)
            .add(remove_item_button)
            .add(add_item_button);

        const auto actions = widget::Column::create();
        actions->set_gap(15.0F)
            .set_main_alignment(widget::LayoutAlignment::start)
            .set_cross_alignment(widget::LayoutAlignment::start)
            .add(todo_label)
            .add(todo_item_group)
            .add(widget::Padding::create(foundation::NanInsets::only_top(20)))
            .add(button_group);

        root->add_child(actions);
        return root;
    }
};

class TodoWindow final: public app::NanWindow {
public:
    using NanWindow::NanWindow;

protected:
    void on_setup() override {
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
#endif

        use_router().push<TodoPage>(TodoPageParams {
            .backend = text_backend_.get(),
            .renderer = text_renderer_.get(),
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
