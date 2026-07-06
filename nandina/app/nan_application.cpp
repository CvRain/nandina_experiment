//
// Created by cvrain on 2026/7/4.
//

#include "nan_application.hpp"
#include "nan_window.hpp"

#include <spdlog/spdlog.h>

namespace nandina::app
{

    NanApplication::NanApplication() {
        spdlog::info("NanApplication: initialized");
    }

    NanApplication::~NanApplication() = default;

    auto NanApplication::graph() -> reactive::Graph& {
        return graph_;
    }

    auto NanApplication::store_base() -> NanStore* {
        return store_.get();
    }

    auto NanApplication::store_type_key() const -> NanTypeKey {
        return store_key_;
    }

    auto NanApplication::run(NanWindow& window) -> int {
        window.open();
        while (!window.should_close()) {
            window.tick();
        }
        window.close();
        spdlog::info("NanApplication: exited");
        return 0;
    }

} // namespace nandina::app
