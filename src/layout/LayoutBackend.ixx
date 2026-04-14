module;

#include <algorithm>
#include <cstdint>
#include <vector>

export module Nandina.Layout.Backend;

import Nandina.Types;

export namespace Nandina::layout_detail {

    enum class LayoutAxis : std::uint8_t {
        column,
        row,
        stack,
    };

    enum class LayoutAlignment : std::uint8_t {
        start,
        center,
        end,
        stretch,
        space_between,
        space_around,
    };

    struct LayoutChildSpec {
        Size preferred_size{};
        int flex_factor = 0;
    };

    struct LayoutInsets {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
    };

    struct LayoutRequest {
        LayoutAxis axis = LayoutAxis::column;
        Rect container_bounds{};
        LayoutInsets padding{};
        float gap = 0.0f;
        LayoutAlignment cross_alignment = LayoutAlignment::start;
        LayoutAlignment main_alignment = LayoutAlignment::start;
        std::vector<LayoutChildSpec> children;

        [[nodiscard]] auto content_bounds() const noexcept -> Rect {
            return {
                container_bounds.x() + padding.left,
                container_bounds.y() + padding.top,
                std::max(0.0f, container_bounds.width() - padding.left - padding.right),
                std::max(0.0f, container_bounds.height() - padding.top - padding.bottom),
            };
        }
    };

    class LayoutBackend {
    public:
        virtual ~LayoutBackend() = default;

        [[nodiscard]] virtual auto compute(const LayoutRequest& request) const -> std::vector<Rect> = 0;
    };

    namespace detail {
        [[nodiscard]] inline auto clamp_non_negative(float value) noexcept -> float {
            return std::max(0.0f, value);
        }

        [[nodiscard]] inline auto resolve_cross_extent(LayoutAlignment align, float available, float desired) noexcept -> float {
            if (align == LayoutAlignment::stretch) {
                return clamp_non_negative(available);
            }
            return std::min(clamp_non_negative(desired), clamp_non_negative(available));
        }

        [[nodiscard]] inline auto resolve_cross_position(float origin, float available, float extent, LayoutAlignment align) noexcept -> float {
            switch (align) {
            case LayoutAlignment::center:
            case LayoutAlignment::space_between:
            case LayoutAlignment::space_around:
                return origin + (available - extent) * 0.5f;
            case LayoutAlignment::end:
                return origin + (available - extent);
            case LayoutAlignment::start:
            case LayoutAlignment::stretch:
            default:
                return origin;
            }
        }

        struct MainAxisPlacement {
            float start_offset = 0.0f;
            float gap = 0.0f;
        };

        [[nodiscard]] inline auto resolve_main_axis_placement(LayoutAlignment justify,
                                                              int child_count,
                                                              float used_extent,
                                                              float available_extent,
                                                              float base_gap) noexcept -> MainAxisPlacement {
            const float free_space = clamp_non_negative(available_extent - used_extent);

            switch (justify) {
            case LayoutAlignment::end:
                return {.start_offset = free_space, .gap = base_gap};
            case LayoutAlignment::center:
                return {.start_offset = free_space * 0.5f, .gap = base_gap};
            case LayoutAlignment::space_between:
                if (child_count > 1) {
                    return {.start_offset = 0.0f, .gap = base_gap + free_space / static_cast<float>(child_count - 1)};
                }
                return {.start_offset = free_space * 0.5f, .gap = base_gap};
            case LayoutAlignment::space_around:
                if (child_count > 0) {
                    const float extra = free_space / static_cast<float>(child_count);
                    return {.start_offset = extra * 0.5f, .gap = base_gap + extra};
                }
                return {.start_offset = 0.0f, .gap = base_gap};
            case LayoutAlignment::start:
            case LayoutAlignment::stretch:
            default:
                return {.start_offset = 0.0f, .gap = base_gap};
            }
        }
    } // namespace detail

    class BasicLayoutBackend final : public LayoutBackend {
    public:
        [[nodiscard]] auto compute(const LayoutRequest& request) const -> std::vector<Rect> override {
            switch (request.axis) {
            case LayoutAxis::column:
                return compute_column(request);
            case LayoutAxis::row:
                return compute_row(request);
            case LayoutAxis::stack:
                return compute_stack(request);
            }

            return {};
        }

