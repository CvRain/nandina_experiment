//
// theme/style_document — styles.toml data compiled into the runtime theme objects.
//

#ifndef NANDINA_EXPERIMENT_THEME_STYLE_DOCUMENT_HPP
#define NANDINA_EXPERIMENT_THEME_STYLE_DOCUMENT_HPP

#include "../text/font_family.hpp"
#include "theme_manager.hpp"

#include <expected>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::theme
{
    struct FontFamilyDeclaration {
        resource::ResourceKey family;
        std::vector<text::FontFaceSpec> faces;
        std::vector<resource::ResourceKey> fallbacks;
        bool is_default = false;
    };

    struct StyleDocument {
        std::map<std::string, NanTheme, std::less<>> themes;
        std::string active_theme = "default";
        std::shared_ptr<NanStyle> style = std::make_shared<NanStyle>();
        std::vector<FontFamilyDeclaration> font_families;

        [[nodiscard]] auto
        apply(ThemeManager& manager, text::FontFamilyRegistry* fonts = nullptr) const
            -> std::expected<void, std::string>;
    };

    [[nodiscard]] auto
    parse_style_document(std::string_view source, std::string_view source_name = "styles.toml")
        -> std::expected<StyleDocument, std::string>;

    [[nodiscard]] auto load_style_document(const std::filesystem::path& path)
        -> std::expected<StyleDocument, std::string>;

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_STYLE_DOCUMENT_HPP
