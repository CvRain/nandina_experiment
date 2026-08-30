//
// Image (texture) node tests — Stage 1 of the texture subsystem.
//

#include "foundation/geometry.hpp"
#include "foundation/nandina_color.hpp"
#include "render/render_device.hpp"
#include "render/texture_cache.hpp"
#include "resource/backends/memory_backend.hpp"
#include "resource/resource_manager.hpp"
#include "scene/scene_tree.hpp"
#include "theme/theme_manager.hpp"
#include "widget/controls.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    [[nodiscard]] auto resource_id(std::string_view value) -> resource::ResourceId {
        return *resource::ResourceId::parse(value);
    }

    /// 记录纹理调用，验证 load / size / draw_texture_region 的调用序与参数。
    class TextureRecordingDevice final: public render::IRenderDevice {
    public:
        struct Region {
            foundation::NanRect source;
            foundation::NanRect destination;
            foundation::NanColor tint;
        };

        std::vector<std::string> loaded;
        int memory_loads = 0;
        std::vector<std::uint8_t> loaded_bytes;
        std::string loaded_media_type;
        int draw_regions = 0;
        std::vector<render::TextureHandle> destroyed;
        std::vector<Region> regions;
        bool fail_loads = false;
        foundation::NanSize size_to_return {64.0F, 32.0F};
        render::ImageLoadOptions last_options {};

        void begin_frame() override {}
        void end_frame() override {}
        void set_clip(const foundation::NanRect&) override {}
        void clear_clip() override {}
        void draw_rect(const foundation::NanRect&, const foundation::NanColor&) override {}
        void draw_rect_outline(const foundation::NanRect&, float, const foundation::NanColor&) override {
        }
        void draw_rounded_rect(const foundation::NanRect&, float, const foundation::NanColor&) override {
        }
        void draw_line(
            const foundation::NanPoint&,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}
        void draw_circle(const foundation::NanPoint&, float, const foundation::NanColor&) override {
        }
        void draw_text(
            std::string_view,
            const foundation::NanPoint&,
            float,
            const foundation::NanColor&
        ) override {}

        [[nodiscard]] auto load_texture_from_file(
            std::string_view path,
            const render::ImageLoadOptions& options
        ) -> render::TextureHandle override {
            if (fail_loads) {
                return {};
            }
            loaded.emplace_back(path);
            last_options = options;
            return render::TextureHandle {.value = loaded.size()};
        }

        [[nodiscard]] auto texture_size(render::TextureHandle) -> foundation::NanSize override {
            return size_to_return;
        }

        [[nodiscard]] auto load_texture_from_memory(
            std::span<const std::uint8_t> bytes,
            std::string_view media_type,
            const render::ImageLoadOptions& options
        ) -> render::TextureHandle override {
            if (fail_loads) {
                return {};
            }
            ++memory_loads;
            loaded_bytes.assign(bytes.begin(), bytes.end());
            loaded_media_type = media_type;
            last_options = options;
            return render::TextureHandle {.value = static_cast<std::uint64_t>(memory_loads + 100)};
        }

        void draw_texture_region(
            render::TextureHandle,
            const foundation::NanRect& source,
            const foundation::NanRect& destination,
            const foundation::NanColor& tint
        ) override {
            ++draw_regions;
            regions.push_back(Region {source, destination, tint});
        }

        void destroy_texture(const render::TextureHandle texture) override {
            destroyed.push_back(texture);
        }
    };
} // namespace

TEST_CASE("image loads its source on first draw and draws the region", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    tree.set_root(image);
    REQUIRE(tree.layout_root(foundation::NanSize(200.0F, 100.0F)) >= 1);

    tree.draw(dev);

    REQUIRE(dev.loaded.size() == 1);
    REQUIRE(dev.loaded[0] == "hero.png");
    REQUIRE(dev.draw_regions == 1);
    // 源为自然尺寸，目标为节点世界矩形。
    REQUIRE(dev.regions[0].source.get_width() == Catch::Approx(64.0F));
    REQUIRE(dev.regions[0].source.get_height() == Catch::Approx(32.0F));
    REQUIRE(dev.regions[0].destination.get_width() == Catch::Approx(200.0F));
    REQUIRE(dev.regions[0].destination.get_height() == Catch::Approx(100.0F));
    REQUIRE(image->natural_size().get_width() == Catch::Approx(64.0F));
}

TEST_CASE("image loads its source exactly once across draws", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    tree.draw(dev);
    tree.draw(dev);

    REQUIRE(dev.loaded.size() == 1);
    REQUIRE(dev.draw_regions == 2);
}

TEST_CASE("image skips drawing when the texture fails to load", "[image]") {
    TextureRecordingDevice dev;
    dev.fail_loads = true;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("missing.png");
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    tree.draw(dev);

    REQUIRE(dev.loaded.empty());
    REQUIRE(dev.draw_regions == 0);
}

TEST_CASE("image multiplies tint alpha by inherited opacity", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    image->set_local_opacity(0.5F);
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    tree.draw(dev);

    REQUIRE(dev.draw_regions == 1);
    REQUIRE(dev.regions[0].tint.alpha() == Catch::Approx(0.5F));
}

