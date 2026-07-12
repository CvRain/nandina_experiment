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
