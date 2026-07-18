#include "app/nan_application.hpp"
#include "app/nan_page.hpp"
#include "app/nan_window.hpp"
#include "foundation/geometry.hpp"
#include "physics2d/physics_world2d.hpp"
#include "render/draw_context.hpp"
#include "scene/canvas_layer.hpp"
#include "scene/input_event.hpp"
#include "theme/theme.hpp"
#include "widget/button.hpp"
#include "widget/label.hpp"
#include "widget/layout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace nandina;

namespace
{
    constexpr float scene_width = 900.0F;
    constexpr float scene_height = 560.0F;

    [[nodiscard]] auto rgb(float r, float g, float b, float alpha = 1.0F)
        -> foundation::NanColor {
        return foundation::NanColor::from(foundation::NanLinearRgb {r, g, b, alpha});
    }

    class PhysicsVisual final: public scene::NanNode2D {
    public:
        enum class Kind { circle, box, sensor, backdrop };

        PhysicsVisual(Kind kind, foundation::NanSize size, foundation::NanColor color):
            kind_(kind), size_(size), color_(color) {}

        [[nodiscard]] auto contains_point(const foundation::NanPoint point) const -> bool override {
            if (kind_ == Kind::circle) {
                const auto radius = size_.get_width() * 0.5F;
                return point.length_squared() <= radius * radius;
            }
            return foundation::NanRect::from_xywh(
                -size_.get_width() * 0.5F,
                -size_.get_height() * 0.5F,
                size_.get_width(),
                size_.get_height()
            ).contains_point(point);
        }

        [[nodiscard]] auto global_bounds() const -> foundation::NanRect override {
            return render::world_bounds_from_local(
                global_transform(),
                foundation::NanRect::from_xywh(
                    -size_.get_width() * 0.5F,
                    -size_.get_height() * 0.5F,
                    size_.get_width(),
                    size_.get_height()
                )
            );
        }

        void set_active(const bool active) { active_ = active; }

        void on_draw(render::DrawContext& ctx) override {
            scene::NanNode2D::on_draw(ctx);
            const auto bounds = render::world_bounds_from_local(
                ctx.world_transform(),
                foundation::NanRect::from_xywh(
                    -size_.get_width() * 0.5F,
                    -size_.get_height() * 0.5F,
                    size_.get_width(),
                    size_.get_height()
                )
            );
            if (kind_ == Kind::circle) {
                ctx.device().draw_circle(
                    ctx.world_transform().transform_point(foundation::NanPoint::zero()),
                    size_.get_width() * 0.5F,
                    color_
                );
                ctx.device().draw_circle(
                    ctx.world_transform().transform_point(foundation::NanPoint::zero()),
                    3.0F,
                    rgb(0.96F, 0.98F, 1.0F)
                );
                return;
            }
            if (kind_ == Kind::sensor) {
                const auto fill = active_ ? rgb(0.12F, 0.64F, 0.48F, 0.35F)
                                          : rgb(0.10F, 0.38F, 0.34F, 0.20F);
                ctx.device().draw_rounded_rect(bounds, 8.0F, fill);
                ctx.device().draw_rect_outline(
                    bounds, active_ ? 3.0F : 1.5F,
                    active_ ? rgb(0.35F, 0.95F, 0.67F) : rgb(0.30F, 0.68F, 0.58F)
                );
                return;
            }
            ctx.device().draw_rect(bounds, color_);
        }

    private:
        Kind kind_;
        foundation::NanSize size_;
        foundation::NanColor color_;
        bool active_ = false;
    };

    class SpawnSurface final: public scene::NanNode2D {
    public:
        explicit SpawnSurface(std::function<void(foundation::NanPoint)> spawn):
            spawn_(std::move(spawn)) {}

        [[nodiscard]] auto contains_point(const foundation::NanPoint point) const -> bool override {
            return point.get_x() >= 0.0F && point.get_y() >= 0.0F
                && point.get_x() <= scene_width && point.get_y() <= scene_height;
        }

