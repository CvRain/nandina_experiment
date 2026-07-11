//
// text/glyph_run_renderer — draw shaped layout lines through a glyph atlas.
//

#ifndef NANDINA_EXPERIMENT_TEXT_GLYPH_RUN_RENDERER_HPP
#define NANDINA_EXPERIMENT_TEXT_GLYPH_RUN_RENDERER_HPP

#include "../widget/primitives/text_layout.hpp"
#include "glyph_atlas.hpp"

namespace nandina::text
{

    class GlyphRunRenderer {
    public:
        GlyphRunRenderer(GlyphAtlas& atlas, GlyphAtlasTexture& texture);

        void draw_line(
            const widget::primitives::TextLayoutLine& line,
            foundation::NanPoint baseline_origin,
            foundation::NanColor color,
            float pixel_size
        );

    private:
        GlyphAtlas& atlas_;
        GlyphAtlasTexture& texture_;
    };

} // namespace nandina::text

#endif // NANDINA_EXPERIMENT_TEXT_GLYPH_RUN_RENDERER_HPP
