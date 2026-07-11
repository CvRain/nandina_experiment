//
// text/glyph_run_renderer — draw shaped layout lines through a glyph atlas.
//

#include "glyph_run_renderer.hpp"

#include <vector>

namespace nandina::text
{

    GlyphRunRenderer::GlyphRunRenderer(GlyphAtlas& atlas, GlyphAtlasTexture& texture):
        atlas_(atlas),
        texture_(texture) {}

    void GlyphRunRenderer::draw_line(
        const widget::primitives::TextLayoutLine& line,
        foundation::NanPoint baseline_origin,
        foundation::NanColor color,
        float pixel_size
    ) {
        std::vector<const GlyphAtlasEntry*> glyphs;
        glyphs.reserve(line.glyphs.size());
        for (const auto& shaped: line.glyphs) {
            glyphs.push_back(&atlas_.cache_glyph(shaped.glyph_index, pixel_size));
        }
        texture_.sync();

        float pen_x = baseline_origin.get_x();
        float pen_y = baseline_origin.get_y();
        for (std::size_t index = 0; index < line.glyphs.size(); ++index) {
            const auto& shaped = line.glyphs[index];
            texture_.draw(
                *glyphs[index],
                foundation::NanPoint(
                    pen_x + shaped.x_offset,
                    pen_y - shaped.y_offset
                ),
                color
            );
            pen_x += shaped.x_advance;
            pen_y -= shaped.y_advance;
        }
    }

} // namespace nandina::text
