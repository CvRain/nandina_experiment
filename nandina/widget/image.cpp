//
// widget/image - textured image node implementation.
//

#include "image.hpp"

#include "../render/draw_context.hpp"
#include "../resource/resource_manager.hpp"
#include "../resource/resource_uri.hpp"

#include <algorithm>
#include <utility>

namespace nandina::widget
{
    Image::Image(std::string source): source_(std::move(source)) {}

    auto Image::create(std::string source) -> std::shared_ptr<Image> {
        return std::make_shared<Image>(std::move(source));
    }

    void Image::set_source(std::string path) {
        if (source_ == path) {
            return;
        }
        source_ = std::move(path);
        texture_ = {};
        natural_size_ = foundation::NanSize {};
        loaded_ = false;
        mark_layout_dirty();
    }

    auto Image::source() const -> std::string_view {
        return source_;
    }

    void Image::set_resource_manager(resource::ResourceManager* resources) {
        if (resources_ == resources) {
            return;
        }
        resources_ = resources;
        texture_ = {};
        natural_size_ = foundation::NanSize {};
        loaded_ = false;
        mark_layout_dirty();
    }

    auto Image::resource_manager() const noexcept -> resource::ResourceManager* {
        return resources_;
    }

    void Image::set_tint(const foundation::NanColor tint) {
        tint_ = tint;
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Image::tint() const -> foundation::NanColor {
        return tint_;
    }

    void Image::set_source_rect(const foundation::NanRect rect) {
        source_rect_ = rect;
        mark_dirty(scene::DirtyFlags::paint);
    }

    void Image::clear_source_rect() {
        source_rect_.reset();
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Image::source_rect() const -> const std::optional<foundation::NanRect>& {
        return source_rect_;
    }

    void Image::set_scale_mode(const ImageScale mode) {
        scale_mode_ = mode;
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Image::scale_mode() const -> ImageScale {
        return scale_mode_;
    }

    void Image::set_alignment(const ImageAlignment alignment) {
        alignment_ = alignment;
        mark_dirty(scene::DirtyFlags::paint);
    }

    auto Image::alignment() const -> ImageAlignment {
        return alignment_;
    }

    auto Image::natural_size() const -> foundation::NanSize {
        return natural_size_;
    }

    void Image::set_load_options(render::ImageLoadOptions options) {
        load_options_ = std::move(options);
        texture_ = {};
        natural_size_ = foundation::NanSize {};
        loaded_ = false;
        mark_layout_dirty();
    }

    auto Image::load_options() const -> const render::ImageLoadOptions& {
        return load_options_;
    }

    auto Image::on_draw(render::DrawContext& ctx) -> void {
        ensure_loaded(ctx.device());
        if (!texture_) {
            return; // 未指定源或加载失败：不绘制。
        }
        const auto world = render::world_bounds_from_local(ctx.world_transform(), local_rect());
        const auto rects = compute_rects(world);
        const auto tint = tint_.with_alpha(tint_.alpha() * ctx.opacity());
        ctx.device().draw_texture_region(texture_, rects.source, rects.destination, tint);
    }

    auto Image::on_measure(const scene::LayoutConstraints constraints) -> foundation::NanSize {
        if (natural_size_.get_width() > 0.0F || natural_size_.get_height() > 0.0F) {
            return constraints.constrain(natural_size_);
        }
        return constraints.constrain(size());
    }

    void Image::ensure_loaded(render::IRenderDevice& device) {
        if (loaded_ || source_.empty()) {
            return;
        }
        loaded_ = true; // 即使失败也标记，避免每帧重试。
        if (source_.starts_with("res://")) {
            if (!resources_) {
                return;
            }
            const auto uri = resource::ResourceUri::parse(source_);
            if (!uri || uri->scheme() != resource::ResourceUriScheme::res) {
                return;
            }
            const auto key = uri->resource_key();
            if (!key) {
                return;
            }
            const auto loaded = resources_->require(*key);
            if (!loaded || !(*loaded)->media_type().starts_with("image/")) {
                return;
            }
            texture_ = device.load_texture_from_memory(
                (*loaded)->bytes(),
                (*loaded)->media_type(),
                load_options_
            );
        }
        else {
            texture_ = device.load_texture_from_file(source_, load_options_);
        }
        natural_size_ = device.texture_size(texture_);
        if (natural_size_.get_width() > 0.0F || natural_size_.get_height() > 0.0F) {
            mark_layout_dirty();
        }
    }

    auto Image::compute_rects(const foundation::NanRect& world) const -> DrawRects {
        const foundation::NanRect source_base = source_rect_.value_or(
            foundation::NanRect::from_xywh(
                0.0F,
                0.0F,
                natural_size_.get_width(),
                natural_size_.get_height()
            )
        );
        const float sw = source_base.get_width();
        const float sh = source_base.get_height();
        const float dw = world.get_width();
        const float dh = world.get_height();
        if (sw <= 0.0F || sh <= 0.0F || dw <= 0.0F || dh <= 0.0F) {
            return DrawRects {source_base, world};
        }

        if (scale_mode_ == ImageScale::contain) {
            const float scale = std::min(dw / sw, dh / sh);
            const float drawn_w = sw * scale;
            const float drawn_h = sh * scale;
            float dx = world.get_left();
            float dy = world.get_top();
            switch (alignment_) {
                case ImageAlignment::start:
                    dx = world.get_left();
                    dy = world.get_top();
                    break;
                case ImageAlignment::end:
                    dx = world.get_right() - drawn_w;
                    dy = world.get_bottom() - drawn_h;
                    break;
                case ImageAlignment::center:
                default:
                    dx = world.get_left() + (dw - drawn_w) * 0.5F;
                    dy = world.get_top() + (dh - drawn_h) * 0.5F;
                    break;
            }
            return DrawRects {
                source_base,
                foundation::NanRect::from_xywh(dx, dy, drawn_w, drawn_h),
            };
        }

        if (scale_mode_ == ImageScale::cover) {
            const float src_ratio = sw / sh;
            const float dst_ratio = dw / dh;
            float crop_w = sw;
            float crop_h = sh;
            if (src_ratio > dst_ratio) {
                crop_w = sh * dst_ratio; // 源更宽：裁宽度
            }
            else {
                crop_h = sw / dst_ratio; // 源更高：裁高度
            }
            const float crop_x = source_base.get_left() + (sw - crop_w) * 0.5F;
            const float crop_y = source_base.get_top() + (sh - crop_h) * 0.5F;
            return DrawRects {
                foundation::NanRect::from_xywh(crop_x, crop_y, crop_w, crop_h),
                world,
            };
        }

        // stretch（默认）：整源拉伸到整目标。
        return DrawRects {source_base, world};
    }
} // namespace nandina::widget
