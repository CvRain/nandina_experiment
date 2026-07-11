//
// FreeType font-face integration tests.
//

#include "text/font_face.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <stdexcept>

using namespace nandina;

TEST_CASE("FreeTypeFontFace reports invalid font paths", "[text][freetype]") {
    REQUIRE_THROWS_AS(
        text::FreeTypeFontFace(std::filesystem::path("missing-nandina-font.ttf")),
        std::runtime_error
    );
}

TEST_CASE("FreeTypeFontFace exposes metrics and grayscale glyphs", "[text][freetype]") {
    text::FreeTypeFontFace face(std::filesystem::path(NANDINA_TEST_FONT_PATH));

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