TEST_CASE("image honors source_rect crop", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    image->set_source_rect(foundation::NanRect::from_xywh(8.0F, 4.0F, 16.0F, 8.0F));
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    tree.draw(dev);

    REQUIRE(dev.draw_regions == 1);
    REQUIRE(dev.regions[0].source.get_left() == Catch::Approx(8.0F));
    REQUIRE(dev.regions[0].source.get_top() == Catch::Approx(4.0F));
    REQUIRE(dev.regions[0].source.get_width() == Catch::Approx(16.0F));
    REQUIRE(dev.regions[0].source.get_height() == Catch::Approx(8.0F));
}

TEST_CASE("image contain letterboxes and centers by default", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    image->set_scale_mode(widget::ImageScale::contain);
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(100.0F, 100.0F));

    tree.draw(dev);

    REQUIRE(dev.draw_regions == 1);
    // 64:32 的自然尺寸放入 100x100：等比缩到宽 100、高 50，垂直居中。
    REQUIRE(dev.regions[0].destination.get_width() == Catch::Approx(100.0F));
    REQUIRE(dev.regions[0].destination.get_height() == Catch::Approx(50.0F));
    REQUIRE(dev.regions[0].destination.get_top() == Catch::Approx(25.0F));
    REQUIRE(dev.regions[0].source.get_width() == Catch::Approx(64.0F));
}

TEST_CASE("image contain alignment start and end anchor the box", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    image->set_scale_mode(widget::ImageScale::contain);
    image->set_alignment(widget::ImageAlignment::start);
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(100.0F, 100.0F));

    tree.draw(dev);
    image->set_alignment(widget::ImageAlignment::end);
    tree.draw(dev);

    REQUIRE(dev.regions[0].destination.get_top() == Catch::Approx(0.0F));
    REQUIRE(dev.regions[0].destination.get_left() == Catch::Approx(0.0F));
    REQUIRE(dev.regions[1].destination.get_bottom() == Catch::Approx(100.0F));
    REQUIRE(dev.regions[1].destination.get_right() == Catch::Approx(100.0F));
}

TEST_CASE("image cover crops the source to the destination aspect", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    image->set_scale_mode(widget::ImageScale::cover);
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(100.0F, 100.0F));

    tree.draw(dev);

    REQUIRE(dev.draw_regions == 1);
    // 64:32 → 100x100 的 cover：裁宽度到 32，水平居中（left=16）。
    REQUIRE(dev.regions[0].source.get_width() == Catch::Approx(32.0F));
    REQUIRE(dev.regions[0].source.get_height() == Catch::Approx(32.0F));
    REQUIRE(dev.regions[0].source.get_left() == Catch::Approx(16.0F));
    REQUIRE(dev.regions[0].destination.get_width() == Catch::Approx(100.0F));
    REQUIRE(dev.regions[0].destination.get_height() == Catch::Approx(100.0F));
}

TEST_CASE("image passes load options to the device", "[image]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("hero.png");
    image->set_load_options(
        render::ImageLoadOptions {
            .resize = foundation::NanSize(32.0F, 32.0F),
            .tint = foundation::NanColor::from_hex(0xFF0000),
        }
    );
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    tree.draw(dev);

    REQUIRE(dev.last_options.resize.has_value());
    REQUIRE(dev.last_options.resize->get_width() == Catch::Approx(32.0F));
    REQUIRE(dev.last_options.resize->get_height() == Catch::Approx(32.0F));
    REQUIRE(dev.last_options.tint.has_value());
}

TEST_CASE(
    "image loads res URI bytes through the resource manager exactly once",
    "[image][resource]"
) {
    auto backend = std::make_shared<resource::MemoryBackend>("images");
    const std::vector<std::uint8_t> png_bytes {0x89, 0x50, 0x4E, 0x47};
    REQUIRE(backend
                ->insert(
                    resource::ResourceId::random(),
                    resource::ResourceKey("assets/hero.png"),
                    "image/png",
                    png_bytes
                )
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend);

    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("res://assets/hero.png");
    image->set_resource_manager(&resources);
    image->set_load_options(
        render::ImageLoadOptions {
            .resize = foundation::NanSize(24.0F, 12.0F),
        }
    );
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(200.0F, 100.0F));

    tree.draw(dev);
    tree.draw(dev);

    REQUIRE(dev.memory_loads == 1);
    REQUIRE(dev.loaded.empty());
    REQUIRE(dev.loaded_bytes == png_bytes);
    REQUIRE(dev.loaded_media_type == "image/png");
    REQUIRE(dev.last_options.resize.has_value());
    REQUIRE(dev.draw_regions == 2);
}

