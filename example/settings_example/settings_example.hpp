//
// Settings example - canonical component and application authoring surface.
//

#ifndef NANDINA_EXPERIMENT_EXAMPLE_SETTINGS_EXAMPLE_HPP
#define NANDINA_EXPERIMENT_EXAMPLE_SETTINGS_EXAMPLE_HPP

#include "widget/build_context.hpp"

#include <memory>

namespace nandina::examples::settings
{
    [[nodiscard]] auto build(const widget::BuildContext& ui) -> std::shared_ptr<scene::NanNode2D>;
} // namespace nandina::examples::settings

#endif // NANDINA_EXPERIMENT_EXAMPLE_SETTINGS_EXAMPLE_HPP
