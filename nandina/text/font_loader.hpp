#ifndef NANDINA_EXPERIMENT_TEXT_FONT_LOADER_HPP
#define NANDINA_EXPERIMENT_TEXT_FONT_LOADER_HPP

#include "../resource/resource_manager.hpp"
#include "font_face.hpp"

#include <expected>
#include <map>
#include <mutex>

namespace nandina::text
{
    enum class FontErrorCode {
        resource_failure,
        invalid_face_index,
        invalid_font,
        duplicate_family,
        duplicate_alias,
        unknown_family,
        invalid_fallback,
        no_matching_face,
        pipeline_failure,
    };

    struct FontError {
        FontErrorCode code;
        std::string operation;
        std::string message;
        std::optional<resource::ResourceError> cause;
    };
    template<typename T>
    using FontResult = std::expected<T, FontError>;

    struct FontFaceKey {
        resource::ResourceId resource_id;
        std::uint32_t face_index = 0;
        auto operator<=>(const FontFaceKey&) const = default;
    };

    class FontLoader {
    public:
        explicit FontLoader(const resource::ResourceManager& resources): resources_(&resources) {}

        [[nodiscard]] auto load(const resource::ResourceKey& key, std::uint32_t face_index = 0)
            -> FontResult<std::shared_ptr<FreeTypeFontFace>>;
        [[nodiscard]] auto load(resource::ResourceId id, std::uint32_t face_index = 0)
            -> FontResult<std::shared_ptr<FreeTypeFontFace>>;
        [[nodiscard]] auto load(resource::ResourceHandle resource, std::uint32_t face_index = 0)
            -> FontResult<std::shared_ptr<FreeTypeFontFace>>;

        void invalidate(resource::ResourceId id);
        void clear();

    private:
        const resource::ResourceManager* resources_;
        std::mutex mutex_;
        std::map<FontFaceKey, std::weak_ptr<FreeTypeFontFace>> cache_;
    };
} // namespace nandina::text
#endif