TEST_CASE("image res URI failures are safe and are not retried", "[image][resource]") {
    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("res://missing.png");
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(100.0F, 100.0F));

    tree.draw(dev);
    tree.draw(dev);

    REQUIRE(dev.memory_loads == 0);
    REQUIRE(dev.loaded.empty());
    REQUIRE(dev.draw_regions == 0);

    auto backend = std::make_shared<resource::MemoryBackend>("documents");
    REQUIRE(backend
                ->insert(
                    resource::ResourceId::random(),
                    resource::ResourceKey("readme.txt"),
                    "text/plain",
                    std::vector<std::uint8_t> {'n', 'o'}
                )
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend);
    image->set_resource_manager(&resources);
    image->set_source("res://still-missing.png");

    tree.draw(dev);
    tree.draw(dev);

    REQUIRE(dev.memory_loads == 0);
    REQUIRE(dev.draw_regions == 0);

    image->set_source("res://readme.txt");

    tree.draw(dev);
    tree.draw(dev);

    REQUIRE(dev.memory_loads == 0);
    REQUIRE(dev.draw_regions == 0);
}

TEST_CASE("image switches between packaged resources and file paths", "[image][resource]") {
    auto backend = std::make_shared<resource::MemoryBackend>("images");
    REQUIRE(backend
                ->insert(
                    resource::ResourceId::random(),
                    resource::ResourceKey("logo.png"),
                    "image/png",
                    std::vector<std::uint8_t> {1, 2, 3}
                )
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend);

    TextureRecordingDevice dev;
    scene::NanSceneTree tree;
    auto image = widget::Image::create("res://logo.png");
    image->set_resource_manager(&resources);
    tree.set_root(image);
    (void)tree.layout_root(foundation::NanSize(100.0F, 100.0F));

    tree.draw(dev);
    image->set_source("logo-from-disk.png");
    tree.draw(dev);
    image->set_source("res://logo.png");
    tree.draw(dev);

    REQUIRE(dev.memory_loads == 2);
    REQUIRE(dev.loaded == std::vector<std::string> {"logo-from-disk.png"});
    REQUIRE(dev.draw_regions == 3);
    REQUIRE(dev.destroyed.size() == 2);
}

TEST_CASE("window texture cache reuses a packaged image across node lifetimes", "[image][cache]") {
    auto backend = std::make_shared<resource::MemoryBackend>("images");
    REQUIRE(backend
                ->insert(
                    resource_id("00112233-4455-4677-8899-aabbccddeeff"),
                    resource::ResourceKey("logo.png"),
                    "image/png",
                    std::vector<std::uint8_t> {1, 2, 3}
                )
                .has_value());
    resource::ResourceManager resources;
    (void)resources.mount(backend);

    TextureRecordingDevice dev;
    render::TextureCache cache(dev);
    scene::NanSceneTree tree;
    tree.set_texture_cache(cache);

    auto first = widget::Image::create("res://logo.png");
    first->set_resource_manager(&resources);
    tree.set_root(first);
    tree.draw(dev);
    tree.set_root(nullptr);
    first.reset();

    auto second = widget::Image::create("res://logo.png");
    second->set_resource_manager(&resources);
    tree.set_root(second);
    tree.draw(dev);

    REQUIRE(dev.memory_loads == 1);
    REQUIRE(dev.draw_regions == 2);
    REQUIRE(dev.destroyed.empty());
    REQUIRE(cache.retained_entries() == 1);
}

TEST_CASE("texture cache keys include image preprocessing options", "[image][cache]") {
    TextureRecordingDevice dev;
    render::TextureCache cache(dev);

    const auto original = cache.load_file("hero.png");
    const auto resized = cache.load_file(
        "hero.png",
        render::ImageLoadOptions {.resize = foundation::NanSize(32.0F, 32.0F)}
    );
    const auto original_again = cache.load_file("hero.png");

    REQUIRE(original != nullptr);
    REQUIRE(resized != nullptr);
    REQUIRE(original_again == original);
    REQUIRE(dev.loaded.size() == 2);
    REQUIRE(cache.retained_entries() == 2);
}

TEST_CASE("texture cache evicts inactive least-recently-used textures", "[image][cache]") {
    TextureRecordingDevice dev;
    render::TextureCache cache(
        dev,
        render::TextureCacheLimits {
            .max_retained_entries = 1,
            .max_retained_bytes = 1024U * 1024U,
        }
    );

    std::weak_ptr<render::CachedTexture> first_weak;
    {
        auto first = cache.load_file("first.png");
        REQUIRE(first != nullptr);
        first_weak = first;
    }
    REQUIRE_FALSE(first_weak.expired());

    {
        auto second = cache.load_file("second.png");
        REQUIRE(second != nullptr);
    }

    REQUIRE(first_weak.expired());
    REQUIRE(dev.destroyed.size() == 1);
    REQUIRE(cache.retained_entries() == 1);
}

TEST_CASE(
    "BuildContext injects resource services into authored images",
    "[image][resource][authoring]"
) {
    reactive::Graph graph;
    reactive::ReactiveScope scope(graph);
    theme::ThemeManager themes;
    resource::ResourceManager resources;
    widget::BuildContext ui(graph, scope, themes, &resources);

    auto image = ui.make<widget::Image>("res://logo.png").build();

    REQUIRE(image->resource_manager() == &resources);
}