        [[nodiscard]] auto global_bounds() const -> foundation::NanRect override {
            return foundation::NanRect::from_xywh(0.0F, 0.0F, scene_width, scene_height);
        }

        auto on_input(scene::InputEvent& event) -> bool override {
            if (event.type() != scene::EventType::mouse_button) {
                return false;
            }
            auto& mouse = static_cast<scene::MouseButtonEvent&>(event);
            if (!mouse.is_pressed() || mouse.button() != scene::MouseButtonEvent::Button::left) {
                return false;
            }
            spawn_(to_local(mouse.screen_pos()));
            return true;
        }

    private:
        std::function<void(foundation::NanPoint)> spawn_;
    };

    class HudPadding final: public widget::Padding {
    public:
        using widget::Padding::Padding;

        [[nodiscard]] auto contains_point(foundation::NanPoint) const -> bool override {
            return false;
        }
    };

    class PhysicsDemoRoot final: public scene::LayerStack {
    public:
        PhysicsDemoRoot(reactive::Graph& graph, theme::NanTheme theme):
            graph_(&graph), theme_(theme) {
            world_layer_ = scene::CanvasLayer::create(scene::CanvasSpace::world, 0);
            hud_layer_ = scene::CanvasLayer::create(scene::CanvasSpace::screen, 10);
            hud_layer_->set_input_mode(scene::LayerInputMode::pass);
            add_layer(world_layer_);
            add_layer(hud_layer_);

            backdrop_ = std::make_shared<PhysicsVisual>(
                PhysicsVisual::Kind::backdrop,
                foundation::NanSize(scene_width, scene_height),
                rgb(0.035F, 0.055F, 0.075F)
            );
            backdrop_->set_position(foundation::NanPoint(scene_width * 0.5F, scene_height * 0.5F));
            backdrop_->set_z_index(-20);
            world_layer_->add_child(backdrop_);

            world_ = physics2d::PhysicsWorld2D::create(physics2d::PhysicsWorldConfig {
                .gravity = foundation::NanPoint(0.0F, 980.0F),
                .pixels_per_meter = 100.0F,
                .fixed_timestep = 1.0F / 60.0F,
                .substeps = 4,
            });
            world_layer_->add_child(world_);
            build_boundaries();
            build_sensor();

            spawn_surface_ = std::make_shared<SpawnSurface>([this](const auto point) {
                if (point.get_y() < scene_height - 80.0F) {
                    spawn_ball(point);
                }
            });
            spawn_surface_->set_z_index(50);
            world_layer_->add_child(spawn_surface_);

            build_hud();
            world_->set_event_handler([this](const physics2d::PhysicsTouchEvent& event) {
                if (event.kind == physics2d::PhysicsTouchKind::sensor_begin
                    && event.first == sensor_shape_) {
                    ++sensor_hits_;
                    sensor_visual_->set_active(true);
                    sensor_active_frames_ = 18;
                    update_status();
                }
            });

            spawn_ball(foundation::NanPoint(250.0F, 90.0F));
            spawn_ball(foundation::NanPoint(450.0F, 60.0F));
            spawn_ball(foundation::NanPoint(650.0F, 120.0F));
        }

        void on_process(float dt) override {
            scene::LayerStack::on_process(dt);
            if (sensor_active_frames_ > 0 && --sensor_active_frames_ == 0) {
                sensor_visual_->set_active(false);
            }
            std::erase_if(balls_, [this](const Ball& ball) {
                if (ball.visual->global_position().get_y() <= scene_height + 180.0F) {
                    return false;
                }
                world_->destroy_body(*ball.body);
                world_layer_->remove_and_delete(*ball.visual);
                return true;
            });
        }

    private:
        struct Ball {
            std::shared_ptr<PhysicsVisual> visual;
            std::shared_ptr<physics2d::PhysicsBody2D> body;
        };

