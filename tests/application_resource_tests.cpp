#include "app/nan_application.hpp"
#include "app/nan_router.hpp"
#include "resource/resource.hpp"
#include "scene/control.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

using namespace nandina;

namespace
{
    struct ServiceCapture {
        resource::ResourceManager* resources = nullptr;
        text::FontLoader* loader = nullptr;
        text::FontFamilyRegistry* families = nullptr;
    };

    struct ServiceParams { ServiceCapture* capture = nullptr; };

    class ServicePage final: public app::NanPageT<ServiceParams> {
    public:
        explicit ServicePage(ServiceParams params): NanPageT(params) {}
        [[nodiscard]] auto route_key() const -> std::string_view override { return "services"; }
        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override {
            REQUIRE(context.has_resource_services());
            params().capture->resources = &context.resources();
            params().capture->loader = &context.font_loader();
            params().capture->families = &context.font_families();
            return std::make_shared<scene::NanControl>();
        }
    };

    class DefaultStartupPage final: public app::NanPageT<app::NoParams> {
    public:
        DefaultStartupPage() = default;
        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "startup";
        }
        [[nodiscard]] auto build(app::PageContext&) -> std::shared_ptr<scene::NanNode2D> override {
            return std::make_shared<scene::NanControl>();
        }
    };

    class RecommendedMainPage final: public app::Page<> {
    public:
        [[nodiscard]] auto build(widget::BuildContext& ui) -> widget::View override {
            return ui.make<widget::Label>("Recommended entry").build();
        }
    };

    static_assert(requires(app::NanApplication& application, app::WindowConfig config) {
        { application.run_page<DefaultStartupPage>(std::move(config)) } -> std::same_as<int>;
    });
    static_assert(
        requires(app::NanApplication& application, app::WindowConfig config, ServiceParams params) {
            { application.run_page<ServicePage>(std::move(config), params) } -> std::same_as<int>;
        }
    );
    static_assert(requires(app::NanApplication& application, app::WindowConfig config) {
        {
            application.run(std::move(config), [](widget::BuildContext& ui) {
                return ui.make<widget::Label>("Hello");
            })
        } -> std::same_as<int>;
    });
    static_assert(requires(app::RunConfig config) {
        {
            app::run(std::move(config), [](widget::BuildContext& ui) {
                return ui.make<widget::Label>("Hello");
            })
        } -> std::same_as<int>;
    });
    static_assert(requires(app::RunConfig config) {
        { app::run<RecommendedMainPage>(std::move(config)) } -> std::same_as<int>;
    });

    [[maybe_unused]] auto compile_default_page_runner(
        app::NanApplication& application,
        app::WindowConfig config
    ) -> int {
        return application.run_page<DefaultStartupPage>(std::move(config));
    }
} // namespace

TEST_CASE("NanApplication always installs builtin resource and font services", "[app][resource]") {
    app::NanApplication application;
    auto font = application.resources().require(resource::ResourceKey("fonts/default"));
    REQUIRE(font.has_value());
    REQUIRE((*font)->storage() == resource::ResourceStorage::embedded_blob);
    auto family = application.font_families().resolve({}, application.font_loader());
    REQUIRE(family.has_value());
    REQUIRE(family->faces.size() == 1);
}

TEST_CASE("functional root views use existing page context and concrete nodes", "[app][view]") {
    app::NanApplication application;
    app::NanRouter router {application.graph(), application.theme_manager()};
    bool received_page_context = false;
    auto params = app::detail::make_root_view_params([&](app::PageContext& context) {
        received_page_context = true;
        return context.ui().make<widget::Label>("Functional root");
    });

    (void)router.push<app::detail::RootViewPage>(std::move(params));

    REQUIRE(received_page_context);
    REQUIRE(router.depth() == 1);
    REQUIRE(router.current_key() == "root");
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE(dynamic_cast<widget::Label*>(router.host()->get_child(0)) != nullptr);
}

TEST_CASE("functional root views accept BuildContext-only factories", "[app][view]") {
    app::NanApplication application;
    app::NanRouter router {application.graph(), application.theme_manager()};
    bool received_build_context = false;
    auto params = app::detail::make_root_view_params([&](widget::BuildContext& ui) {
        received_build_context = true;
        return ui.make<widget::Button>("Continue");
    });

    (void)router.push<app::detail::RootViewPage>(std::move(params));

    REQUIRE(received_build_context);
    REQUIRE(dynamic_cast<widget::Button*>(router.host()->get_child(0)) != nullptr);
}

TEST_CASE("recommended pages adapt BuildContext onto the existing router", "[app][page]") {
    app::NanApplication application;
    app::NanRouter router {application.graph(), application.theme_manager()};

    (void)router.push<RecommendedMainPage>();

    REQUIRE(router.depth() == 1);
    REQUIRE(router.host()->child_count() == 1);
    REQUIRE(dynamic_cast<widget::Label*>(router.host()->get_child(0)) != nullptr);
}

TEST_CASE("optional font fallback is absent without a project package", "[app][resource][font]") {
    app::NanApplication application;
    auto registered = text::register_optional_font_fallback(
        application.font_families(),
        application.resources(),
        resource::ResourceKey("families/zh-cn"),
        resource::ResourceKey("fonts/fallback/zh-cn")
    );
    REQUIRE(registered.has_value());
    REQUIRE_FALSE(*registered);
}

TEST_CASE("NanApplication rejects malformed discovered packages", "[app][resource]") {
    const auto root = std::filesystem::temp_directory_path()
        / ("nandina-app-" + resource::ResourceId::random().to_string());
    std::filesystem::create_directories(root / "resources");
    { std::ofstream file(root / "resources/resources.db"); file << "not sqlite"; }
    REQUIRE_THROWS_AS(
        app::NanApplication(app::NanApplicationConfig {
            .application_id = "org.nandina.invalid",
            .executable_path = root / "application",
            .environment = {{"HOME", "/nonexistent-home"}},
        }),
        std::runtime_error
    );
    std::error_code error;
    std::filesystem::remove_all(root, error);
}
