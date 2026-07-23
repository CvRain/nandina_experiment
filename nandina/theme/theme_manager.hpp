//
// theme/theme_manager — named themes, active style rules, and revision notifications.
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

        [[nodiscard]] auto theme() const -> const NanTheme&;
        [[nodiscard]] auto active_name() const noexcept -> std::string_view;
        [[nodiscard]] auto revision() const noexcept -> std::uint64_t;

        void set_style(std::shared_ptr<const NanStyle> style);
        [[nodiscard]] auto style() const -> const NanStyle&;

        void add_observer(ThemeObserver& observer);
        void remove_observer(ThemeObserver& observer) noexcept;

    private:
        void publish_revision();

        std::map<std::string, NanTheme, std::less<>> themes_;
        std::string active_name_ = "default";
        std::shared_ptr<const NanStyle> style_ = default_style();
        std::vector<ThemeObserver*> observers_;
        std::uint64_t revision_ = 1;
    };

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_THEME_MANAGER_HPP
