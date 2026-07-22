//
// Created by cvrain on 2026/7/4.
//

#include "nan_application.hpp"
#include "nan_window.hpp"

#include "../foundation/nan_logger.hpp"
#include "../resource/backends/builtin_backend.hpp"
#include "../resource/backends/sqlite_backend.hpp"
#include "../resource/platform_resource_locator.hpp"

namespace nandina::app
{

    namespace
    {
        void install_builtin_services(
            resource::ResourceManager& resources,
            std::vector<std::shared_ptr<resource::IResourceBackend>>& backends,
            text::FontFamilyRegistry& families
        ) {
            auto builtin = resource::BuiltinBackend::create();
            (void)resources.mount(builtin, -1000);
            backends.push_back(std::move(builtin));
            auto registered = text::register_builtin_default_font_family(families);
            if (!registered) {
                throw std::runtime_error(
                    "NanApplication: cannot register built-in default font family"
                );
            }
        }
    } // namespace

    NanApplication::NanApplication() {
        log::initialize();
        install_builtin_services(resources_, resource_backends_, font_families_);
        font_loader_ = std::make_unique<text::FontLoader>(resources_);
        log::get("app.application").info("NanApplication: initialized");
    }

    NanApplication::NanApplication(NanApplicationConfig config):
        application_id_(std::move(config.application_id)) {
        log::initialize({.name = application_id_.empty() ? "nandina" : application_id_});
        install_builtin_services(resources_, resource_backends_, font_families_);
        auto locator = resource::PlatformResourceLocator::create({
            .application_id = application_id_,
            .executable_path = std::move(config.executable_path),
            .environment = std::move(config.environment),
        });
        if (!locator) {
            throw std::invalid_argument("NanApplication: " + locator.error());
        }
        int priority = 1000;
        for (const auto& location: locator->resource_roots()) {
            const auto database = location.root / config.resource_package;
            std::error_code error;
            if (!std::filesystem::exists(database, error)) {
                if (error) {
                    throw std::runtime_error(
                        "NanApplication: cannot inspect resource package path"
                    );
                }
                --priority;
                continue;
            }
            auto backend = resource::SQLiteBackend::open({
                .name = "package:" + database.string(),
                .database = database,
                .external_root = location.root,
            });
            if (!backend) {
                throw std::runtime_error(
                    "NanApplication: cannot mount resource package " + database.string() + ": "
                    + backend.error().message
                );
            }
            (void)resources_.mount(*backend, priority);
            resource_backends_.push_back(std::move(*backend));
            --priority;
        }
        font_loader_ = std::make_unique<text::FontLoader>(resources_);
        log::get("app.application").info(
            "NanApplication: initialized {}",
            application_id_
        );
    }

    NanApplication::~NanApplication() = default;

    auto NanApplication::graph() -> reactive::Graph& {
        return graph_;
    }

    void NanApplication::set_theme(theme::NanTheme theme) {
        theme_ = theme;
    }

    auto NanApplication::theme() const -> const theme::NanTheme& {
        return theme_;
    }

    auto NanApplication::resources() -> resource::ResourceManager& {
        return resources_;
    }
    auto NanApplication::resources() const -> const resource::ResourceManager& {
        return resources_;
    }
    auto NanApplication::font_loader() -> text::FontLoader& {
        return *font_loader_;
    }
    auto NanApplication::font_families() -> text::FontFamilyRegistry& {
        return font_families_;
    }
    auto NanApplication::application_id() const noexcept -> std::string_view {
        return application_id_;
    }

    auto NanApplication::dispatcher() -> UiDispatcher& {
        return dispatcher_;
    }

    auto NanApplication::background_executor() -> BackgroundExecutor& {
        return background_executor_;
    }

    auto NanApplication::store_base() -> NanStore* {
        return store_.get();
    }

    auto NanApplication::store_type_key() const -> NanTypeKey {
        return store_key_;
    }

    auto NanApplication::run(NanWindow& window) -> int {
        dispatcher_.require_ui_thread();
        window.open();
        while (!window.should_close()) {
            window.tick();
        }
        window.close();
        log::get("app.application").info("NanApplication: exited");
        return 0;
    }

} // namespace nandina::app
