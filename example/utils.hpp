#ifndef NANOSUIT_UTILS_HPP
#define NANOSUIT_UTILS_HPP

#include <spdlog/spdlog.h>

#include <cmath>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include "foundation/geometry.hpp"
#include "foundation/nandina_color.hpp"
#include "scene/input_event.hpp"
#include "scene/node2d.hpp"
#include "scene/scene_tree.hpp"

using namespace nandina;

[[nodiscard]] auto to_raylib(const foundation::NanColor& color) -> Color {
    const auto rgba = color.to<foundation::NanHexRgb>();
    return {
        static_cast<unsigned char>(rgba.red),
        static_cast<unsigned char>(rgba.green),
        static_cast<unsigned char>(rgba.blue),
        static_cast<unsigned char>(rgba.alpha)
    };
}

[[nodiscard]] auto lighten(Color color, int amount) -> Color {
    auto bump = [amount](unsigned char channel) -> unsigned char {
        const int value = static_cast<int>(channel) + amount;
        return static_cast<unsigned char>(value > 255 ? 255 : value);
    };

    return {bump(color.r), bump(color.g), bump(color.b), color.a};
}

[[nodiscard]] auto lerp_color(Color a, Color b, float t) -> Color {
    const auto lerp = [t](unsigned char from, unsigned char to) -> unsigned char {
        return static_cast<unsigned char>(from + (to - from) * t);
    };

    return {lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a)};
}

class GroupNode final: public scene::NanNode2D {
public:
    [[nodiscard]] auto contains_point(foundation::NanPoint /*local_point*/) const -> bool override {
        return false;
    }
};

class CardNode final: public scene::NanNode2D {
public:
    CardNode(
        foundation::NanSize size,
        Color base_color,
        std::string label,
        bool interactive = true
    ):
        size_(size),
        base_color_(base_color),
        hover_color_(lighten(base_color, 22)),
        toggled_color_(lighten(base_color, 56)),
        hover_toggled_color_(lighten(base_color, 80)),
        label_(std::move(label)),
        interactive_(interactive) {}

    [[nodiscard]] auto contains_point(foundation::NanPoint local_point) const -> bool override {
        const auto hw = size_.get_width() / 2.0F;
        const auto hh = size_.get_height() / 2.0F;
        return local_point.get_x() >= -hw && local_point.get_x() <= hw && local_point.get_y() >= -hh
            && local_point.get_y() <= hh;
    }

    [[nodiscard]] auto global_bounds() const -> foundation::NanRect override {
        const auto hw = size_.get_width() / 2.0F;
        const auto hh = size_.get_height() / 2.0F;

        const auto top_left = to_global(foundation::NanPoint(-hw, -hh));
        const auto top_right = to_global(foundation::NanPoint(hw, -hh));
        const auto bottom_left = to_global(foundation::NanPoint(-hw, hh));
        const auto bottom_right = to_global(foundation::NanPoint(hw, hh));

        auto bounds = foundation::NanRect::from_points(top_left, top_right);
        bounds = bounds.united(foundation::NanRect::from_points(top_left, bottom_left));
        bounds = bounds.united(foundation::NanRect::from_points(top_left, bottom_right));
        return bounds;
    }

    auto on_input(scene::InputEvent& event) -> bool override {
            if (!interactive_) {
                return false;
            }

            if (dynamic_cast<const scene::MouseEnterEvent*>(&event)) {
                hovered_ = true;
                return false;
            }

            if (dynamic_cast<const scene::MouseLeaveEvent*>(&event)) {
                hovered_ = false;
                return false;
            }

            if (dynamic_cast<const scene::FocusEnterEvent*>(&event)) {
                focused_ = true;
                return false;
            }

            if (dynamic_cast<const scene::FocusLeaveEvent*>(&event)) {
                focused_ = false;
                return false;
            }

            if (const auto* button = dynamic_cast<const scene::MouseButtonEvent*>(&event)) {
                    if (button->button() == scene::MouseButtonEvent::Button::left
                        && button->is_pressed())
                    {
                        toggled_ = !toggled_;
                        event.accept();
                        spdlog::info("click: {}", name());
                        return true;
                    }
            }

            if (const auto* key = dynamic_cast<const scene::KeyEvent*>(&event)) {
                    if (key->is_pressed()
                        && (key->keycode() == KEY_ENTER || key->keycode() == KEY_SPACE))
                    {
                        toggled_ = !toggled_;
                        event.accept();
                        spdlog::info("key activate: {}", name());
                        return true;
                    }
            }

        return false;
    }

