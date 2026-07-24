//
// theme/theme_manager — named themes, appearance-aware families, and revision notifications.
//

#include "theme_manager.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nandina::theme
{
    ThemeManager::ThemeManager() {
        themes_.emplace(active_name_, default_theme());
    }

    ThemeManager::~ThemeManager() {
        const auto observers = std::move(observers_);
        for (auto* observer: observers) {
            observer->on_theme_manager_destroyed(*this);
        }
    }

    auto ThemeManager::register_theme(std::string name, NanTheme theme) -> bool {
        if (name.empty()) {
            throw std::invalid_argument("ThemeManager theme name cannot be empty");
        }
        const bool replaced = themes_.contains(name);
        const bool replaces_active = active_name() == name;
        themes_.insert_or_assign(std::move(name), std::move(theme));
        if (replaces_active) {
            publish_revision();
        }
        return !replaced;
    }

    auto ThemeManager::contains(const std::string_view name) const -> bool {
        return themes_.contains(name);
    }

    auto ThemeManager::activate(const std::string_view name) -> bool {
        const auto found = themes_.find(name);
        if (found == themes_.end()) {
            return false;
        }
        if (active_family_.empty() && active_name_ == name) {
            return true;
        }
        const std::string previous(effective_theme_name());
        active_family_.clear();
        active_name_ = found->first;
        if (previous != active_name_) {
            publish_revision();
        }
        return true;
    }

    void ThemeManager::set_theme(NanTheme theme) {
        themes_.insert_or_assign("application", std::move(theme));
        if (active_family_.empty() && active_name_ == "application") {
            publish_revision();
            return;
        }
        active_family_.clear();
        active_name_ = "application";
        publish_revision();
    }

    auto
    ThemeManager::register_family(std::string name, std::string light_theme, std::string dark_theme)
        -> bool {
        if (name.empty()) {
            throw std::invalid_argument("ThemeManager family name cannot be empty");
        }
        if (!themes_.contains(light_theme) || !themes_.contains(dark_theme)) {
            throw std::invalid_argument("ThemeManager family variants must be registered themes");
        }
        const bool replaced = families_.contains(name);
        const bool replaces_active = active_family_ == name;
        const std::string previous(effective_theme_name());
        families_.insert_or_assign(
            std::move(name),
            ThemeFamily {
                .light_theme = std::move(light_theme),
                .dark_theme = std::move(dark_theme),
            }
        );
        if (replaces_active && previous != effective_theme_name()) {
            publish_revision();
        }
        return !replaced;
    }

    auto ThemeManager::contains_family(const std::string_view name) const -> bool {
        return families_.contains(name);
    }

    auto ThemeManager::activate_family(const std::string_view name) -> bool {
        const auto found = families_.find(name);
        if (found == families_.end()) {
            return false;
        }
        if (active_family_ == name) {
            return true;
        }
        const std::string previous(effective_theme_name());
        active_family_ = found->first;
        if (previous != effective_theme_name()) {
            publish_revision();
        }
        return true;
    }

    void ThemeManager::set_preference(const ThemePreference preference) {
        if (preference_ == preference) {
            return;
        }
        const std::string previous(effective_theme_name());
        preference_ = preference;
        if (previous != effective_theme_name()) {
            publish_revision();
        }
    }

    void ThemeManager::set_system_appearance(const ColorAppearance appearance) {
        if (system_appearance_ == appearance) {
            return;
        }
        const std::string previous(effective_theme_name());
        system_appearance_ = appearance;
        if (previous != effective_theme_name()) {
            publish_revision();
        }
    }

    auto ThemeManager::theme() const -> const NanTheme& {
        return themes_.at(effective_theme_name());
    }

    auto ThemeManager::active_name() const noexcept -> std::string_view {
        return effective_theme_name();
    }

    auto ThemeManager::active_family() const noexcept -> std::string_view {
        return active_family_;
    }

    auto ThemeManager::preference() const noexcept -> ThemePreference {
        return preference_;
    }

    auto ThemeManager::appearance() const noexcept -> ColorAppearance {
        switch (preference_) {
            case ThemePreference::light:
                return ColorAppearance::light;
            case ThemePreference::dark:
                return ColorAppearance::dark;
            case ThemePreference::system:
                return system_appearance_;
        }
        return system_appearance_;
    }

    auto ThemeManager::revision() const noexcept -> std::uint64_t {
        return revision_;
    }

    void ThemeManager::set_style(std::shared_ptr<const NanStyle> style) {
        if (!style) {
            throw std::invalid_argument("ThemeManager style cannot be null");
        }
        style_ = std::move(style);
        publish_revision();
    }

    auto ThemeManager::style() const -> const NanStyle& {
        return *style_;
    }

    void ThemeManager::add_observer(ThemeObserver& observer) {
        if (std::ranges::find(observers_, &observer) == observers_.end()) {
            observers_.push_back(&observer);
        }
    }

    void ThemeManager::remove_observer(ThemeObserver& observer) noexcept {
        std::erase(observers_, &observer);
    }

    auto ThemeManager::effective_theme_name() const noexcept -> std::string_view {
        if (active_family_.empty()) {
            return active_name_;
        }
        const auto& family = families_.at(active_family_);
        return appearance() == ColorAppearance::dark ? family.dark_theme : family.light_theme;
    }

    void ThemeManager::publish_revision() {
        ++revision_;
        const auto observers = observers_;
        for (auto* observer: observers) {
            if (std::ranges::find(observers_, observer) != observers_.end()) {
                observer->on_theme_revision_changed(*this);
            }
        }
    }

} // namespace nandina::theme