        void build_boundaries() {
            add_static_box(
                foundation::NanPoint(scene_width * 0.5F, scene_height - 20.0F),
                foundation::NanSize(scene_width - 60.0F, 28.0F), 0.0F
            );
            add_static_box(
                foundation::NanPoint(170.0F, 360.0F),
                foundation::NanSize(260.0F, 18.0F), -0.16F
            );
            add_static_box(
                foundation::NanPoint(690.0F, 310.0F),
                foundation::NanSize(280.0F, 18.0F), 0.18F
            );
            add_static_box(
                foundation::NanPoint(20.0F, scene_height * 0.5F),
                foundation::NanSize(24.0F, scene_height), 0.0F
            );
            add_static_box(
                foundation::NanPoint(scene_width - 20.0F, scene_height * 0.5F),
                foundation::NanSize(24.0F, scene_height), 0.0F
            );
        }

        void add_static_box(
            const foundation::NanPoint position,
            const foundation::NanSize size,
            const float rotation
        ) {
            auto visual = std::make_shared<PhysicsVisual>(
                PhysicsVisual::Kind::box, size, rgb(0.18F, 0.24F, 0.30F)
            );
            visual->set_position(position);
            visual->set_rotation(rotation);
            world_layer_->add_child(visual);
            auto body = world_->create_body({
                .type = physics2d::PhysicsBodyType::static_body,
                .position = position,
                .rotation = rotation,
            });
            (void)body->create_box(size, {.friction = 0.75F, .restitution = 0.18F});
            static_bodies_.push_back(std::move(body));
        }

        void build_sensor() {
            const auto size = foundation::NanSize(180.0F, 52.0F);
            const auto position = foundation::NanPoint(scene_width * 0.5F, scene_height - 74.0F);
            sensor_visual_ = std::make_shared<PhysicsVisual>(
                PhysicsVisual::Kind::sensor, size, rgb(0.1F, 0.5F, 0.4F, 0.25F)
            );
            sensor_visual_->set_position(position);
            sensor_visual_->set_z_index(5);
            world_layer_->add_child(sensor_visual_);
            auto body = world_->create_body({
                .type = physics2d::PhysicsBodyType::static_body,
                .position = position,
            });
            sensor_shape_ = body->create_box(size, {
                .density = 0.0F,
                .filter = {.category = 2, .mask = 1},
                .sensor = true,
            });
            static_bodies_.push_back(std::move(body));
        }

        void spawn_ball(foundation::NanPoint position) {
            position = foundation::NanPoint(
                std::clamp(position.get_x(), 55.0F, scene_width - 55.0F),
                std::clamp(position.get_y(), 45.0F, scene_height - 150.0F)
            );
            const float radius = 14.0F + static_cast<float>(balls_.size() % 4) * 3.0F;
            const auto colors = std::array {
                rgb(0.18F, 0.62F, 0.92F), rgb(0.92F, 0.34F, 0.42F),
                rgb(0.96F, 0.68F, 0.20F), rgb(0.46F, 0.78F, 0.50F),
            };
            auto visual = std::make_shared<PhysicsVisual>(
                PhysicsVisual::Kind::circle,
                foundation::NanSize(radius * 2.0F, radius * 2.0F),
                colors[balls_.size() % colors.size()]
            );
            visual->set_position(position);
            visual->set_z_index(10);
            world_layer_->add_child(visual);
            auto body = world_->create_body({
                .type = physics2d::PhysicsBodyType::dynamic_body,
                .position = position,
                .rotation = static_cast<float>(balls_.size()) * 0.27F,
                .visual = visual,
            });
            (void)body->create_circle(radius, {
                .density = 1.0F,
                .friction = 0.45F,
                .restitution = 0.58F,
                .filter = {.category = 1, .mask = ~std::uint64_t {0}},
            });
            balls_.push_back({std::move(visual), std::move(body)});
            update_status();
        }