    void on_draw() override {
        const auto gt = global_transform();
        const auto pos = gt.position();
        const auto w = size_.get_width() * gt.scale_x();
        const auto h = size_.get_height() * gt.scale_y();
        const auto angle = gt.rotation() * 180.0F / std::numbers::pi_v<float>;

        const auto fill = current_fill_color();
        const auto shadow = lerp_color(fill, BLACK, 0.28F);
        const auto text = hovered_ ? BLACK : Color {32, 34, 42, 255};

        DrawRectanglePro(
            {pos.get_x() - w / 2.0F + 4.0F, pos.get_y() - h / 2.0F + 5.0F, w, h},
            {w / 2.0F, h / 2.0F},
            angle,
            Color {shadow.r, shadow.g, shadow.b, 42}
        );
        DrawRectanglePro(
            {pos.get_x() - w / 2.0F, pos.get_y() - h / 2.0F, w, h},
            {w / 2.0F, h / 2.0F},
            angle,
            fill
        );

            if (focused_) {
                DrawRectangleLinesEx(
                    {pos.get_x() - w / 2.0F - 3.0F,
                     pos.get_y() - h / 2.0F - 3.0F,
                     w + 6.0F,
                     h + 6.0F},
                    2.0F,
                    Color {26, 76, 160, 255}
                );
            }

            if (!label_.empty()) {
                const auto font_size = static_cast<int>(h < 28.0F ? 11.0F : 12.0F);
                const auto tw = static_cast<float>(MeasureText(label_.c_str(), font_size));
                DrawText(
                    label_.c_str(),
                    static_cast<int>(pos.get_x() - tw / 2.0F),
                    static_cast<int>(pos.get_y() - font_size / 2.0F),
                    font_size,
                    text
                );
            }
    }

private:
    [[nodiscard]] auto current_fill_color() const -> Color {
            if (toggled_ && hovered_) {
                return hover_toggled_color_;
            }
            if (toggled_) {
                return toggled_color_;
            }
            if (hovered_) {
                return hover_color_;
            }
        return base_color_;
    }

    foundation::NanSize size_;
    Color base_color_;
    Color hover_color_;
    Color toggled_color_;
    Color hover_toggled_color_;
    std::string label_;
    bool interactive_ = true;
    bool hovered_ = false;
    bool focused_ = false;
    bool toggled_ = false;
};

struct DeepChain {
    std::unique_ptr<CardNode> root;
    scene::NanNode2D* leaf = nullptr;
};

struct TransformRig {
    std::unique_ptr<GroupNode> root;
    GroupNode* spinner = nullptr;
    CardNode* shell = nullptr;
    scene::NanNode2D* button = nullptr;
    GroupNode* root_node = nullptr;
};

struct ChainRig {
    std::unique_ptr<GroupNode> root;
    scene::NanNode2D* leaf_a = nullptr;
    scene::NanNode2D* leaf_b = nullptr;
};

struct StackRig {
    std::unique_ptr<GroupNode> root;
    scene::NanNode2D* low = nullptr;
    scene::NanNode2D* mid = nullptr;
    scene::NanNode2D* high = nullptr;
};

auto build_deep_chain(Color color, float dx, float dy, const char* prefix) -> DeepChain {
    auto leaf = std::make_unique<CardNode>(foundation::NanSize(24, 16), color, "L4", false);
    leaf->set_name(std::string(prefix) + "_leaf");
    leaf->set_position(foundation::NanPoint(dx, dy));
    auto* leaf_ptr = leaf.get();

    auto l3 = std::make_unique<CardNode>(foundation::NanSize(28, 20), color, "L3", false);
    l3->set_position(foundation::NanPoint(dx, dy));
    l3->add_child(std::move(leaf));

    auto l2 = std::make_unique<CardNode>(foundation::NanSize(32, 24), color, "L2", false);
    l2->set_position(foundation::NanPoint(dx, dy));
    l2->add_child(std::move(l3));

    auto l1 = std::make_unique<CardNode>(foundation::NanSize(36, 28), color, "L1", false);
    l1->set_position(foundation::NanPoint(dx, dy));
    l1->add_child(std::move(l2));

    auto root = std::make_unique<CardNode>(foundation::NanSize(40, 32), color, "Root", false);
    root->set_name(std::string(prefix) + "_root");
    root->add_child(std::move(l1));

    return {std::move(root), leaf_ptr};
}

