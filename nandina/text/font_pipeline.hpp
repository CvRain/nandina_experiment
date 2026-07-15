#ifndef NANDINA_EXPERIMENT_TEXT_FONT_PIPELINE_HPP
#define NANDINA_EXPERIMENT_TEXT_FONT_PIPELINE_HPP

#include "font_family.hpp"
#include "glyph_atlas.hpp"
#include "glyph_run_renderer.hpp"

#include <map>
#include <mutex>

namespace nandina::text
{
    struct FontPipelineOptions {
        int atlas_width = 1024;
        int atlas_height = 1024;
        int atlas_padding = 1;
        auto operator<=>(const FontPipelineOptions&) const = default;
    };

    class FontPipeline final {
    public:
        FontPipeline(
            render::IRenderDevice& device,
            ResolvedFontFamily family,
            FontPipelineOptions options = {}
        );

        [[nodiscard]] auto pipeline() const -> widget::primitives::TextPipeline;
        [[nodiscard]] auto backend() const -> const HarfBuzzTextLayoutBackend&;
        [[nodiscard]] auto font_count() const -> std::size_t;

    private:
        std::vector<std::shared_ptr<FreeTypeFontFace>> faces_;
        std::unique_ptr<HarfBuzzTextLayoutBackend> backend_;
        std::vector<std::unique_ptr<GlyphAtlas>> atlases_;
        std::vector<std::unique_ptr<GlyphAtlasTexture>> textures_;
        std::vector<GlyphAtlasBinding> bindings_;
        std::unique_ptr<GlyphRunRenderer> renderer_;
    };

    class FontPipelineCache final {
    public:
        FontPipelineCache(
            render::IRenderDevice& device,
            FontLoader& loader,
            const FontFamilyRegistry& families
        );

        [[nodiscard]] auto get(FontRequest request, FontPipelineOptions options = {})
            -> FontResult<std::shared_ptr<FontPipeline>>;
        void clear();

    private:
        struct Key {
            std::optional<resource::ResourceKey> family;
            int weight = 400;
            FontSlant slant = FontSlant::normal;
            FontPipelineOptions options;
            auto operator<=>(const Key&) const = default;
        };

        render::IRenderDevice* device_;
        FontLoader* loader_;
        const FontFamilyRegistry* families_;
        std::mutex mutex_;
        std::map<Key, std::weak_ptr<FontPipeline>> cache_;
    };
} // namespace nandina::text
#endif
