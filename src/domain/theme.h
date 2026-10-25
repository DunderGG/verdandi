#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace verdandi::domain
{

/**
 * A colour of the monitor's theme, named by what it colours. The first five
 * are the palette's surfaces, text and accent, which Windows' palette gives
 * unless the user chooses them (DC-151). The others the widgets draw
 * themselves, and each has a shade for a light surface and one for a dark
 * surface (DC-150).
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

/** The colours that the user chose for one colour scheme. A role without one keeps its own. */
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

    bool operator==(const ThemeColors&) const = default;
};

/**
 * The colours that the user chose for the light and the dark colour scheme
 * (DC-151). Default-constructed, it chooses none, and the monitor draws in
 * its own colours.
 */
struct Theme
{
    ThemeColors light{};
    ThemeColors dark{};

    /** Returns the colours chosen for the dark scheme if darkScheme is true, and for the light one otherwise. */
    [[nodiscard]] const ThemeColors& colors(bool darkScheme) const
    {
        return darkScheme ? dark : light;
    }

    bool operator==(const Theme&) const = default;
};

} // namespace verdandi::domain