auto build_transform_rig(foundation::NanPoint center, Color primary, Color accent) -> TransformRig {
    TransformRig rig;

    auto root = std::make_unique<GroupNode>();
    rig.root_node = root.get();
    rig.root_node->set_name("transform_rig");
    rig.root_node->set_position(center);

    auto spinner = std::make_unique<GroupNode>();
    rig.spinner = spinner.get();
    rig.spinner->set_name("transform_spinner");

    auto shell = std::make_unique<CardNode>(foundation::NanSize(148, 104), primary, "Panel", false);
    rig.shell = shell.get();
    rig.shell->set_name("panel_shell");

    auto card = std::make_unique<CardNode>(foundation::NanSize(86, 62), accent, "Card", false);
    card->set_name("panel_card");
    card->set_position(foundation::NanPoint(0, 0));

    auto button = std::make_unique<CardNode>(
        foundation::NanSize(46, 28),
        Color {220, 180, 80, 255},
        "Node",
        true
    );
    rig.button = button.get();
    rig.button->set_name("panel_button");
    rig.button->set_position(foundation::NanPoint(0, 0));

    card->add_child(std::move(button));
    shell->add_child(std::move(card));
    spinner->add_child(std::move(shell));
    root->add_child(std::move(spinner));
    rig.root = std::move(root);

    return rig;
}

auto build_chain_rig(foundation::NanPoint center, Color chain_color, Color accent) -> ChainRig {
    ChainRig rig;

    auto root = std::make_unique<GroupNode>();
    root->set_name("chain_rig");
    root->set_position(center);

    auto [chain_a, leaf_a] = build_deep_chain(chain_color, 8.0F, 12.0F, "chain_a");
    auto [chain_b, leaf_b] = build_deep_chain(accent, 14.0F, -8.0F, "chain_b");
    chain_b->set_position(foundation::NanPoint(160, 0));

    rig.leaf_a = leaf_a;
    rig.leaf_b = leaf_b;

    root->add_child(std::move(chain_a));
    root->add_child(std::move(chain_b));
    rig.root = std::move(root);

    return rig;
}

auto build_stack_rig(foundation::NanPoint center, Color low, Color mid, Color high) -> StackRig {
    StackRig rig;

    auto root = std::make_unique<GroupNode>();
    root->set_name("stack_rig");
    root->set_position(center);

    auto low_card = std::make_unique<CardNode>(foundation::NanSize(82, 82), low, "z=0", true);
    rig.low = low_card.get();
    low_card->set_name("stack_low");
    low_card->set_position(foundation::NanPoint(-30, -20));

    auto mid_card = std::make_unique<CardNode>(foundation::NanSize(82, 82), mid, "z=5", true);
    rig.mid = mid_card.get();
    mid_card->set_name("stack_mid");
    mid_card->set_position(foundation::NanPoint(20, -30));
    mid_card->set_z_index(5);

    auto high_card = std::make_unique<CardNode>(foundation::NanSize(82, 82), high, "z=10", true);
    rig.high = high_card.get();
    high_card->set_name("stack_high");
    high_card->set_position(foundation::NanPoint(10, 20));
    high_card->set_z_index(10);

    root->add_child(std::move(low_card));
    root->add_child(std::move(mid_card));
    root->add_child(std::move(high_card));
    rig.root = std::move(root);

    return rig;
}

void draw_panel(float x, float y, float w, float h, Color bg, Color border) {
    DrawRectangle(
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<int>(w),
        static_cast<int>(h),
        bg
    );
    DrawRectangleLines(
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<int>(w),
        static_cast<int>(h),
        border
    );
}

void draw_section(
    float x,
    float y,
    float w,
    float h,
    const char* title,
    const char* subtitle,
    Color text,
    Color subtle,
    Color bg,
    Color border
) {
    draw_panel(x, y, w, h, bg, border);
    DrawText(title, static_cast<int>(x + 10), static_cast<int>(y + 8), 14, text);
    DrawText(subtitle, static_cast<int>(x + 10), static_cast<int>(y + 26), 12, subtle);
}

#endif // NANOSUIT_UTILS_HPP
