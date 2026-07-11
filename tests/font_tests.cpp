//
// FreeType font-face integration tests.
//

#include "text/font_face.hpp"
#include "text/glyph_atlas.hpp"
#include "text/glyph_run_renderer.hpp"
#include "text/harfbuzz_text_backend.hpp"
#include "scene/scene_tree.hpp"
#include "widget/primitives/text.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <vector>

using namespace nandina;

namespace
{
    class TextureRecordingDevice final: public render::IRenderDevice {
    public:
        int creates = 0;
        int updates = 0;
        int destroys = 0;
        int draws = 0;
        int text_draws = 0;
        foundation::NanRect last_source;
        foundation::NanRect last_destination;

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {}
        void draw_rect_outline(
            const foundation::NanRect&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_rounded_rect(
            const foundation::NanRect&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_circle(
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {
            ++text_draws;
        }

        [[nodiscard]] auto supports_alpha_textures() const -> bool override {
            return true;
        }

        [[nodiscard]] auto create_alpha_texture(
            int width,
            int height,
            std::span<const std::uint8_t> alpha
        ) -> render::TextureHandle override {
            REQUIRE(alpha.size() == static_cast<std::size_t>(width * height));
            ++creates;
            return {.value = 1};
        }

        void update_alpha_texture(
            render::TextureHandle,
            int width,
            int height,
            std::span<const std::uint8_t> alpha
        ) override {
            REQUIRE(alpha.size() == static_cast<std::size_t>(width * height));
            ++updates;
        }

        void destroy_texture(render::TextureHandle) override {
            ++destroys;
        }

        void draw_texture_region(
            render::TextureHandle,
            const foundation::NanRect& source,
            const foundation::NanRect& destination,
            const foundation::NanColor&
        ) override {
            ++draws;
            last_source = source;
            last_destination = destination;
        }
    };

    [[nodiscard]] auto test_font_path() -> std::filesystem::path {
        return std::filesystem::path(NANDINA_TEST_FONT_PATH);
    }
} // namespace

TEST_CASE("FreeTypeFontFace reports invalid font paths", "[text][freetype]") {
    REQUIRE_THROWS_AS(
        text::FreeTypeFontFace(std::filesystem::path("missing-nandina-font.ttf")),
        std::runtime_error
    );
}

TEST_CASE("FreeTypeFontFace exposes metrics and grayscale glyphs", "[text][freetype]") {
    text::FreeTypeFontFace face(test_font_path());

    REQUIRE_FALSE(face.family_name().empty());
    REQUIRE(face.glyph_index(U'a') != 0);
    REQUIRE(face.glyph_index(U'中') == 0);

    const auto font_metrics = face.metrics(24.0F);
    REQUIRE(font_metrics.ascender > 0.0F);
    REQUIRE(font_metrics.descender <= 0.0F);
    REQUIRE(font_metrics.line_height > 0.0F);

    const auto glyph_metrics = face.glyph_metrics(U'a', 24.0F);
    REQUIRE(glyph_metrics.glyph_index == face.glyph_index(U'a'));
    REQUIRE(glyph_metrics.advance_x > 0.0F);
    REQUIRE(glyph_metrics.width > 0.0F);
    REQUIRE(glyph_metrics.height > 0.0F);

    const auto bitmap = face.rasterize(U'a', 24.0F);
    REQUIRE(bitmap.metrics.glyph_index == glyph_metrics.glyph_index);
    REQUIRE(bitmap.width > 0);
    REQUIRE(bitmap.height > 0);
    REQUIRE(bitmap.pitch == bitmap.width);
    REQUIRE(bitmap.alpha.size() == static_cast<std::size_t>(bitmap.width * bitmap.height));
}

TEST_CASE("GlyphAtlas caches and packs FreeType glyphs", "[text][atlas]") {
    auto face = std::make_shared<text::FreeTypeFontFace>(test_font_path());
    text::GlyphAtlas atlas(face, 64, 64);

    const auto& first = atlas.cache(U'a', 24.0F);
    const auto first_revision = atlas.revision();
    const auto& cached = atlas.cache(U'a', 24.0F);

    REQUIRE(&first == &cached);
    REQUIRE(first_revision == 1);
    REQUIRE(atlas.revision() == first_revision);
    REQUIRE(first.pixel_bounds.is_valid());
    REQUIRE(first.metrics.advance_x > 0.0F);
    REQUIRE(atlas.find(U'a', 24.0F) == &first);
    REQUIRE(std::ranges::any_of(atlas.pixels(), [](std::uint8_t alpha) { return alpha != 0; }));
}

TEST_CASE("GlyphAtlasTexture uploads revisions and positions glyphs", "[text][atlas][render]") {
    auto face = std::make_shared<text::FreeTypeFontFace>(test_font_path());
    text::GlyphAtlas atlas(face, 64, 64);
    TextureRecordingDevice device;

    {
        text::GlyphAtlasTexture texture(device, atlas);
        REQUIRE(device.creates == 1);
        REQUIRE(texture.uploaded_revision() == 0);

        const auto& glyph = atlas.cache(U'a', 24.0F);
        texture.draw(
            glyph,
            foundation::NanPoint(10.0F, 30.0F),
            foundation::NanColor::from(foundation::NanOklch {})
        );

        REQUIRE(device.updates == 1);
        REQUIRE(device.draws == 1);
        REQUIRE(texture.uploaded_revision() == atlas.revision());
        REQUIRE(device.last_source.get_left() == Catch::Approx(glyph.pixel_bounds.get_left()));
        REQUIRE(device.last_source.get_top() == Catch::Approx(glyph.pixel_bounds.get_top()));
        REQUIRE(device.last_source.get_width() == Catch::Approx(glyph.pixel_bounds.get_width()));
        REQUIRE(device.last_source.get_height() == Catch::Approx(glyph.pixel_bounds.get_height()));
        REQUIRE(device.last_destination.get_left()
                == Catch::Approx(10.0F + glyph.metrics.bearing_x));
        REQUIRE(device.last_destination.get_top()
                == Catch::Approx(30.0F - glyph.metrics.bearing_y));

        texture.sync();
        REQUIRE(device.updates == 1);
    }
    REQUIRE(device.destroys == 1);
}

TEST_CASE("HarfBuzz backend produces glyph runs with source clusters", "[text][harfbuzz]") {
    auto face = std::make_shared<text::FreeTypeFontFace>(test_font_path());
    text::HarfBuzzTextLayoutBackend backend(face);

    const auto layout = backend.layout(widget::primitives::TextLayoutInput {
        .text = "ab",
        .style = widget::primitives::TextStyle {
            .font_size = 24.0F,
            .overflow = widget::primitives::TextOverflow::clip,
            .max_lines = 1,
        },
        .constraints = scene::LayoutConstraints::loose(),
    });

    REQUIRE(layout.lines.size() == 1);
    REQUIRE(layout.lines.front().glyphs.size() == 2);
    REQUIRE(layout.lines.front().glyphs[0].cluster == 0);
    REQUIRE(layout.lines.front().glyphs[1].cluster == 1);
    REQUIRE(layout.lines.front().glyphs[0].glyph_index == face->glyph_index(U'a'));
    REQUIRE(layout.lines.front().glyphs[1].glyph_index == face->glyph_index(U'b'));
    REQUIRE(layout.lines.front().size.get_width() > 0.0F);
    REQUIRE(layout.baseline > 0.0F);
}

TEST_CASE("HarfBuzz glyph runs draw through the atlas texture", "[text][harfbuzz][render]") {
    auto face = std::make_shared<text::FreeTypeFontFace>(test_font_path());
    text::HarfBuzzTextLayoutBackend backend(face);
    const auto layout = backend.layout(widget::primitives::TextLayoutInput {
        .text = "ab",
        .style = widget::primitives::TextStyle {.font_size = 24.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });

    text::GlyphAtlas atlas(face, 64, 64);
    TextureRecordingDevice device;
    text::GlyphAtlasTexture texture(device, atlas);
    text::GlyphRunRenderer renderer(atlas, texture);
    renderer.draw_line(
        layout.lines.front(),
        foundation::NanPoint(10.0F, 30.0F),
        foundation::NanColor::from(foundation::NanOklch {}),
        layout.font_size
    );

    REQUIRE(device.draws == static_cast<int>(layout.lines.front().glyphs.size()));
    REQUIRE(device.updates >= 1);
    REQUIRE(texture.uploaded_revision() == atlas.revision());
}

TEST_CASE("HarfBuzz backend honors explicit line limits", "[text][harfbuzz]") {
    auto face = std::make_shared<text::FreeTypeFontFace>(test_font_path());
    text::HarfBuzzTextLayoutBackend backend(face);

    const auto layout = backend.layout(widget::primitives::TextLayoutInput {
        .text = "a\nb\na",
        .style = widget::primitives::TextStyle {
            .font_size = 24.0F,
            .overflow = widget::primitives::TextOverflow::wrap,
            .max_lines = 2,
        },
        .constraints = scene::LayoutConstraints::loose(),
    });

    REQUIRE(layout.lines.size() == 2);
    REQUIRE(layout.lines[0].text_offset == 0);
    REQUIRE(layout.lines[1].text_offset == 2);
    REQUIRE(layout.overflowed);
}

TEST_CASE("Text draws HarfBuzz layouts through the glyph atlas renderer", "[text][harfbuzz][widget]") {
    auto face = std::make_shared<text::FreeTypeFontFace>(test_font_path());
    text::HarfBuzzTextLayoutBackend backend(face);
    text::GlyphAtlas atlas(face, 64, 64);
    TextureRecordingDevice device;
    text::GlyphAtlasTexture texture(device, atlas);
    text::GlyphRunRenderer renderer(atlas, texture);

    auto value = std::make_shared<widget::primitives::Text>("ab", backend);
    value->set_font_size(24.0F);
    value->set_layout_renderer(&renderer);

    scene::NanSceneTree tree;
    tree.set_root(value);
    tree.draw(device);

    REQUIRE(value->layout_renderer() == &renderer);
    REQUIRE(value->layout_result().lines.front().glyphs.size() == 2);
    REQUIRE(device.draws == 2);
    REQUIRE(device.text_draws == 0);
}
