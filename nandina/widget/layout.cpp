//
// widget/layout — minimal widget-style layout containers.
//

#include "layout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace nandina::widget
{
    namespace
    {
        template<typename List>
        void prune_expired(List& items) {
            std::erase_if(items, [](const auto& item) { return item.expired(); });
        }

        [[nodiscard]] auto child_constraints(scene::LayoutConstraints constraints)
            -> scene::LayoutConstraints {
            return {
                .min_width = 0.0F,
                .max_width = constraints.max_width,
                .min_height = 0.0F,
                .max_height = constraints.max_height,
            };
        }

        [[nodiscard]] auto alignment_offset(
            LayoutAlignment alignment,
            float available,
            float used
        ) -> float {
            const auto remaining = std::max(0.0F, available - used);
            switch (alignment) {
            case LayoutAlignment::center:
                return remaining * 0.5F;
            case LayoutAlignment::end:
                return remaining;
            case LayoutAlignment::start:
            case LayoutAlignment::stretch:
            default:
                return 0.0F;
            }
        }

        [[nodiscard]] auto stretched_extent(
            LayoutAlignment alignment,
            float available,
            float measured
        ) -> float {
            if (alignment == LayoutAlignment::stretch) {
                return std::max(0.0F, available);
            }
            return measured;
        }

        [[nodiscard]] auto main_extent(foundation::NanSize size, LayoutAxis axis) -> float {
            return axis == LayoutAxis::horizontal ? size.get_width() : size.get_height();
        }

        [[nodiscard]] auto cross_extent(foundation::NanSize size, LayoutAxis axis) -> float {
            return axis == LayoutAxis::horizontal ? size.get_height() : size.get_width();
        }

        [[nodiscard]] auto max_main(scene::LayoutConstraints constraints, LayoutAxis axis) -> float {
            return axis == LayoutAxis::horizontal ? constraints.max_width : constraints.max_height;
        }

        [[nodiscard]] auto size_from_extents(float main, float cross, LayoutAxis axis)
            -> foundation::NanSize {
            if (axis == LayoutAxis::horizontal) {
                return foundation::NanSize(main, cross);
            }
            return foundation::NanSize(cross, main);
        }

        [[nodiscard]] auto rect_from_extents(
            float main_pos,
            float cross_pos,
            float main_size,
            float cross_size,
            LayoutAxis axis
        ) -> foundation::NanRect {
            if (axis == LayoutAxis::horizontal) {
                return foundation::NanRect::from_xywh(main_pos, cross_pos, main_size, cross_size);
            }
            return foundation::NanRect::from_xywh(cross_pos, main_pos, cross_size, main_size);
        }

        [[nodiscard]] auto effective_limit(float limit) -> float {
            return std::isfinite(limit) ? limit : std::numeric_limits<float>::infinity();
        }

        [[nodiscard]] auto expanded_flex(const std::shared_ptr<scene::NanControl>& child) -> int;

        struct WrapRun {
            std::vector<std::shared_ptr<scene::NanControl>> children;
            float main = 0.0F;
            float cross = 0.0F;
        };

        [[nodiscard]] auto collect_wrap_runs(
            std::vector<std::weak_ptr<scene::NanControl>>& items,
            scene::LayoutConstraints constraints,
            LayoutAxis axis,
            float gap
        ) -> std::vector<WrapRun> {
            prune_expired(items);

            std::vector<WrapRun> runs;
            WrapRun current;
            const float limit = effective_limit(max_main(constraints, axis));
            const bool can_wrap = std::isfinite(limit) && limit >= 0.0F;

            for (auto& item: items) {
                auto child = item.lock();
                if (!child || !child->visible()) {
                    continue;
                }

                const auto measured = child->measure_layout(child_constraints(constraints));
                const float child_main = main_extent(measured, axis);
                const float child_cross = cross_extent(measured, axis);
                const float next_main = current.children.empty()
                    ? child_main
                    : current.main + gap + child_main;

                if (can_wrap && !current.children.empty() && next_main > limit) {
                    runs.push_back(std::move(current));
                    current = WrapRun {};
                }

                if (!current.children.empty()) {
                    current.main += gap;
                }
                current.main += child_main;
                current.cross = std::max(current.cross, child_cross);
                current.children.push_back(std::move(child));
            }

            if (!current.children.empty()) {
                runs.push_back(std::move(current));
            }
            return runs;
        }
    } // namespace

    Flex::Flex(LayoutAxis axis): axis_(axis) {}

    auto Flex::create(LayoutAxis axis) -> std::shared_ptr<Flex> {
        return std::make_shared<Flex>(axis);
    }

    auto Flex::add(std::shared_ptr<scene::NanControl> child) -> Flex& {
        if (!child) {
            throw std::runtime_error("Flex::add: child is null");
        }
        items_.push_back(child);
        add_child(std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Flex::set_axis(LayoutAxis axis) -> Flex& {
        axis_ = axis;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Flex::set_gap(float gap) -> Flex& {
        gap_ = gap;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Flex::set_main_alignment(LayoutAlignment alignment) -> Flex& {
        main_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Flex::set_cross_alignment(LayoutAlignment alignment) -> Flex& {
        cross_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Flex::axis() const -> LayoutAxis {
        return axis_;
    }

    auto Flex::gap() const -> float {
        return gap_;
    }

    auto Flex::main_alignment() const -> LayoutAlignment {
        return main_alignment_;
    }

    auto Flex::cross_alignment() const -> LayoutAlignment {
        return cross_alignment_;
    }

    void Flex::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Flex::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        prune_expired(items_);

        float used_main = 0.0F;
        float used_cross = 0.0F;
        std::size_t visible_count = 0;
        for (auto& item: items_) {
            auto child = item.lock();
            if (!child || !child->visible()) {
                continue;
            }
            if (visible_count > 0) {
                used_main += gap_;
            }
            const auto measured = child->measure_layout(child_constraints(constraints));
            used_main += main_extent(measured, axis_);
            used_cross = std::max(used_cross, cross_extent(measured, axis_));
            ++visible_count;
        }

        return constraints.constrain(size_from_extents(used_main, used_cross, axis_));
    }

    auto Flex::on_layout() -> void {
        prune_expired(items_);

        const float available_main = main_extent(size(), axis_);
        const float available_cross = cross_extent(size(), axis_);
        float fixed_main = 0.0F;
        int total_flex = 0;
        std::size_t count = 0;
        for (auto& item: items_) {
            auto child = item.lock();
            if (!child || !child->visible()) {
                continue;
            }
            if (count > 0) {
                fixed_main += gap_;
            }
            if (const int flex = expanded_flex(child); flex > 0) {
                total_flex += flex;
            } else {
                fixed_main += main_extent(child->measured_size(), axis_);
            }
            ++count;
        }

        const auto remaining_main = std::max(0.0F, available_main - fixed_main);
        const auto used_main = total_flex > 0 ? available_main : fixed_main;
        float main_pos = alignment_offset(main_alignment_, available_main, used_main);
        std::size_t visible_count = 0;
        for (auto& item: items_) {
            auto child = item.lock();
            if (!child || !child->visible()) {
                continue;
            }
            if (visible_count > 0) {
                main_pos += gap_;
            }

            const auto child_size = child->measured_size();
            const int flex = expanded_flex(child);
            const auto child_main = total_flex > 0 && flex > 0
                ? remaining_main * (static_cast<float>(flex) / static_cast<float>(total_flex))
                : main_extent(child_size, axis_);
            const auto child_cross = stretched_extent(cross_alignment_, available_cross, cross_extent(child_size, axis_));
            const auto child_cross_pos = alignment_offset(cross_alignment_, available_cross, child_cross);

            child->layout_to(rect_from_extents(main_pos, child_cross_pos, child_main, child_cross, axis_));
            main_pos += child_main;
            ++visible_count;
        }
    }

    void Flex::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    Wrap::Wrap(LayoutAxis axis): axis_(axis) {}

    auto Wrap::create(LayoutAxis axis) -> std::shared_ptr<Wrap> {
        return std::make_shared<Wrap>(axis);
    }

    auto Wrap::add(std::shared_ptr<scene::NanControl> child) -> Wrap& {
        if (!child) {
            throw std::runtime_error("Wrap::add: child is null");
        }
        items_.push_back(child);
        add_child(std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::set_axis(LayoutAxis axis) -> Wrap& {
        axis_ = axis;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::set_gap(float gap) -> Wrap& {
        gap_ = gap;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::set_run_gap(float gap) -> Wrap& {
        run_gap_ = gap;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::set_main_alignment(LayoutAlignment alignment) -> Wrap& {
        main_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::set_cross_alignment(LayoutAlignment alignment) -> Wrap& {
        cross_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::set_run_alignment(LayoutAlignment alignment) -> Wrap& {
        run_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Wrap::axis() const -> LayoutAxis {
        return axis_;
    }

    auto Wrap::gap() const -> float {
        return gap_;
    }

    auto Wrap::run_gap() const -> float {
        return run_gap_;
    }

    auto Wrap::main_alignment() const -> LayoutAlignment {
        return main_alignment_;
    }

    auto Wrap::cross_alignment() const -> LayoutAlignment {
        return cross_alignment_;
    }

    auto Wrap::run_alignment() const -> LayoutAlignment {
        return run_alignment_;
    }

    void Wrap::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Wrap::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        const auto runs = collect_wrap_runs(items_, constraints, axis_, gap_);
        float used_main = 0.0F;
        float used_cross = 0.0F;
        for (std::size_t i = 0; i < runs.size(); ++i) {
            used_main = std::max(used_main, runs[i].main);
            if (i > 0) {
                used_cross += run_gap_;
            }
            used_cross += runs[i].cross;
        }
        return constraints.constrain(size_from_extents(used_main, used_cross, axis_));
    }

    auto Wrap::on_layout() -> void {
        const auto runs = collect_wrap_runs(items_, scene::LayoutConstraints::tight(size()), axis_, gap_);
        const float available_main = main_extent(size(), axis_);
        const float available_cross = cross_extent(size(), axis_);
        float used_cross = 0.0F;
        for (std::size_t i = 0; i < runs.size(); ++i) {
            if (i > 0) {
                used_cross += run_gap_;
            }
            used_cross += runs[i].cross;
        }

        float run_pos = alignment_offset(run_alignment_, available_cross, used_cross);
        for (const auto& run: runs) {
            float child_pos = alignment_offset(main_alignment_, available_main, run.main);
            for (const auto& child: run.children) {
                const auto child_size = child->measured_size();
                const auto child_main = main_extent(child_size, axis_);
                const auto child_cross = stretched_extent(cross_alignment_, run.cross, cross_extent(child_size, axis_));
                const auto child_cross_pos = run_pos + alignment_offset(cross_alignment_, run.cross, child_cross);
                child->layout_to(rect_from_extents(child_pos, child_cross_pos, child_main, child_cross, axis_));
                child_pos += child_main + gap_;
            }
            run_pos += run.cross + run_gap_;
        }
    }

    void Wrap::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    auto Column::create() -> std::shared_ptr<Column> {
        return std::make_shared<Column>();
    }

    auto Column::add(std::shared_ptr<scene::NanControl> child) -> Column& {
        if (!child) {
            throw std::runtime_error("Column::add: child is null");
        }
        items_.push_back(child);
        add_child(std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Column::set_gap(float gap) -> Column& {
        gap_ = gap;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Column::set_main_alignment(LayoutAlignment alignment) -> Column& {
        main_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Column::set_cross_alignment(LayoutAlignment alignment) -> Column& {
        cross_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Column::gap() const -> float {
        return gap_;
    }

    auto Column::main_alignment() const -> LayoutAlignment {
        return main_alignment_;
    }

    auto Column::cross_alignment() const -> LayoutAlignment {
        return cross_alignment_;
    }

    void Column::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Column::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        prune_expired(items_);

        float y = 0.0F;
        float max_width = 0.0F;
        std::size_t visible_count = 0;
        for (auto& item: items_) {
            auto child = item.lock();
            if (!child || !child->visible()) {
                continue;
            }
            if (visible_count > 0) {
                y += gap_;
            }
            const auto measured = child->measure_layout(child_constraints(constraints));
            y += measured.get_height();
            max_width = std::max(max_width, measured.get_width());
            ++visible_count;
        }

        return constraints.constrain(foundation::NanSize(max_width, y));
    }

    auto Column::on_layout() -> void {
        prune_expired(items_);

        float fixed_height = 0.0F;
        int total_flex = 0;
        std::size_t count = 0;
        for (auto& item: items_) {
            auto child = item.lock();
            if (!child || !child->visible()) {
                continue;
            }
            if (count > 0) {
                fixed_height += gap_;
            }
            if (const int flex = expanded_flex(child); flex > 0) {
                total_flex += flex;
            } else {
                fixed_height += child->measured_size().get_height();
            }
            ++count;
        }

        const auto remaining_height = std::max(0.0F, height() - fixed_height);
        const auto used_height = total_flex > 0 ? height() : fixed_height;
        float y = alignment_offset(main_alignment_, height(), used_height);
        std::size_t visible_count = 0;
        for (auto& item: items_) {
            auto child = item.lock();
            if (!child || !child->visible()) {
                continue;
            }
            if (visible_count > 0) {
                y += gap_;
            }
            const auto child_size = child->measured_size();
            const auto child_height = [&] {
                const int flex = expanded_flex(child);
                if (total_flex > 0 && flex > 0) {
                    return remaining_height * (static_cast<float>(flex) / static_cast<float>(total_flex));
                }
                return child_size.get_height();
            }();
            const auto child_width = stretched_extent(cross_alignment_, width(), child_size.get_width());
            const auto child_x = alignment_offset(cross_alignment_, width(), child_width);
            child->layout_to(foundation::NanRect::from_xywh(
                child_x,
                y,
                child_width,
                child_height
            ));
            y += child_height;
            ++visible_count;
        }
    }

    void Column::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    auto Row::create() -> std::shared_ptr<Row> {
        return std::make_shared<Row>();
    }

    auto Row::add(std::shared_ptr<scene::NanControl> child) -> Row& {
        if (!child) {
            throw std::runtime_error("Row::add: child is null");
        }
        items_.push_back(child);
        add_child(std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Row::set_gap(float gap) -> Row& {
        gap_ = gap;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Row::set_main_alignment(LayoutAlignment alignment) -> Row& {
        main_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Row::set_cross_alignment(LayoutAlignment alignment) -> Row& {
        cross_alignment_ = alignment;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Row::gap() const -> float {
        return gap_;
    }

    auto Row::main_alignment() const -> LayoutAlignment {
        return main_alignment_;
    }

    auto Row::cross_alignment() const -> LayoutAlignment {
        return cross_alignment_;
    }

    void Row::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Row::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        prune_expired(items_);

        float x = 0.0F;
        float max_height = 0.0F;
        std::size_t visible_count = 0;
        for (auto& item: items_) {
            auto child = item.lock();
            if (!child || !child->visible()) {
                continue;
            }
            if (visible_count > 0) {
                x += gap_;
            }
            const auto measured = child->measure_layout(child_constraints(constraints));
            x += measured.get_width();
            max_height = std::max(max_height, measured.get_height());
            ++visible_count;
        }

        return constraints.constrain(foundation::NanSize(x, max_height));
    }

    auto Row::on_layout() -> void {
        prune_expired(items_);

        float fixed_width = 0.0F;
        int total_flex = 0;
        std::size_t count = 0;
        for (auto& item: items_) {
            auto child = item.lock();
            if (!child || !child->visible()) {
                continue;
            }
            if (count > 0) {
                fixed_width += gap_;
            }
            if (const int flex = expanded_flex(child); flex > 0) {
                total_flex += flex;
            } else {
                fixed_width += child->measured_size().get_width();
            }
            ++count;
        }

        const auto remaining_width = std::max(0.0F, width() - fixed_width);
        const auto used_width = total_flex > 0 ? width() : fixed_width;
        float x = alignment_offset(main_alignment_, width(), used_width);
        std::size_t visible_count = 0;
        for (auto& item: items_) {
            auto child = item.lock();
            if (!child || !child->visible()) {
                continue;
            }
            if (visible_count > 0) {
                x += gap_;
            }
            const auto child_size = child->measured_size();
            const auto child_width = [&] {
                const int flex = expanded_flex(child);
                if (total_flex > 0 && flex > 0) {
                    return remaining_width * (static_cast<float>(flex) / static_cast<float>(total_flex));
                }
                return child_size.get_width();
            }();
            const auto child_height = stretched_extent(cross_alignment_, height(), child_size.get_height());
            const auto child_y = alignment_offset(cross_alignment_, height(), child_height);
            child->layout_to(foundation::NanRect::from_xywh(
                x,
                child_y,
                child_width,
                child_height
            ));
            x += child_width;
            ++visible_count;
        }
    }

    void Row::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    Padding::Padding(foundation::NanInsets insets): insets_(insets) {}

    auto Padding::create(foundation::NanInsets insets) -> std::shared_ptr<Padding> {
        return std::make_shared<Padding>(insets);
    }

    auto Padding::set_child(std::shared_ptr<scene::NanControl> child) -> Padding& {
        if (!child) {
            throw std::runtime_error("Padding::set_child: child is null");
        }
        if (auto current = child_.lock()) {
            remove_child(*current);
        }
        child_ = child;
        add_child(std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Padding::set_padding(foundation::NanInsets insets) -> Padding& {
        insets_ = insets;
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Padding::padding() const -> foundation::NanInsets {
        return insets_;
    }

    void Padding::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Padding::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto child = child_.lock();
        if (!child) {
            return constraints.constrain(
                foundation::NanSize(insets_.horizontal_sum(), insets_.vertical_sum())
            );
        }

        const auto measured = child->measure_layout(constraints.deflated(insets_));
        return constraints.constrain(
            foundation::NanSize(
                measured.get_width() + insets_.horizontal_sum(),
                measured.get_height() + insets_.vertical_sum()
            )
        );
    }

    auto Padding::on_layout() -> void {
        auto child = child_.lock();
        if (!child) {
            return;
        }

        const auto content_width = std::max(0.0F, width() - insets_.horizontal_sum());
        const auto content_height = std::max(0.0F, height() - insets_.vertical_sum());
        const auto child_size = child->measured_size();
        child->layout_to(foundation::NanRect::from_xywh(
            insets_.get_left(),
            insets_.get_top(),
            std::min(child_size.get_width(), content_width),
            std::min(child_size.get_height(), content_height)
        ));
    }

    void Padding::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    auto Center::create() -> std::shared_ptr<Center> {
        return std::make_shared<Center>();
    }

    auto Center::set_child(std::shared_ptr<scene::NanControl> child) -> Center& {
        if (!child) {
            throw std::runtime_error("Center::set_child: child is null");
        }
        if (auto current = child_.lock()) {
            remove_child(*current);
        }
        child_ = child;
        add_child(std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    void Center::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Center::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto child = child_.lock();
        if (!child) {
            return constraints.constrain(foundation::NanSize::zero());
        }

        const auto measured = child->measure_layout(child_constraints(constraints));
        return constraints.constrain(measured);
    }

    auto Center::on_layout() -> void {
        auto child = child_.lock();
        if (!child) {
            return;
        }

        const auto child_size = child->measured_size();
        const auto child_width = std::min(child_size.get_width(), width());
        const auto child_height = std::min(child_size.get_height(), height());
        child->layout_to(foundation::NanRect::from_xywh(
            (width() - child_width) * 0.5F,
            (height() - child_height) * 0.5F,
            child_width,
            child_height
        ));
    }

    void Center::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    Expanded::Expanded(int flex): flex_(std::max(1, flex)) {}

    auto Expanded::create(int flex) -> std::shared_ptr<Expanded> {
        return std::make_shared<Expanded>(flex);
    }

    auto Expanded::set_child(std::shared_ptr<scene::NanControl> child) -> Expanded& {
        if (!child) {
            throw std::runtime_error("Expanded::set_child: child is null");
        }
        if (auto current = child_.lock()) {
            remove_child(*current);
        }
        child_ = child;
        add_child(std::move(child));
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Expanded::set_flex(int flex) -> Expanded& {
        flex_ = std::max(1, flex);
        mark_layout_dirty();
        relayout();
        return *this;
    }

    auto Expanded::flex() const -> int {
        return flex_;
    }

    auto Expanded::layout_flex_factor() const -> int {
        return flex_;
    }

    void Expanded::relayout() {
        (void)measure_layout(scene::LayoutConstraints::loose());
        layout_to(foundation::NanRect::from_origin_size(position(), measured_size()));
    }

    auto Expanded::on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize {
        auto child = child_.lock();
        if (!child) {
            return constraints.constrain(foundation::NanSize::zero());
        }
        return constraints.constrain(child->measure_layout(child_constraints(constraints)));
    }

    auto Expanded::on_layout() -> void {
        auto child = child_.lock();
        if (!child) {
            return;
        }
        (void)child->measure_layout(scene::LayoutConstraints::tight(size()));
        child->layout_to(local_rect());
    }

    void Expanded::on_ready() {
        scene::NanControl::on_ready();
        relayout();
    }

    namespace
    {
        auto expanded_flex(const std::shared_ptr<scene::NanControl>& child) -> int {
            return child != nullptr ? child->layout_flex_factor() : 0;
        }
    } // namespace

} // namespace nandina::widget
