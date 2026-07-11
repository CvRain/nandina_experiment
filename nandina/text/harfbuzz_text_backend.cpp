//
// text/harfbuzz_text_backend — HarfBuzz shaping into TextLayoutResult glyph runs.
//

#include "harfbuzz_text_backend.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb-ft.h>
#include <hb.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace nandina::text
{
    namespace
    {
        [[nodiscard]] auto pixels(hb_position_t value) -> float {
            return static_cast<float>(value) / 64.0F;
        }
    } // namespace

    class HarfBuzzTextLayoutBackend::Impl {
    public:
        explicit Impl(std::shared_ptr<FreeTypeFontFace> value): face(std::move(value)) {
            if (!face) {
                throw std::invalid_argument("HarfBuzzTextLayoutBackend requires a font face");
            }
            font = hb_ft_font_create_referenced(static_cast<FT_Face>(face->native_face_handle()));
            if (font == nullptr) {
                throw std::runtime_error("Failed to create HarfBuzz font from FreeType face");
            }
        }

        ~Impl() {
            hb_font_destroy(font);
        }

        [[nodiscard]] auto shape_line(
            std::string_view text,
            std::size_t source_offset,
            float pixel_size,
            float line_height,
            float baseline
        ) const -> widget::primitives::TextLayoutLine {
            if (text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                throw std::length_error("Text line is too large for HarfBuzz");
            }

            (void)face->metrics(pixel_size);
            hb_ft_font_changed(font);

            const std::unique_ptr<hb_buffer_t, decltype(&hb_buffer_destroy)> buffer(
                hb_buffer_create(),
                &hb_buffer_destroy
            );
            hb_buffer_add_utf8(
                buffer.get(),
                text.data(),
                static_cast<int>(text.size()),
                0,
                static_cast<int>(text.size())
            );
            hb_buffer_guess_segment_properties(buffer.get());
            hb_shape(font, buffer.get(), nullptr, 0);

            unsigned int glyph_count = 0;
            const auto* infos = hb_buffer_get_glyph_infos(buffer.get(), &glyph_count);
            const auto* positions = hb_buffer_get_glyph_positions(buffer.get(), &glyph_count);

            widget::primitives::TextLayoutLine line {
                .text_offset = source_offset,
                .text_length = text.size(),
                .visible_text = std::string(text),
                .size = foundation::NanSize(0.0F, line_height),
                .baseline = baseline,
            };
            line.glyphs.reserve(glyph_count);

            float width = 0.0F;
            for (unsigned int index = 0; index < glyph_count; ++index) {
                const float advance = pixels(positions[index].x_advance);
                line.glyphs.push_back(widget::primitives::TextLayoutLine::Glyph {
                    .glyph_index = infos[index].codepoint,
                    .cluster = source_offset + infos[index].cluster,
                    .x_advance = advance,
                    .y_advance = pixels(positions[index].y_advance),
                    .x_offset = pixels(positions[index].x_offset),
                    .y_offset = pixels(positions[index].y_offset),
                });
                width += advance;
            }
            line.size = foundation::NanSize(std::abs(width), line_height);
            return line;
        }

        std::shared_ptr<FreeTypeFontFace> face;
        hb_font_t* font = nullptr;
    };

    HarfBuzzTextLayoutBackend::HarfBuzzTextLayoutBackend(std::shared_ptr<FreeTypeFontFace> face):
        impl_(std::make_unique<Impl>(std::move(face))) {}

    HarfBuzzTextLayoutBackend::~HarfBuzzTextLayoutBackend() = default;

    auto HarfBuzzTextLayoutBackend::layout(widget::primitives::TextLayoutInput input) const
        -> widget::primitives::TextLayoutResult {
        const auto metrics = impl_->face->metrics(input.style.font_size);
        const float line_height = metrics.line_height > 0.0F
            ? metrics.line_height
            : input.style.font_size * 1.2F;
        const float baseline = metrics.ascender;
        const auto max_lines = static_cast<std::size_t>(std::max(1, input.style.max_lines));

        widget::primitives::TextLayoutResult result;
        result.font_size = input.style.font_size;
        result.baseline = baseline;

        std::size_t offset = 0;
        bool pending_empty_line = input.text.empty();
        while ((offset < input.text.size() || pending_empty_line) && result.lines.size() < max_lines) {
            const auto newline = input.text.find('\n', offset);
            const auto end = newline != std::string_view::npos ? newline : input.text.size();
            result.lines.push_back(impl_->shape_line(
                input.text.substr(offset, end - offset),
                offset,
                input.style.font_size,
                line_height,
                baseline
            ));
            pending_empty_line = false;
            offset = end;
            if (offset < input.text.size() && input.text[offset] == '\n') {
                ++offset;
                pending_empty_line = offset == input.text.size();
            }
        }

        float width = 0.0F;
        for (const auto& line: result.lines) {
            width = std::max(width, line.size.get_width());
        }
        if (std::isfinite(input.constraints.max_width)
            && width > input.constraints.max_width)
        {
            result.overflowed = true;
        }
        result.overflowed = result.overflowed || offset < input.text.size() || pending_empty_line;
        result.size = input.constraints.constrain(foundation::NanSize(
            width,
            line_height * static_cast<float>(result.lines.size())
        ));
        return result;
    }

    auto HarfBuzzTextLayoutBackend::font_face() const
        -> const std::shared_ptr<FreeTypeFontFace>& {
        return impl_->face;
    }

} // namespace nandina::text