        void reset() {
            for (auto& ball: balls_) {
                world_->destroy_body(*ball.body);
                world_layer_->remove_and_delete(*ball.visual);
            }
            balls_.clear();
            sensor_hits_ = 0;
            spawn_ball(foundation::NanPoint(300.0F, 80.0F));
            spawn_ball(foundation::NanPoint(600.0F, 100.0F));
            update_status();
        }

        void build_hud() {
            title_ = widget::Label::create(*graph_, "Nandina Physics Canvas", theme_);
            title_->set_font_size(22.0F);
            status_ = widget::Label::create(*graph_, "", theme_);
            status_->set_color(theme_.palette.on_surface_variant);
            auto spawn = widget::Button::create("生成刚体", theme_);
            spawn->set_on_click([this] {
                const float x = 120.0F + static_cast<float>((balls_.size() * 137) % 650);
                spawn_ball(foundation::NanPoint(x, 55.0F));
            });
            auto reset_button = widget::Button::create("重置", theme_);
            reset_button->set_treatment(theme::ButtonTreatment::outlined);
            reset_button->set_on_click([this] { reset(); });
            auto buttons = widget::Row::create();
            buttons->set_gap(8.0F).add(spawn).add(reset_button);
            auto panel = widget::Column::create();
            panel->set_gap(6.0F).add(title_).add(status_).add(buttons);
            auto padding = std::make_shared<HudPadding>(foundation::NanInsets::all(14.0F));
            padding->set_child(panel);
            hud_layer_->set_layout_root(padding);
            update_status();
        }

        void update_status() {
            if (status_ != nullptr) {
                status_->set_text(
                    std::to_string(balls_.size()) + " bodies · "
                    + std::to_string(sensor_hits_) + " sensor hits · 点击场景生成"
                );
            }
        }

        reactive::Graph* graph_;
        theme::NanTheme theme_;
        std::shared_ptr<scene::CanvasLayer> world_layer_;
        std::shared_ptr<scene::CanvasLayer> hud_layer_;
        std::shared_ptr<physics2d::PhysicsWorld2D> world_;
        std::shared_ptr<PhysicsVisual> backdrop_;
        std::shared_ptr<PhysicsVisual> sensor_visual_;
        std::shared_ptr<SpawnSurface> spawn_surface_;
        std::shared_ptr<physics2d::PhysicsShape2D> sensor_shape_;
        std::vector<std::shared_ptr<physics2d::PhysicsBody2D>> static_bodies_;
        std::vector<Ball> balls_;
        std::shared_ptr<widget::Label> title_;
        std::shared_ptr<widget::Label> status_;
        std::size_t sensor_hits_ = 0;
        int sensor_active_frames_ = 0;
    };

    class PhysicsPage final: public app::NanPageT<app::NoParams> {
    public:
        [[nodiscard]] auto route_key() const -> std::string_view override { return "physics"; }
        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override {
            return std::make_shared<PhysicsDemoRoot>(context.graph(), context.theme());
        }
    };

    class PhysicsWindow final: public app::NanWindow {
    public:
        using NanWindow::NanWindow;
    protected:
        void on_setup() override { use_router().push<PhysicsPage>(); }
    };
} // namespace

auto main() -> int {
    app::NanApplication application(
        app::NanApplicationConfig::for_process("org.nandina.physics-demo")
    );
    application.set_theme(theme::default_theme());
    PhysicsWindow window {
        application,
        app::WindowConfig {
            .title = "Nandina - Physics Canvas",
            .width = static_cast<int>(scene_width),
            .height = static_cast<int>(scene_height),
            .target_fps = 120,
            .decorated = true,
            .resizable = false,
            .msaa = true,
            .vsync = false,
            .background = rgb(0.025F, 0.035F, 0.05F),
        },
    };
    return application.run(window);
}