    private:
        [[nodiscard]] static auto compute_column(const LayoutRequest& request) -> std::vector<Rect> {
            const auto content_bounds = request.content_bounds();
            const float avail_w = detail::clamp_non_negative(content_bounds.width());
            const float avail_h = detail::clamp_non_negative(content_bounds.height());

            float fixed_total = 0.0f;
            int flex_total = 0;

            for (const auto& child : request.children) {
                if (child.flex_factor > 0) {
                    flex_total += child.flex_factor;
                } else {
                    fixed_total += detail::clamp_non_negative(child.preferred_size.height());
                }
            }

            const int child_count = static_cast<int>(request.children.size());
            const float gap_total = child_count > 1 ? request.gap * static_cast<float>(child_count - 1) : 0.0f;
            const float remaining = detail::clamp_non_negative(avail_h - fixed_total - gap_total);
            const float flex_inv = flex_total > 0 ? 1.0f / static_cast<float>(flex_total) : 0.0f;
            const float used_h = fixed_total + gap_total + (flex_total > 0 ? remaining : 0.0f);
            const auto placement = detail::resolve_main_axis_placement(request.main_alignment,
                                                                       child_count,
                                                                       used_h,
                                                                       avail_h,
                                                                       request.gap);

            std::vector<Rect> frames;
            frames.reserve(request.children.size());

            float cursor_y = content_bounds.y() + placement.start_offset;
            bool first = true;

            for (const auto& child : request.children) {
                if (!first) {
                    cursor_y += placement.gap;
                }
                first = false;

                const float child_h = (child.flex_factor > 0 && flex_total > 0)
                    ? remaining * (static_cast<float>(child.flex_factor) * flex_inv)
                    : detail::clamp_non_negative(child.preferred_size.height());
                const float child_w = detail::resolve_cross_extent(request.cross_alignment,
                                                                   avail_w,
                                                                   detail::clamp_non_negative(child.preferred_size.width()));
                const float child_x = detail::resolve_cross_position(content_bounds.x(),
                                                                     avail_w,
                                                                     child_w,
                                                                     request.cross_alignment);

                frames.emplace_back(child_x, cursor_y, child_w, child_h);
                cursor_y += child_h;
            }

            return frames;
        }

        [[nodiscard]] static auto compute_row(const LayoutRequest& request) -> std::vector<Rect> {
            const auto content_bounds = request.content_bounds();
            const float avail_w = detail::clamp_non_negative(content_bounds.width());
            const float avail_h = detail::clamp_non_negative(content_bounds.height());

            float fixed_total = 0.0f;
            int flex_total = 0;

            for (const auto& child : request.children) {
                if (child.flex_factor > 0) {
                    flex_total += child.flex_factor;
                } else {
                    fixed_total += detail::clamp_non_negative(child.preferred_size.width());
                }
            }

            const int child_count = static_cast<int>(request.children.size());
            const float gap_total = child_count > 1 ? request.gap * static_cast<float>(child_count - 1) : 0.0f;
            const float remaining = detail::clamp_non_negative(avail_w - fixed_total - gap_total);
            const float flex_inv = flex_total > 0 ? 1.0f / static_cast<float>(flex_total) : 0.0f;
            const float used_w = fixed_total + gap_total + (flex_total > 0 ? remaining : 0.0f);
            const auto placement = detail::resolve_main_axis_placement(request.main_alignment,
                                                                       child_count,
                                                                       used_w,
                                                                       avail_w,
                                                                       request.gap);

            std::vector<Rect> frames;
            frames.reserve(request.children.size());

            float cursor_x = content_bounds.x() + placement.start_offset;
            bool first = true;

            for (const auto& child : request.children) {
                if (!first) {
                    cursor_x += placement.gap;
                }
                first = false;

                const float child_w = (child.flex_factor > 0 && flex_total > 0)
                    ? remaining * (static_cast<float>(child.flex_factor) * flex_inv)
                    : detail::clamp_non_negative(child.preferred_size.width());
                const float child_h = detail::resolve_cross_extent(request.cross_alignment,
                                                                   avail_h,
                                                                   detail::clamp_non_negative(child.preferred_size.height()));
                const float child_y = detail::resolve_cross_position(content_bounds.y(),
                                                                     avail_h,
                                                                     child_h,
                                                                     request.cross_alignment);

                frames.emplace_back(cursor_x, child_y, child_w, child_h);
                cursor_x += child_w;
            }

            return frames;
        }

        [[nodiscard]] static auto compute_stack(const LayoutRequest& request) -> std::vector<Rect> {
            const auto content_bounds = request.content_bounds();
            const float avail_w = detail::clamp_non_negative(content_bounds.width());
            const float avail_h = detail::clamp_non_negative(content_bounds.height());

            std::vector<Rect> frames;
            frames.reserve(request.children.size());

            for (const auto& child : request.children) {
                const float child_w = detail::resolve_cross_extent(request.cross_alignment,
                                                                   avail_w,
                                                                   detail::clamp_non_negative(child.preferred_size.width()));
                const float child_h = detail::resolve_cross_extent(request.main_alignment,
                                                                   avail_h,
                                                                   detail::clamp_non_negative(child.preferred_size.height()));
                const float child_x = detail::resolve_cross_position(content_bounds.x(),
                                                                     avail_w,
                                                                     child_w,
                                                                     request.cross_alignment);
                const float child_y = detail::resolve_cross_position(content_bounds.y(),
                                                                     avail_h,
                                                                     child_h,
                                                                     request.main_alignment);
                frames.emplace_back(child_x, child_y, child_w, child_h);
            }

            return frames;
        }
    };

    [[nodiscard]] inline auto default_layout_backend() -> const LayoutBackend& {
        static const BasicLayoutBackend backend{};
        return backend;
    }

} // namespace Nandina::layout_detail