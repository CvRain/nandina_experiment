//
// render/texture_cache - window-scoped shared image texture residency.
//

#ifndef NANDINA_EXPERIMENT_RENDER_TEXTURE_CACHE_HPP
#define NANDINA_EXPERIMENT_RENDER_TEXTURE_CACHE_HPP

#include "render_device.hpp"

#include <cstddef>
#include <list>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::render
{
    class CachedTexture final {
    public:
        /// The render device must outlive every shared reference to this resource.
        CachedTexture(IRenderDevice& device, TextureHandle handle, NanSize size) noexcept;
        ~CachedTexture();

        CachedTexture(const CachedTexture&) = delete;
        auto operator=(const CachedTexture&) -> CachedTexture& = delete;

        [[nodiscard]] auto handle() const noexcept -> TextureHandle;
        [[nodiscard]] auto size() const noexcept -> NanSize;
        [[nodiscard]] auto estimated_bytes() const noexcept -> std::size_t;

    private:
        IRenderDevice* device_;
        TextureHandle handle_;
        NanSize size_;
        std::size_t estimated_bytes_ = 0;
    };

    struct TextureCacheLimits {
        std::size_t max_retained_entries = 64;
        std::size_t max_retained_bytes = 128U * 1024U * 1024U;
    };

    class TextureCache final {
    public:
        explicit TextureCache(IRenderDevice& device, TextureCacheLimits limits = {});
        ~TextureCache();

        TextureCache(const TextureCache&) = delete;
        auto operator=(const TextureCache&) -> TextureCache& = delete;

        [[nodiscard]] auto load_file(
            std::string_view path,
            const ImageLoadOptions& options = {}
        ) -> std::shared_ptr<CachedTexture>;

        [[nodiscard]] auto load_memory(
            std::string_view cache_key,
            std::span<const std::uint8_t> bytes,
            std::string_view media_type,
            const ImageLoadOptions& options = {}
        ) -> std::shared_ptr<CachedTexture>;

        void clear();
        [[nodiscard]] auto retained_entries() const noexcept -> std::size_t;
        [[nodiscard]] auto retained_bytes() const noexcept -> std::size_t;

    private:
        enum class SourceKind { file, memory };

        struct Key {
            SourceKind kind = SourceKind::file;
            std::string source;
            std::string media_type;
            ImageLoadOptions options;
        };

        struct IndexedEntry {
            Key key;
            std::weak_ptr<CachedTexture> texture;
        };

        struct RetainedEntry {
            Key key;
            std::shared_ptr<CachedTexture> texture;
        };

        [[nodiscard]] auto find(const Key& key) -> std::shared_ptr<CachedTexture>;
        void index(Key key, const std::shared_ptr<CachedTexture>& texture);
        void retain(const Key& key, std::shared_ptr<CachedTexture> texture);
        void trim();
        void prune_expired();
        [[nodiscard]] static auto same_key(const Key& lhs, const Key& rhs) -> bool;

        IRenderDevice* device_;
        TextureCacheLimits limits_;
        std::vector<IndexedEntry> index_;
        std::list<RetainedEntry> retained_;
        std::size_t retained_bytes_ = 0;
    };
} // namespace nandina::render

#endif // NANDINA_EXPERIMENT_RENDER_TEXTURE_CACHE_HPP
