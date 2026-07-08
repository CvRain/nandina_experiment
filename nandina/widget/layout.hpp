//
// widget/layout — minimal widget-style layout containers.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_LAYOUT_HPP
#define NANDINA_EXPERIMENT_WIDGET_LAYOUT_HPP

#include "../foundation/geometry.hpp"
#include "../scene/control.hpp"

#include <memory>
#include <vector>

namespace nandina::widget
{

    enum class LayoutAlignment {
        start,
        center,
        end,
        stretch,
    };

    class Column: public scene::NanControl {
    public:
        [[nodiscard]] static auto create() -> std::shared_ptr<Column>;

        auto add(std::shared_ptr<scene::NanControl> child) -> Column&;
        auto set_gap(float gap) -> Column&;
        auto set_main_alignment(LayoutAlignment alignment) -> Column&;
        auto set_cross_alignment(LayoutAlignment alignment) -> Column&;
        [[nodiscard]] auto gap() const -> float;
        [[nodiscard]] auto main_alignment() const -> LayoutAlignment;
        [[nodiscard]] auto cross_alignment() const -> LayoutAlignment;

        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        float gap_ = 0.0F;
        LayoutAlignment main_alignment_ = LayoutAlignment::start;
        LayoutAlignment cross_alignment_ = LayoutAlignment::start;
        std::vector<std::weak_ptr<scene::NanControl>> items_;
    };

    class Row: public scene::NanControl {
    public:
        [[nodiscard]] static auto create() -> std::shared_ptr<Row>;

        auto add(std::shared_ptr<scene::NanControl> child) -> Row&;
        auto set_gap(float gap) -> Row&;
        auto set_main_alignment(LayoutAlignment alignment) -> Row&;
        auto set_cross_alignment(LayoutAlignment alignment) -> Row&;
        [[nodiscard]] auto gap() const -> float;
        [[nodiscard]] auto main_alignment() const -> LayoutAlignment;
        [[nodiscard]] auto cross_alignment() const -> LayoutAlignment;

        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        float gap_ = 0.0F;
        LayoutAlignment main_alignment_ = LayoutAlignment::start;
        LayoutAlignment cross_alignment_ = LayoutAlignment::start;
        std::vector<std::weak_ptr<scene::NanControl>> items_;
    };

    class Padding: public scene::NanControl {
    public:
        explicit Padding(foundation::NanInsets insets = foundation::NanInsets::all(0.0F));

        [[nodiscard]] static auto
        create(foundation::NanInsets insets = foundation::NanInsets::all(0.0F))
            -> std::shared_ptr<Padding>;

        auto set_child(std::shared_ptr<scene::NanControl> child) -> Padding&;
        auto set_padding(foundation::NanInsets insets) -> Padding&;
        [[nodiscard]] auto padding() const -> foundation::NanInsets;

        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        foundation::NanInsets insets_;
        std::weak_ptr<scene::NanControl> child_;
    };

    class Center: public scene::NanControl {
    public:
        [[nodiscard]] static auto create() -> std::shared_ptr<Center>;

        auto set_child(std::shared_ptr<scene::NanControl> child) -> Center&;
        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        std::weak_ptr<scene::NanControl> child_;
    };

    class Expanded: public scene::NanControl {
    public:
        explicit Expanded(int flex = 1);

        [[nodiscard]] static auto create(int flex = 1) -> std::shared_ptr<Expanded>;

        auto set_child(std::shared_ptr<scene::NanControl> child) -> Expanded&;
        auto set_flex(int flex) -> Expanded&;
        [[nodiscard]] auto flex() const -> int;
        [[nodiscard]] auto layout_flex_factor() const -> int override;

        void relayout();

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize override;
        auto on_layout() -> void override;
        void on_ready() override;

    private:
        int flex_ = 1;
        std::weak_ptr<scene::NanControl> child_;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_LAYOUT_HPP
