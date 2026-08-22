//
// Image (texture) node tests — Stage 1 of the texture subsystem.
//

#include "foundation/geometry.hpp"
#include "foundation/nandina_color.hpp"
#include "render/render_device.hpp"
#include "scene/scene_tree.hpp"
#include "widget/image.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace nandina;

namespace
{
    /// 记录纹理调用，验证 load / size / draw_texture_region 的调用序与参数。
    class TextureRecordingDevice final: public render::IRenderDevice {
    public:
        struct Region {
            foundation::NanRect source;
            foundation::NanRect destination;
            foundation::NanColor tint;
        };

        std::vector<std::string> loaded;
        int draw_regions = 0;
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

        void draw_texture_region(
            render::TextureHandle,
            const foundation::NanRect& source,
            const foundation::NanRect& destination,
            const foundation::NanColor& tint
        ) override {
            ++draw_regions;
            regions.push_back(Region {source, destination, tint});
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
