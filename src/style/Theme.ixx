module;

export module Nandina.Style.Theme;

import Nandina.Core.Color;

export namespace Nandina {
    struct Theme {
        struct Colors {
            Color primary{37, 99, 235, 255};
            Color background{255, 255, 255, 255};
            Color foreground{15, 23, 42, 255};
            Color muted{148, 163, 184, 255};
            Color border{226, 232, 240, 255};
        } colors;

        struct Radius {
            float sm = 4.0f;
            float md = 8.0f;
            float lg = 12.0f;
            float full = 9999.0f;
        } radius;

        struct Spacing {
            float xs = 4.0f;
            float sm = 8.0f;
            float md = 12.0f;
            float lg = 16.0f;
            float xl = 24.0f;
        } spacing;

        struct Typography {
            float xs = 12.0f;
            float sm = 14.0f;
            float md = 16.0f;
            float lg = 20.0f;
        } typography;

        [[nodiscard]] static auto Default() noexcept -> Theme&;
    };

    auto Theme::Default() noexcept -> Theme& {
        static Theme theme{};
        return theme;
    }
}