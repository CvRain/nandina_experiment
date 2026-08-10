//
// Created by cvrain on 2026/8/10.
//

#ifndef NANDINA_EXPERIMENT_BASE_WINDOW_HPP
#define NANDINA_EXPERIMENT_BASE_WINDOW_HPP

#include "app/nan_page.hpp"

#include <memory>

namespace nandina::example::base_window
{

    class MainPage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(widget::BuildContext& ui) -> widget::View override;
    };
} // namespace nandina::example::base_window

#endif // NANDINA_EXPERIMENT_BASE_WINDOW_HPP
