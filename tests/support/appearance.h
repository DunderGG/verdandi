#pragma once

#include <array>

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QString>
#include <QStyle>
#include <QStyleHints>

#include "domain/theme.h"
#include "ui/theme/theme_colors.h"
#include "ui/theme_controller.h"

// Draws the application in a colour scheme, a style and the colours a user
// chose while a test runs, and gives back the system's scheme, the style it
// had and the theme's own colours afterwards.

namespace verdandi::tests
{

/** Sets the application's colour scheme while it exists, and returns to the system's afterwards. */
class ScopedColorScheme
{
public:
    explicit ScopedColorScheme(Qt::ColorScheme scheme)
    {
        QGuiApplication::styleHints()->setColorScheme(scheme);
        QCoreApplication::processEvents();
    }

    ~ScopedColorScheme()
    {
        QGuiApplication::styleHints()->unsetColorScheme();
        QCoreApplication::processEvents();
    }

    ScopedColorScheme(const ScopedColorScheme&) = delete;
    ScopedColorScheme& operator=(const ScopedColorScheme&) = delete;
    ScopedColorScheme(ScopedColorScheme&&) = delete;
    ScopedColorScheme& operator=(ScopedColorScheme&&) = delete;
};

/** Sets the application's style, such as "windowsvista", while it exists, and gives back its style afterwards. */
class ScopedStyle
{
public:
    explicit ScopedStyle(const QString& name) : m_previous(QApplication::style()->name())
    {
        QApplication::setStyle(name);
    }

    ~ScopedStyle()
    {
        QApplication::setStyle(m_previous);
    }

    ScopedStyle(const ScopedStyle&) = delete;
    ScopedStyle& operator=(const ScopedStyle&) = delete;
    ScopedStyle(ScopedStyle&&) = delete;
    ScopedStyle& operator=(ScopedStyle&&) = delete;

private:
    QString m_previous;
};

/** A colour scheme and the style that Qt picks on one version of Windows. */
struct Appearance
{
    const char* name{};
    Qt::ColorScheme scheme{};
    const char* style{};
};

/**
 * Each scheme in the style that Qt picks on Windows 11 and on Windows 10,
 * whose windowsvista style the monitor replaces with Fusion in the dark
 * scheme (DC-148). Named styles make the tests the same on every host.
 */
inline constexpr std::array kAppearances{
    Appearance{"light, windows11", Qt::ColorScheme::Light, "windows11"},
    Appearance{"dark, windows11", Qt::ColorScheme::Dark, "windows11"},
    Appearance{"light, windowsvista", Qt::ColorScheme::Light, "windowsvista"},
    Appearance{"dark, windowsvista, drawn with Fusion", Qt::ColorScheme::Dark, "windowsvista"},
};

/** A theme that chooses color for role in the dark scheme, or in the light scheme if darkScheme is false. */
[[nodiscard]] inline domain::Theme themeWith(domain::ThemeColorRole role, bool darkScheme, domain::RgbColor color)
{
    domain::Theme theme;
    (darkScheme ? theme.dark : theme.light)[role] = color;
    return theme;
}

/**
 * Makes theme the colours that the user chose while it exists, as
 * ui::ThemeController does, and clears them afterwards.
 */
class ScopedCustomColors
{
public:
    explicit ScopedCustomColors(const domain::Theme& theme)
    {
        ui::theme::setCustomColors(theme);
    }

    ~ScopedCustomColors()
    {
        ui::theme::setCustomColors(domain::Theme{});
    }

    ScopedCustomColors(const ScopedCustomColors&) = delete;
    ScopedCustomColors& operator=(const ScopedCustomColors&) = delete;
    ScopedCustomColors(ScopedCustomColors&&) = delete;
    ScopedCustomColors& operator=(ScopedCustomColors&&) = delete;
};

/**
 * Draws the application in appearance while it exists, as the monitor does:
 * the style, then the scheme, then a ui::ThemeController, which replaces a
 * style that draws only light. They are undone in reverse.
 */
class ScopedAppearance
{
public:
    explicit ScopedAppearance(const Appearance& appearance)
        : m_style(appearance.style), m_colorScheme(appearance.scheme)
    {}

private:
    ScopedStyle m_style;
    ScopedColorScheme m_colorScheme;
    ui::ThemeController m_controller;
};

} // namespace verdandi::tests
