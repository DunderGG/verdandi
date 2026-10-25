#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace verdandi::domain
{

/**
 * A colour of the monitor's theme, named by what it colours. The first five
 * are the palette's surfaces, text and accent, which Windows' palette gives
 * unless a theme or the user chooses them (DC-151). The others the widgets
 * draw themselves, and each has a shade for a light surface and one for a
 * dark surface (DC-150).
 */
enum class ThemeColorRole
{
    /** The window behind the tabs and the menu bar, and the dialogs. */
    Window,
    /** The tab pages, tables, lists and text boxes. */
    Page,
    /** The dashboard's and the Network tab's cards, which are the pages' colour unless chosen. */
    Card,
    /** The text. */
    Text,
    /** Selections, check boxes, and the current tab. */
    Accent,
    /** The charts' lines, in the order that a chart's series take them: CPU, memory and Receive take the first. */
    ChartLine1,
    ChartLine2,
    ChartLine3,
    ChartLine4,
    ChartLine5,
    /** The charts' grid and frame. */
    ChartGrid,
    /** Text that says a state is good, such as Healthy or an adapter that is Up. */
    Good,
    /** Text that says a state needs attention. */
    Warning,
    /** Text that says a state is critical. */
    Critical,
    /** The row of a process that started in the last few seconds. */
    ProcessStarted,
    /** The row of a process that has exited, until it is removed. */
    ProcessExited,
    /** The padlock before a process whose details Windows denied. */
    AccessDenied,
    /** The shield before a process that the monitor never opens. */
    AccessNeverOpened,
    /** The barred circle before a process that has no details to read. */
    AccessUnavailable,
};

/**
 * The number of ThemeColorRole values, for arrays indexed by a role's value.
 * AccessUnavailable must stay the last value: a value added after it takes
 * its place here.
 */
inline constexpr std::size_t kThemeColorRoleCount = static_cast<std::size_t>(ThemeColorRole::AccessUnavailable) + 1;

/** An opaque colour in sRGB, eight bits per channel. */
struct RgbColor
{
    std::uint8_t red{0};
    std::uint8_t green{0};
    std::uint8_t blue{0};

    bool operator==(const RgbColor&) const = default;
};

/** The colours that a theme, or the user, chooses. A role without one keeps the colour scheme's own. */
struct ThemeColors
{
    std::array<std::optional<RgbColor>, kThemeColorRoleCount> colors{};

    [[nodiscard]] const std::optional<RgbColor>& operator[](ThemeColorRole role) const
    {
        return colors[static_cast<std::size_t>(role)];
    }

    [[nodiscard]] std::optional<RgbColor>& operator[](ThemeColorRole role)
    {
        return colors[static_cast<std::size_t>(role)];
    }

    /** Returns whether no role has a colour. */
    [[nodiscard]] bool empty() const
    {
        return std::ranges::none_of(colors, [](const std::optional<RgbColor>& color) { return color.has_value(); });
    }

    /** Returns these colours, with each colour that changes chooses in place of this one's. */
    [[nodiscard]] ThemeColors withChanges(const ThemeColors& changes) const
    {
        ThemeColors changed = *this;
        for (std::size_t role = 0; role < kThemeColorRoleCount; ++role) {
            if (changes.colors[role]) {
                changed.colors[role] = changes.colors[role];
            }
        }
        return changed;
    }

    bool operator==(const ThemeColors&) const = default;
};

/**
 * The colour scheme that a theme draws in: Windows' light or dark palette,
 * and the light or dark shades of the colours that the widgets draw.
 */
enum class ColorScheme
{
    Light,
    Dark,
};

/**
 * A theme (DC-153): the colour scheme that it draws in, and the colours that
 * it chooses there. A role that it leaves out keeps the scheme's own colour:
 * Windows' for the palette's roles, and the monitor's built-in shade for the
 * others. Default-constructed, it is the Light theme.
 */
struct Theme
{
    ColorScheme scheme{ColorScheme::Light};
    ThemeColors colors{};

    bool operator==(const Theme&) const = default;
};

/** A theme of the themes folder, named after its file (DC-153). */
struct NamedTheme
{
    std::string name{};
    Theme theme{};

    bool operator==(const NamedTheme&) const = default;
};

/**
 * The names of the themes built into the monitor, which the settings choose
 * as they choose a theme of the themes folder: the one of Light and Dark
 * that Windows sets for apps, which the monitor follows, and Light and Dark,
 * which draw in Windows' palette for their scheme. No theme file can take
 * one (isBuiltInThemeName()).
 */
inline constexpr std::string_view kFollowWindowsTheme = "Follow Windows";
inline constexpr std::string_view kLightTheme = "Light";
inline constexpr std::string_view kDarkTheme = "Dark";

/** Returns whether name is a built-in theme's name, whatever the case of its ASCII letters. */
[[nodiscard]] constexpr bool isBuiltInThemeName(std::string_view name)
{
    const auto lower = [](char letter) {
        return letter >= 'A' && letter <= 'Z' ? static_cast<char>(letter - 'A' + 'a') : letter;
    };
    const auto equalIgnoringCase = [&lower](std::string_view first, std::string_view second) {
        return std::ranges::equal(first, second, [&lower](char a, char b) { return lower(a) == lower(b); });
    };
    return equalIgnoringCase(name, kFollowWindowsTheme) || equalIgnoringCase(name, kLightTheme) ||
           equalIgnoringCase(name, kDarkTheme);
}

/** Returns the built-in theme for scheme: Light or Dark, with no colours of their own. */
[[nodiscard]] constexpr std::string_view builtInThemeName(ColorScheme scheme)
{
    return scheme == ColorScheme::Dark ? kDarkTheme : kLightTheme;
}

} // namespace verdandi::domain
