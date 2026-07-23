//
// theme/theme_manager — named themes, active style rules, and revision notifications.
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
        const bool replaces_active = active_name_ == name;
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
        if (active_name_ == name) {
            return true;
        }
        active_name_ = found->first;
        publish_revision();
        return true;
    }

    void ThemeManager::set_theme(NanTheme theme) {
        themes_.insert_or_assign("application", std::move(theme));
        if (active_name_ == "application") {
            publish_revision();
            return;
        }
        active_name_ = "application";
        publish_revision();
    }

    auto ThemeManager::theme() const -> const NanTheme& {
        return themes_.at(active_name_);
    }

    auto ThemeManager::active_name() const noexcept -> std::string_view {
        return active_name_;
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
