#include "gallery_page.hpp"

#include "foundation/geometry.hpp"
#include "theme/tokens.hpp"
#include "widget/controls.hpp"
#include "widget/layout.hpp"

#include <array>
#include <utility>

namespace nandina::examples::settings
{
    namespace
    {
        [[nodiscard]] auto heading(const widget::BuildContext& ui, std::string text)
            -> std::shared_ptr<widget::Label> {
            return ui.make<widget::Label>(std::move(text)).font_size(20.0F).build();
        }
    }

    auto GalleryPage::build(app::PageContext& context) -> std::shared_ptr<scene::NanNode2D> {
        auto ui = context.ui();
        const auto gallery_image = [&ui](std::string source, std::string title, foundation::NanSize upload_size) {
            auto image = ui.make<widget::Image>(std::move(source))
                .configure([upload_size](widget::Image& image) {
                    image.set_size(foundation::NanSize(420.0F, 240.0F));
                    image.set_scale_mode(widget::ImageScale::contain);
                    image.set_load_options(render::ImageLoadOptions {.resize = upload_size});
                })
                .build();
            return ui.column()
                .gap(6.0F)
                .cross_alignment(widget::LayoutAlignment::start)
                .children(
                    ui.make<widget::Label>(std::move(title))
                        .color_token(theme::ColorToken::on_surface_variant),
                    image
                )
                .build();
        };
        auto content = ui.column()
            .gap(18.0F)
            .cross_alignment(widget::LayoutAlignment::stretch)
            .children(
                heading(ui, "Gallery"),
                ui.make<widget::Label>(
                       "Packaged image loading (res://) with contain scaling and bounded upload size."
                   )
                    .color_token(theme::ColorToken::on_surface_variant)
            )
            .build();
        constexpr std::array samples {
            std::pair {"res://random_wallpaper.png", "random_wallpaper.png"},
            std::pair {"res://random_wallpaper.jpg", "random_wallpaper.jpg"},
            std::pair {"res://random_wallpaper-1.png", "random_wallpaper-1.png"},
            std::pair {"res://random_wallpaper-1.jpg", "random_wallpaper-1.jpg"},
        };
        const std::array upload_sizes {
            foundation::NanSize(640.0F, 478.0F),
            foundation::NanSize(640.0F, 453.0F),
            foundation::NanSize(840.0F, 473.0F),
            foundation::NanSize(640.0F, 484.0F),
        };
        for (std::size_t index = 0; index < samples.size(); ++index) {
            content->add_child(gallery_image(samples[index].first, samples[index].second, upload_sizes[index]));
        }
        return ui.padding(foundation::NanInsets::all(20.0F))
            .child(ui.scroll_view().child(content))
            .build();
    }
}
