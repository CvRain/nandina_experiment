//
// theme/theme_manager — named themes, appearance-aware families, and revision notifications.
//

#ifndef NANDINA_EXPERIMENT_THEME_THEME_MANAGER_HPP
#define NANDINA_EXPERIMENT_THEME_THEME_MANAGER_HPP

#include "nan_style.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace nandina::theme
{
    enum class ColorAppearance {
        light,
        dark,
    };

    enum class ThemePreference {
        system,
        light,
        dark,
    };

    struct ThemeFamily {
        std::string light_theme;
        std::string dark_theme;
    };

    class ThemeManager;

    class ThemeObserver {
    public:
        virtual ~ThemeObserver() = default;
        virtual void on_theme_revision_changed(const ThemeManager& manager) = 0;
        virtual void on_theme_manager_destroyed(const ThemeManager& manager) noexcept = 0;
    };

    class ThemeManager {
    public:
        ThemeManager();
        ~ThemeManager();

        ThemeManager(const ThemeManager&) = delete;
        auto operator=(const ThemeManager&) -> ThemeManager& = delete;
        ThemeManager(ThemeManager&&) = delete;
        auto operator=(ThemeManager&&) -> ThemeManager& = delete;

        [[nodiscard]] auto register_theme(std::string name, NanTheme theme) -> bool;
        [[nodiscard]] auto contains(std::string_view name) const -> bool;
        [[nodiscard]] auto activate(std::string_view name) -> bool;
        void set_theme(NanTheme theme);

        [[nodiscard]] auto
        register_family(std::string name, std::string light_theme, std::string dark_theme) -> bool;
        [[nodiscard]] auto contains_family(std::string_view name) const -> bool;
        [[nodiscard]] auto activate_family(std::string_view name) -> bool;
        void set_preference(ThemePreference preference);
        void set_system_appearance(ColorAppearance appearance);

        [[nodiscard]] auto theme() const -> const NanTheme&;
        [[nodiscard]] auto active_name() const noexcept -> std::string_view;
        [[nodiscard]] auto active_family() const noexcept -> std::string_view;
        [[nodiscard]] auto preference() const noexcept -> ThemePreference;
        [[nodiscard]] auto appearance() const noexcept -> ColorAppearance;
        [[nodiscard]] auto revision() const noexcept -> std::uint64_t;

        void set_style(std::shared_ptr<const NanStyle> style);
        [[nodiscard]] auto style() const -> const NanStyle&;

        void add_observer(ThemeObserver& observer);
        void remove_observer(ThemeObserver& observer) noexcept;

    private:
        [[nodiscard]] auto effective_theme_name() const noexcept -> std::string_view;
        void publish_revision();

        std::map<std::string, NanTheme, std::less<>> themes_;
        std::map<std::string, ThemeFamily, std::less<>> families_;
        std::string active_name_ = "default";
        std::string active_family_;
        ThemePreference preference_ = ThemePreference::system;
        ColorAppearance system_appearance_ = ColorAppearance::light;
        std::shared_ptr<const NanStyle> style_ = default_style();
        std::vector<ThemeObserver*> observers_;
        std::uint64_t revision_ = 1;
    };

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_THEME_MANAGER_HPP
