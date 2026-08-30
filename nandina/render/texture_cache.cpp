#include "texture_cache.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace nandina::render
{
    namespace
    {
        [[nodiscard]] auto estimate_rgba_bytes(const NanSize size) noexcept -> std::size_t {
            const float width = size.get_width();
            const float height = size.get_height();
            if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0F || height <= 0.0F)
            {
                return 0;
            }
            const long double bytes = static_cast<long double>(width)
                * static_cast<long double>(height) * 4.0L;
            if (bytes >= static_cast<long double>(std::numeric_limits<std::size_t>::max())) {
                return std::numeric_limits<std::size_t>::max();
            }
            return static_cast<std::size_t>(bytes);
        }

        [[nodiscard]] auto same_options(
            const ImageLoadOptions& lhs,
            const ImageLoadOptions& rhs
        ) -> bool {
            return lhs.crop == rhs.crop && lhs.resize == rhs.resize && lhs.tint == rhs.tint;
        }
    } // namespace

    CachedTexture::CachedTexture(
        IRenderDevice& device,
        const TextureHandle handle,
        const NanSize size
    ) noexcept:
        device_(&device),
        handle_(handle),
        size_(size),
        estimated_bytes_(estimate_rgba_bytes(size)) {}

    CachedTexture::~CachedTexture() {
        if (device_ != nullptr && handle_) {
            device_->destroy_texture(handle_);
        }
    }

    auto CachedTexture::handle() const noexcept -> TextureHandle {
        return handle_;
    }

    auto CachedTexture::size() const noexcept -> NanSize {
        return size_;
    }

    auto CachedTexture::estimated_bytes() const noexcept -> std::size_t {
        return estimated_bytes_;
    }

    TextureCache::TextureCache(IRenderDevice& device, const TextureCacheLimits limits):
        device_(&device), limits_(limits) {}

    TextureCache::~TextureCache() {
        clear();
    }

    auto TextureCache::load_file(
        const std::string_view path,
        const ImageLoadOptions& options
    ) -> std::shared_ptr<CachedTexture> {
        if (path.empty()) {
            return {};
        }
        const Key key {
            .kind = SourceKind::file,
            .source = std::string(path),
            .media_type = {},
            .options = options,
        };
        if (auto cached = find(key)) {
            retain(key, cached);
            return cached;
        }
        const auto handle = device_->load_texture_from_file(path, options);
        if (!handle) {
            return {};
        }
        auto texture = std::make_shared<CachedTexture>(
            *device_, handle, device_->texture_size(handle)
        );
        index(key, texture);
        retain(key, texture);
        return texture;
    }

    auto TextureCache::load_memory(
        const std::string_view cache_key,
        const std::span<const std::uint8_t> bytes,
        const std::string_view media_type,
        const ImageLoadOptions& options
    ) -> std::shared_ptr<CachedTexture> {
        if (cache_key.empty() || bytes.empty() || media_type.empty()) {
            return {};
        }
        const Key key {
            .kind = SourceKind::memory,
            .source = std::string(cache_key),
            .media_type = std::string(media_type),
            .options = options,
        };
        if (auto cached = find(key)) {
            retain(key, cached);
            return cached;
        }
        const auto handle = device_->load_texture_from_memory(bytes, media_type, options);
        if (!handle) {
            return {};
        }
        auto texture = std::make_shared<CachedTexture>(
            *device_, handle, device_->texture_size(handle)
        );
        index(key, texture);
        retain(key, texture);
        return texture;
    }

    void TextureCache::clear() {
        index_.clear();
        retained_.clear();
        retained_bytes_ = 0;
    }

    auto TextureCache::retained_entries() const noexcept -> std::size_t {
        return retained_.size();
    }

    auto TextureCache::retained_bytes() const noexcept -> std::size_t {
        return retained_bytes_;
    }

    auto TextureCache::find(const Key& key) -> std::shared_ptr<CachedTexture> {
        prune_expired();
        const auto found = std::ranges::find_if(index_, [&key](const IndexedEntry& entry) {
            return same_key(entry.key, key);
        });
        return found != index_.end() ? found->texture.lock() : nullptr;
    }

    void TextureCache::index(Key key, const std::shared_ptr<CachedTexture>& texture) {
        index_.push_back(IndexedEntry {.key = std::move(key), .texture = texture});
    }

    void TextureCache::retain(const Key& key, std::shared_ptr<CachedTexture> texture) {
        for (auto it = retained_.begin(); it != retained_.end(); ++it) {
            if (same_key(it->key, key)) {
                retained_bytes_ -= it->texture->estimated_bytes();
                retained_.erase(it);
                break;
            }
        }
        retained_bytes_ += texture->estimated_bytes();
        retained_.push_front(RetainedEntry {.key = key, .texture = std::move(texture)});
        trim();
    }

    void TextureCache::trim() {
        while (!retained_.empty()
               && (retained_.size() > limits_.max_retained_entries
                   || retained_bytes_ > limits_.max_retained_bytes))
        {
            retained_bytes_ -= retained_.back().texture->estimated_bytes();
            retained_.pop_back();
        }
        prune_expired();
    }

    void TextureCache::prune_expired() {
        std::erase_if(index_, [](const IndexedEntry& entry) { return entry.texture.expired(); });
    }

    auto TextureCache::same_key(const Key& lhs, const Key& rhs) -> bool {
        return lhs.kind == rhs.kind && lhs.source == rhs.source
            && lhs.media_type == rhs.media_type && same_options(lhs.options, rhs.options);
    }
} // namespace nandina::render
