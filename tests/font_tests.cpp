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
#include <array>
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

    [[nodiscard]] auto rtl_test_font_path() -> std::filesystem::path {
        return std::filesystem::path(NANDINA_RTL_TEST_FONT_PATH);
    }

    [[nodiscard]] auto arabic_mark_test_font_path() -> std::filesystem::path {
        return std::filesystem::path(NANDINA_ARABIC_MARK_TEST_FONT_PATH);
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

TEST_CASE("HarfBuzz caret geometry follows shaped graphemes", "[text][harfbuzz][caret]") {
    auto face = std::make_shared<text::FreeTypeFontFace>(test_font_path());
    text::HarfBuzzTextLayoutBackend backend(face);
    constexpr std::string_view source = "a\xCC\x81" "b";
    const auto layout = backend.layout(widget::primitives::TextLayoutInput {
        .text = source,
        .style = widget::primitives::TextStyle {.font_size = 24.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });

    const auto& line = layout.lines.front();
    REQUIRE(line.caret_stops.size() == 3);
    REQUIRE(line.caret_stops[0].source_offset == 0);
    REQUIRE(line.caret_stops[1].source_offset == 3);
    REQUIRE(line.caret_stops[2].source_offset == source.size());
    REQUIRE(line.caret_for_source(2).source_offset == 0);
    REQUIRE(line.caret_for_source(source.size()).x == Catch::Approx(line.size.get_width()));
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

    int expected_draws = 0;
    for (const auto& glyph: layout.lines.front().glyphs) {
        const auto bitmap = backend.font_face(glyph.font_index)->rasterize_glyph(
            glyph.glyph_index,
            layout.font_size
        );
        expected_draws += bitmap.width > 0 && bitmap.height > 0 ? 1 : 0;
    }
    REQUIRE(device.draws == expected_draws);
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

TEST_CASE("FriBidi orders pure RTL and mixed-direction glyph runs", "[text][harfbuzz][bidi]") {
    auto face = std::make_shared<text::FreeTypeFontFace>(rtl_test_font_path());
    text::HarfBuzzTextLayoutBackend backend(face);

    const auto rtl = backend.layout(widget::primitives::TextLayoutInput {
        .text = "لسان",
        .style = widget::primitives::TextStyle {.font_size = 32.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });
    REQUIRE(rtl.lines.size() == 1);
    REQUIRE(rtl.lines.front().right_to_left);
    REQUIRE(rtl.lines.front().glyphs.size() >= 2);
    REQUIRE(rtl.lines.front().glyphs.front().cluster
            > rtl.lines.front().glyphs.back().cluster);

    const auto mixed = backend.layout(widget::primitives::TextLayoutInput {
        .text = "abc لسان",
        .style = widget::primitives::TextStyle {.font_size = 32.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });
    REQUIRE_FALSE(mixed.lines.front().right_to_left);
    bool has_visual_cluster_inversion = false;
    for (std::size_t index = 1; index < mixed.lines.front().glyphs.size(); ++index) {
        has_visual_cluster_inversion = has_visual_cluster_inversion
            || mixed.lines.front().glyphs[index - 1].cluster
                > mixed.lines.front().glyphs[index].cluster;
    }
    REQUIRE(has_visual_cluster_inversion);

    const auto& rtl_line = rtl.lines.front();
    REQUIRE(rtl_line.caret_for_source(0).x == Catch::Approx(rtl_line.size.get_width()));
    REQUIRE(rtl_line.caret_for_source(std::string_view("لسان").size()).x
            == Catch::Approx(0.0F));

    bool has_split_bidi_boundary = false;
    const auto& mixed_stops = mixed.lines.front().caret_stops;
    for (std::size_t left = 0; left < mixed_stops.size(); ++left) {
        for (std::size_t right = left + 1; right < mixed_stops.size(); ++right) {
            has_split_bidi_boundary = has_split_bidi_boundary
                || (mixed_stops[left].source_offset == mixed_stops[right].source_offset
                    && std::abs(mixed_stops[left].x - mixed_stops[right].x) > 0.01F);
        }
    }
    REQUIRE(has_split_bidi_boundary);
}

TEST_CASE("FriBidi reshapes RTL overflow at logical cluster boundaries", "[text][harfbuzz][bidi][overflow]") {
    auto face = std::make_shared<text::FreeTypeFontFace>(rtl_test_font_path());
    text::HarfBuzzTextLayoutBackend backend(face);
    constexpr std::string_view source = "لسان لسان";

    const auto natural = backend.layout(widget::primitives::TextLayoutInput {
        .text = source,
        .style = widget::primitives::TextStyle {.font_size = 32.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });
    const float width_limit = natural.size.get_width() * 0.55F;
    const scene::LayoutConstraints constraints {
        .min_width = 0.0F,
        .max_width = width_limit,
        .min_height = 0.0F,
        .max_height = 200.0F,
    };

    const auto wrapped = backend.layout(widget::primitives::TextLayoutInput {
        .text = source,
        .style = widget::primitives::TextStyle {
            .font_size = 32.0F,
            .overflow = widget::primitives::TextOverflow::wrap,
            .max_lines = 4,
        },
        .constraints = constraints,
    });
    REQUIRE(wrapped.lines.size() > 1);
    std::size_t consumed = 0;
    for (const auto& line: wrapped.lines) {
        REQUIRE(line.right_to_left);
        REQUIRE(line.text_offset == consumed);
        REQUIRE(line.size.get_width() <= width_limit + 0.01F);
        consumed += line.text_length;
    }
    REQUIRE(consumed == source.size());
    REQUIRE_FALSE(wrapped.overflowed);

    const auto clipped = backend.layout(widget::primitives::TextLayoutInput {
        .text = source,
        .style = widget::primitives::TextStyle {
            .font_size = 32.0F,
            .overflow = widget::primitives::TextOverflow::clip,
            .max_lines = 1,
        },
        .constraints = constraints,
    });
    REQUIRE(clipped.overflowed);
    REQUIRE(clipped.lines.front().right_to_left);
    REQUIRE(clipped.lines.front().text_length == source.size());
    REQUIRE(clipped.lines.front().visible_text == source);
    REQUIRE(clipped.lines.front().size.get_width() > width_limit);
    REQUIRE(clipped.size.get_width() <= width_limit + 0.01F);

    const auto ellipsis = backend.layout(widget::primitives::TextLayoutInput {
        .text = source,
        .style = widget::primitives::TextStyle {
            .font_size = 32.0F,
            .overflow = widget::primitives::TextOverflow::ellipsis,
            .max_lines = 1,
        },
        .constraints = constraints,
    });
    REQUIRE(ellipsis.overflowed);
    REQUIRE(ellipsis.lines.front().right_to_left);
    REQUIRE((ellipsis.lines.front().visible_text.empty()
             || ellipsis.lines.front().visible_text.ends_with("...")));
    REQUIRE(ellipsis.lines.front().size.get_width() <= width_limit + 0.01F);

    const auto rtl_with_numbers = backend.layout(widget::primitives::TextLayoutInput {
        .text = "لسان 123456789",
        .style = widget::primitives::TextStyle {
            .font_size = 32.0F,
            .overflow = widget::primitives::TextOverflow::wrap,
            .max_lines = 12,
        },
        .constraints = scene::LayoutConstraints {
            .min_width = 0.0F,
            .max_width = width_limit * 0.7F,
            .min_height = 0.0F,
            .max_height = 500.0F,
        },
    });
    bool found_numeric_continuation = false;
    for (const auto& line: rtl_with_numbers.lines) {
        if (!line.visible_text.empty() && line.visible_text.front() >= '0'
            && line.visible_text.front() <= '9')
        {
            REQUIRE(line.right_to_left);
            found_numeric_continuation = true;
        }
    }
    REQUIRE(found_numeric_continuation);
}

TEST_CASE("HarfBuzz selects fallback fonts per shaping cluster", "[text][harfbuzz][fallback]") {
    auto primary = std::make_shared<text::FreeTypeFontFace>(test_font_path());
    auto arabic = std::make_shared<text::FreeTypeFontFace>(rtl_test_font_path());
    text::HarfBuzzTextLayoutBackend backend(primary, {arabic});

    const auto layout = backend.layout(widget::primitives::TextLayoutInput {
        .text = "abلسان",
        .style = widget::primitives::TextStyle {.font_size = 32.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });

    REQUIRE(backend.font_count() == 2);
    REQUIRE(backend.font_face(0) == primary);
    REQUIRE(backend.font_face(1) == arabic);
    bool used_primary = false;
    bool used_fallback = false;
    for (const auto& glyph: layout.lines.front().glyphs) {
        REQUIRE(glyph.glyph_index != 0);
        used_primary = used_primary || glyph.font_index == 0;
        used_fallback = used_fallback || glyph.font_index == 1;
    }
    REQUIRE(used_primary);
    REQUIRE(used_fallback);
    REQUIRE_FALSE(layout.missing_glyphs);
    const auto primary_metrics = primary->metrics(layout.font_size);
    const auto fallback_metrics = arabic->metrics(layout.font_size);
    const float combined_extent = std::max(primary_metrics.ascender, fallback_metrics.ascender)
        - std::min(primary_metrics.descender, fallback_metrics.descender);
    REQUIRE(layout.lines.front().size.get_height() >= combined_extent);

    auto arabic_marks = std::make_shared<text::FreeTypeFontFace>(arabic_mark_test_font_path());
    text::HarfBuzzTextLayoutBackend mark_backend(primary, {arabic_marks});
    const auto combining = mark_backend.layout(widget::primitives::TextLayoutInput {
        .text = "aكِ",
        .style = widget::primitives::TextStyle {.font_size = 32.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });
    bool saw_arabic_grapheme = false;
    for (const auto& glyph: combining.lines.front().glyphs) {
        if (glyph.cluster >= 1 && glyph.cluster < 5) {
            REQUIRE(glyph.font_index == 1);
            REQUIRE(glyph.glyph_index != 0);
            saw_arabic_grapheme = true;
        }
    }
    REQUIRE(saw_arabic_grapheme);

    const auto variation = backend.layout(widget::primitives::TextLayoutInput {
        .text = "a\xEF\xB8\x8F",
        .style = widget::primitives::TextStyle {.font_size = 32.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });
    REQUIRE_FALSE(variation.lines.front().glyphs.empty());
    REQUIRE(variation.lines.front().glyphs.front().font_index == 0);
    REQUIRE(variation.missing_glyphs);

    text::HarfBuzzTextLayoutBackend no_fallback(primary);
    const auto missing = no_fallback.layout(widget::primitives::TextLayoutInput {
        .text = "لسان",
        .style = widget::primitives::TextStyle {.font_size = 32.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });
    REQUIRE(missing.missing_glyphs);
}

TEST_CASE("GlyphRunRenderer routes fallback glyphs to matching atlases", "[text][harfbuzz][fallback][render]") {
    auto primary = std::make_shared<text::FreeTypeFontFace>(test_font_path());
    auto arabic = std::make_shared<text::FreeTypeFontFace>(rtl_test_font_path());
    text::HarfBuzzTextLayoutBackend backend(primary, {arabic});
    const auto layout = backend.layout(widget::primitives::TextLayoutInput {
        .text = "abلسان",
        .style = widget::primitives::TextStyle {.font_size = 32.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });

    text::GlyphAtlas primary_atlas(primary, 128, 128);
    text::GlyphAtlas fallback_atlas(arabic, 128, 128);
    TextureRecordingDevice device;
    text::GlyphAtlasTexture primary_texture(device, primary_atlas);
    text::GlyphAtlasTexture fallback_texture(device, fallback_atlas);
    const std::array bindings {
        text::GlyphAtlasBinding {.atlas = &primary_atlas, .texture = &primary_texture},
        text::GlyphAtlasBinding {.atlas = &fallback_atlas, .texture = &fallback_texture},
    };
    const std::array swapped_bindings {
        text::GlyphAtlasBinding {.atlas = &fallback_atlas, .texture = &fallback_texture},
        text::GlyphAtlasBinding {.atlas = &primary_atlas, .texture = &primary_texture},
    };
    REQUIRE_THROWS_AS(
        text::GlyphRunRenderer(backend, swapped_bindings),
        std::invalid_argument
    );
    text::GlyphRunRenderer renderer(backend, bindings);

    renderer.draw_line(
        layout.lines.front(),
        foundation::NanPoint(0.0F, layout.baseline),
        foundation::NanColor::from(foundation::NanOklch {}),
        layout.font_size
    );

    int expected_draws = 0;
    for (const auto& glyph: layout.lines.front().glyphs) {
        const auto bitmap = backend.font_face(glyph.font_index)->rasterize_glyph(
            glyph.glyph_index,
            layout.font_size
        );
        expected_draws += bitmap.width > 0 && bitmap.height > 0 ? 1 : 0;
    }
    REQUIRE(device.draws == expected_draws);
    REQUIRE(primary_atlas.revision() > 0);
    REQUIRE(fallback_atlas.revision() > 0);
    REQUIRE(primary_texture.uploaded_revision() == primary_atlas.revision());
    REQUIRE(fallback_texture.uploaded_revision() == fallback_atlas.revision());
}

TEST_CASE("GlyphRunRenderer rejects mismatched atlas texture bindings", "[text][fallback][render]") {
    auto face = std::make_shared<text::FreeTypeFontFace>(test_font_path());
    text::HarfBuzzTextLayoutBackend backend(face);
    text::GlyphAtlas first(face, 64, 64);
    text::GlyphAtlas second(face, 64, 64);
    TextureRecordingDevice device;
    text::GlyphAtlasTexture texture(device, first);
    const std::array bindings {
        text::GlyphAtlasBinding {.atlas = &second, .texture = &texture},
    };

    REQUIRE_THROWS_AS(text::GlyphRunRenderer(backend, bindings), std::invalid_argument);
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
