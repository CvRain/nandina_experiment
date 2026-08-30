#ifndef NANDINA_EXPERIMENT_EXAMPLE_SETTINGS_GALLERY_PAGE_HPP
#define NANDINA_EXPERIMENT_EXAMPLE_SETTINGS_GALLERY_PAGE_HPP

#include "app/nan_page.hpp"

namespace nandina::examples::settings
{
    class GalleryPage final: public app::NanPageT<app::NoParams> {
    public:
        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "gallery";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override;
    };
}

#endif
