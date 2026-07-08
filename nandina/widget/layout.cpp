//
// widget/layout — minimal widget-style layout containers.
//

#include "layout.hpp"

#include <algorithm>
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

        [[nodiscard]] auto expanded_flex(const std::shared_ptr<scene::NanControl>& child) -> int;
    } // namespace

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
