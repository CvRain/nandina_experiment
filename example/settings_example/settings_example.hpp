//
// Settings example - a router-driven dashboard showcasing components and state.
//
// The app shell (ShellPage) owns a nested keep-alive router: a persistent
// sidebar switches section pages in the content area, and an About-page button
// pushes a parameterized DetailPage to exercise the stack (push/pop).
//

#ifndef NANDINA_EXPERIMENT_EXAMPLE_SETTINGS_EXAMPLE_HPP
#define NANDINA_EXPERIMENT_EXAMPLE_SETTINGS_EXAMPLE_HPP

#include "app/nan_page.hpp"
#include "app/nan_router.hpp"
#include "app/nan_store.hpp"
#include "reactive/signal.hpp"
#include "widget/build_context.hpp"

#include <memory>
#include <string>

namespace nandina::examples::settings
{
    /// Application-wide settings state, shared by keep-alive section pages.
    class SettingsStore final: public app::NanStore {
    public:
        explicit SettingsStore(reactive::Graph& graph);

        reactive::Signal<std::string> profile;
        reactive::Signal<bool> notifications;
        reactive::Signal<bool> diagnostics;
        reactive::Signal<bool> reduced_motion;
        reactive::Signal<float> interface_scale;
        reactive::Signal<int> language;
        reactive::Signal<int> family;
        reactive::Signal<std::string> status;
        reactive::Signal<bool> confirm_reset;
    };

    /// Persistent sidebar + nested router content area (the app's first page).
    class ShellPage final: public app::NanPageT<app::NoParams> {
    public:
        ShellPage() = default;
        ~ShellPage() override;

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "shell";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override;

    private:
        std::unique_ptr<app::NanRouter> content_;
    };

    /// Profile + preferences + actions (stateful controls).
    class GeneralPage final: public app::NanPageT<app::NoParams> {
    public:
        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "general";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override;
    };

    /// Appearance + theme family + brand colors.
    class AppearancePage final: public app::NanPageT<app::NoParams> {
    public:
        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "appearance";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override;
    };

    /// Component showcase: Tabs, Basics (Avatar/Chip/Divider), Button treatments.
    class ComponentsPage final: public app::NanPageT<app::NoParams> {
    public:
        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "components";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override;
    };

    /// Static info + a button that pushes a parameterized DetailPage.
    class AboutPage final: public app::NanPageT<app::NoParams> {
    public:
        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "about";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override;
    };

    struct DetailParams {
        std::string title;
        std::string body;
    };

    /// Parameterized page demonstrating push/pop with route params.
    class DetailPage final: public app::NanPageT<DetailParams> {
    public:
        explicit DetailPage(DetailParams params): NanPageT(std::move(params)) {}

        [[nodiscard]] auto route_key() const -> std::string_view override {
            return "detail";
        }

        [[nodiscard]] auto build(app::PageContext& context)
            -> std::shared_ptr<scene::NanNode2D> override;
    };

} // namespace nandina::examples::settings

#endif // NANDINA_EXPERIMENT_EXAMPLE_SETTINGS_EXAMPLE_HPP
