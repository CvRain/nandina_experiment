//
// Meson bundled-font copy integration tests.
//

#include "text/font_face.hpp"
#include "text/glyph_atlas.hpp"
#include "text/harfbuzz_text_backend.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <memory>

using namespace nandina;

TEST_CASE("Meson copies and FreeType loads the bundled default font", "[text][font][resource]") {
    const std::filesystem::path font_path(NANDINA_BUNDLED_FONT_PATH);

    REQUIRE(std::filesystem::is_regular_file(font_path));
    auto face = std::make_shared<text::FreeTypeFontFace>(font_path);
    REQUIRE(face->family_name() == "Caskaydia Cove");
    REQUIRE(face->glyph_index(U'A') != 0);
    REQUIRE(face->glyph_index(U'=') != 0);

    text::HarfBuzzTextLayoutBackend backend(face);
    const auto layout = backend.layout(widget::primitives::TextLayoutInput {
        .text = "if (a == b) -> {}",
        .style = widget::primitives::TextStyle {.font_size = 24.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });
    REQUIRE_FALSE(layout.lines.empty());
    REQUIRE_FALSE(layout.lines.front().glyphs.empty());

    text::GlyphAtlas atlas(face, 256, 256);
    for (const auto& glyph: layout.lines.front().glyphs) {
        (void)atlas.cache_glyph(glyph.glyph_index, layout.font_size);
    }
    REQUIRE(atlas.revision() > 0);
}

TEST_CASE("HarfBuzz overflow preserves shaping clusters", "[text][harfbuzz][overflow]") {
    auto face = std::make_shared<text::FreeTypeFontFace>(
        std::filesystem::path(NANDINA_BUNDLED_FONT_PATH)
    );
    text::HarfBuzzTextLayoutBackend backend(face);
    constexpr std::string_view source = "abcdef";

    const auto natural = backend.layout(widget::primitives::TextLayoutInput {
        .text = source,
        .style = widget::primitives::TextStyle {.font_size = 24.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });
    const float width_limit = natural.size.get_width() * 0.55F;

    const auto clipped = backend.layout(widget::primitives::TextLayoutInput {
        .text = source,
        .style = widget::primitives::TextStyle {
            .font_size = 24.0F,
            .overflow = widget::primitives::TextOverflow::clip,
            .max_lines = 1,
        },
        .constraints = scene::LayoutConstraints {
            .min_width = 0.0F,
            .max_width = width_limit,
            .min_height = 0.0F,
            .max_height = 100.0F,
        },
    });
    REQUIRE(clipped.overflowed);
    REQUIRE(clipped.lines.front().text_length == source.size());
    REQUIRE(clipped.lines.front().visible_text == source);
    REQUIRE(clipped.lines.front().size.get_width() > width_limit);
    REQUIRE(clipped.size.get_width() <= width_limit);

    const auto ellipsis = backend.layout(widget::primitives::TextLayoutInput {
        .text = source,
        .style = widget::primitives::TextStyle {
            .font_size = 24.0F,
            .overflow = widget::primitives::TextOverflow::ellipsis,
            .max_lines = 1,
        },
        .constraints = scene::LayoutConstraints {
            .min_width = 0.0F,
            .max_width = width_limit,
            .min_height = 0.0F,
            .max_height = 100.0F,
        },
    });
    REQUIRE(ellipsis.overflowed);
    REQUIRE(ellipsis.lines.front().visible_text.ends_with("..."));
    REQUIRE(ellipsis.lines.front().text_length < source.size());
    REQUIRE(ellipsis.lines.front().size.get_width() <= width_limit);

    const auto wrapped = backend.layout(widget::primitives::TextLayoutInput {
        .text = source,
        .style = widget::primitives::TextStyle {
            .font_size = 24.0F,
            .overflow = widget::primitives::TextOverflow::wrap,
            .max_lines = 4,
        },
        .constraints = scene::LayoutConstraints {
            .min_width = 0.0F,
            .max_width = width_limit,
            .min_height = 0.0F,
            .max_height = 200.0F,
        },
    });
    REQUIRE(wrapped.lines.size() > 1);
    std::size_t consumed = 0;
    for (const auto& line: wrapped.lines) {
        REQUIRE(line.text_offset == consumed);
        REQUIRE(line.size.get_width() <= width_limit);
        consumed += line.text_length;
    }
    REQUIRE(consumed == source.size());
    REQUIRE_FALSE(wrapped.overflowed);

    constexpr std::string_view combined = "a\xCC\x81" "b";
    const auto combined_natural = backend.layout(widget::primitives::TextLayoutInput {
        .text = combined,
        .style = widget::primitives::TextStyle {.font_size = 24.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });
    const auto combined_clip = backend.layout(widget::primitives::TextLayoutInput {
        .text = combined,
        .style = widget::primitives::TextStyle {
            .font_size = 24.0F,
            .overflow = widget::primitives::TextOverflow::clip,
            .max_lines = 1,
        },
        .constraints = scene::LayoutConstraints {
            .min_width = 0.0F,
            .max_width = combined_natural.size.get_width() * 0.75F,
            .min_height = 0.0F,
            .max_height = 100.0F,
        },
    });
    REQUIRE(combined_clip.lines.front().text_length == combined.size());
    REQUIRE(combined_clip.lines.front().visible_text == combined);
}

TEST_CASE("HarfBuzz scale reshapes text at an effective font size", "[text][harfbuzz][scale]") {
    auto face = std::make_shared<text::FreeTypeFontFace>(
        std::filesystem::path(NANDINA_BUNDLED_FONT_PATH)
    );
    text::HarfBuzzTextLayoutBackend backend(face);
    constexpr std::string_view source = "Scale the complete shaped line";

    const auto natural = backend.layout(widget::primitives::TextLayoutInput {
        .text = source,
        .style = widget::primitives::TextStyle {.font_size = 32.0F},
        .constraints = scene::LayoutConstraints::loose(),
    });
    const float width_limit = natural.size.get_width() * 0.5F;

    const auto scaled = backend.layout(widget::primitives::TextLayoutInput {
        .text = source,
        .style = widget::primitives::TextStyle {
            .font_size = 32.0F,
            .overflow = widget::primitives::TextOverflow::scale,
            .max_lines = 1,
        },
        .constraints = scene::LayoutConstraints {
            .min_width = 0.0F,
            .max_width = width_limit,
            .min_height = 0.0F,
            .max_height = 100.0F,
        },
    });

    REQUIRE(scaled.font_size < 32.0F);
    REQUIRE(scaled.font_size >= 1.0F);
    REQUIRE(scaled.lines.size() == 1);
    REQUIRE(scaled.lines.front().text_length == source.size());
    REQUIRE(scaled.lines.front().visible_text == source);
    REQUIRE_FALSE(scaled.lines.front().glyphs.empty());
    REQUIRE(scaled.lines.front().size.get_width() <= width_limit + 0.01F);
    REQUIRE(scaled.size.get_width() <= width_limit + 0.01F);
    REQUIRE(scaled.baseline < natural.baseline);
    REQUIRE(scaled.size.get_height() < natural.size.get_height());
    REQUIRE_FALSE(scaled.overflowed);

    const auto zero_width = backend.layout(widget::primitives::TextLayoutInput {
        .text = source,
        .style = widget::primitives::TextStyle {
            .font_size = 32.0F,
            .overflow = widget::primitives::TextOverflow::scale,
            .max_lines = 1,
        },
        .constraints = scene::LayoutConstraints {
            .min_width = 0.0F,
            .max_width = 0.0F,
            .min_height = 0.0F,
            .max_height = 100.0F,
        },
    });
    REQUIRE(zero_width.font_size == 1.0F);
    REQUIRE(zero_width.overflowed);
}
