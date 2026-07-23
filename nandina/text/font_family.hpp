#ifndef NANDINA_EXPERIMENT_TEXT_FONT_FAMILY_HPP
#define NANDINA_EXPERIMENT_TEXT_FONT_FAMILY_HPP

#include "../resource/backends/builtin_backend.hpp"
#include "font_loader.hpp"

#include <map>
#include <set>
#include <shared_mutex>

namespace nandina::text
{
    inline constexpr std::string_view builtin_default_font_family_key = "families/default-ui";

    enum class FontSlant { normal, italic, oblique };

    struct FontFaceSpec {
        resource::ResourceKey resource;
        std::uint32_t face_index = 0;
        int weight = 400;
        FontSlant slant = FontSlant::normal;
    };

    struct FontRequest {
        std::optional<resource::ResourceKey> family;
        int weight = 400;

        FontSlant slant = FontSlant::normal;

        auto operator<=>(const FontRequest&) const = default;
    };

    struct ResolvedFontFamily {
        std::vector<FontFaceSpec> specs;
        std::vector<std::shared_ptr<FreeTypeFontFace>> faces;
    };

    class FontFamilyRegistry {
    public:
        [[nodiscard]] auto
        register_family(resource::ResourceKey family, std::vector<FontFaceSpec> faces)
            -> FontResult<void>;
        [[nodiscard]] auto add_alias(resource::ResourceKey alias, resource::ResourceKey family)
            -> FontResult<void>;
        [[nodiscard]] auto
        set_fallbacks(resource::ResourceKey family, std::vector<resource::ResourceKey> fallbacks)
            -> FontResult<void>;
        [[nodiscard]] auto set_default_family(resource::ResourceKey family) -> FontResult<void>;
        [[nodiscard]] auto set_default_fallbacks(std::vector<resource::ResourceKey> families)
            -> FontResult<void>;
        [[nodiscard]] auto resolve(const FontRequest& request, FontLoader& loader) const
            -> FontResult<ResolvedFontFamily>;

    private:
        struct Family {
            std::vector<FontFaceSpec> faces;
            std::vector<resource::ResourceKey> fallbacks;
        };
        mutable std::shared_mutex mutex_;
        std::map<resource::ResourceKey, Family> families_;
        std::map<resource::ResourceKey, resource::ResourceKey> aliases_;
        std::optional<resource::ResourceKey> default_family_;
        std::vector<resource::ResourceKey> default_fallbacks_;
    };

    [[nodiscard]] auto register_builtin_default_font_family(FontFamilyRegistry& registry)
        -> FontResult<void>;
    [[nodiscard]] auto register_optional_font_fallback(
        FontFamilyRegistry& registry,
        resource::ResourceManager& resources,
        resource::ResourceKey family,
        resource::ResourceKey font
    ) -> FontResult<bool>;
} // namespace nandina::text
#endif
