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
}

TEST_CASE("NanApplication always installs builtin resource and font services", "[app][resource]") {
    app::NanApplication application;
    auto font = application.resources().require(resource::ResourceKey("fonts/default"));
    REQUIRE(font.has_value());
    REQUIRE((*font)->storage() == resource::ResourceStorage::embedded_blob);
    auto family = application.font_families().resolve({}, application.font_loader());
    REQUIRE(family.has_value());
    REQUIRE(family->faces.size() == 1);
}

TEST_CASE("NanApplication discovers executable-relative SQLite packages", "[app][resource]") {
    const auto executable = std::filesystem::path(NANDINA_TEST_PACKAGE_EXECUTABLE);
    app::NanApplication application({
        .application_id = "org.nandina.todo",
        .executable_path = executable,
        .environment = {{"HOME", "/nonexistent-home"}},
    });
    REQUIRE(application.application_id() == "org.nandina.todo");
    auto font = application.resources().require(resource::ResourceKey("fonts/default"));
    REQUIRE(font.has_value());
    REQUIRE((*font)->key().value() == "fonts/default-ui.ttf");
    auto registered = text::register_optional_font_fallback(
        application.font_families(),
        application.resources(),
        resource::ResourceKey("families/zh-cn"),
        resource::ResourceKey("fonts/fallback/zh-cn")
    );
    REQUIRE(registered.has_value());
    REQUIRE(*registered);
    auto family = application.font_families().resolve({}, application.font_loader());
    REQUIRE(family.has_value());
    REQUIRE(family->faces.size() == 2);
    REQUIRE(family->faces[1]->glyph_index(U'中') != 0);
    REQUIRE(family->faces[1]->glyph_index(U'文') != 0);

    ServiceCapture capture;
    app::NanRouter router {
        application.graph(), application.theme(), nullptr, nullptr,
        &application.resources(), &application.font_loader(), &application.font_families()
    };
    router.push<ServicePage>(ServiceParams {.capture = &capture});
    REQUIRE(capture.resources == &application.resources());
    REQUIRE(capture.loader == &application.font_loader());
    REQUIRE(capture.families == &application.font_families());
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
