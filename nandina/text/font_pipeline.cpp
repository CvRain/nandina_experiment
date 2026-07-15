#include "font_pipeline.hpp"

#include <stdexcept>

namespace nandina::text
{
    FontPipeline::FontPipeline(
        render::IRenderDevice& device,
        ResolvedFontFamily family,
        const FontPipelineOptions options
    ):
        faces_(std::move(family.faces)) {
        if (faces_.empty() || options.atlas_width <= 0 || options.atlas_height <= 0
            || options.atlas_padding < 0)
        {
            throw std::invalid_argument("FontPipeline requires faces and valid atlas dimensions");
        }
        backend_ = std::make_unique<HarfBuzzTextLayoutBackend>(
            faces_.front(),
            std::vector<std::shared_ptr<FreeTypeFontFace>>(faces_.begin() + 1, faces_.end())
        );
        atlases_.reserve(faces_.size());
        textures_.reserve(faces_.size());
        bindings_.reserve(faces_.size());
        for (const auto& face: faces_) {
            auto atlas = std::make_unique<GlyphAtlas>(
                face,
                options.atlas_width,
                options.atlas_height,
                options.atlas_padding
            );
            auto texture = std::make_unique<GlyphAtlasTexture>(device, *atlas);
            bindings_.push_back({.atlas = atlas.get(), .texture = texture.get()});
            atlases_.push_back(std::move(atlas));
            textures_.push_back(std::move(texture));
        }
        renderer_ = std::make_unique<GlyphRunRenderer>(*backend_, bindings_);
    }

    auto FontPipeline::pipeline() const -> widget::primitives::TextPipeline {
        return {.backend = backend_.get(), .renderer = renderer_.get()};
    }
    auto FontPipeline::backend() const -> const HarfBuzzTextLayoutBackend& {
        return *backend_;
    }
    auto FontPipeline::font_count() const -> std::size_t {
        return faces_.size();
    }

    FontPipelineCache::FontPipelineCache(
        render::IRenderDevice& device,
        FontLoader& loader,
        const FontFamilyRegistry& families
    ):
        device_(&device),
        loader_(&loader),
        families_(&families) {}

    auto FontPipelineCache::get(FontRequest request, const FontPipelineOptions options)
        -> FontResult<std::shared_ptr<FontPipeline>> {
        const Key key {
            .family = request.family,
            .weight = request.weight,
            .slant = request.slant,
            .options = options,
        };
        std::lock_guard lock(mutex_);
        if (const auto found = cache_.find(key); found != cache_.end()) {
            if (auto cached = found->second.lock()) {
                return cached;
            }
        }
        auto family = families_->resolve(request, *loader_);
        if (!family) {
            return std::unexpected(family.error());
        }
        try {
            auto pipeline = std::make_shared<FontPipeline>(*device_, std::move(*family), options);
            cache_[key] = pipeline;
            return pipeline;
        }
        catch (const std::exception& exception) {
            return std::unexpected(
                FontError {
                    .code = FontErrorCode::pipeline_failure,
                    .operation = "font.pipeline",
                    .message = exception.what(),
                }
            );
        }
    }
    void FontPipelineCache::clear() {
        std::lock_guard lock(mutex_);
        cache_.clear();
    }
} // namespace nandina::text
