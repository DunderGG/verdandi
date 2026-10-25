#pragma once

#include <string_view>

#include <QObject>
#include <QString>

#include "domain/settings.h"

namespace verdandi::ui
{

/**
 * Follows the colour scheme that the application draws in (DC-147). Qt reads
 * the one that Windows sets for applications when the QApplication starts,
 * and again whenever Windows changes it, and its windows11 style and the
 * windows' title bars follow it. The scheme is Qt::ColorScheme::Light or
 * Dark, or Unknown while a Windows contrast theme is on, whose own colours
 * then replace both.
 *
 * The settings can choose light or dark whatever Windows sets (DC-149):
 * setSettings() asks Qt for that scheme, which Qt then draws in as if Windows
 * had set it, until the settings choose Windows' scheme again or the
 * controller is destroyed. A contrast theme still replaces it.
 *
 * Qt's windowsvista style, which it picks on Windows 10, draws only the light
 * scheme. While the scheme is dark, the controller replaces it with Fusion,
 * which draws the dark palette, and makes the palette's window and base
 * colours explicit, so that Fusion draws the widgets with the application's
 * palette rather than the Windows theme's per-class palettes (DC-148). When
 * the scheme turns light, and when the controller is destroyed, it gives the
 * application back its style and palette.
 *
 * One controller serves the whole application: main.cpp creates it before
 * the main window, so that it outlives every window.
 *
 * Not thread-safe; use only on the UI thread.
 */
class ThemeController : public QObject
{
    Q_OBJECT

public:
    /** Requires a QApplication. Replaces the style at once if the scheme is dark and the style draws only light. */
    explicit ThemeController(QObject* parent = nullptr);
    ~ThemeController() override;

    /** Returns the scheme that the application draws in now. */
    [[nodiscard]] Qt::ColorScheme colorScheme() const;

    /**
     * Draws the application in the colour scheme that settings choose. Asks
     * Qt only when the choice differs from the previous settings', since Qt
     * then gives every widget its style again. Windows' scheme until set.
     */
    void setSettings(const domain::Settings& settings);

signals:
    /** Emitted with the new scheme when the scheme that the application draws in changes. */
    void colorSchemeChanged(Qt::ColorScheme scheme);

private:
    // Draws the application in scheme: Fusion in place of a style that draws
    // only light while scheme is dark, and the application's own style otherwise.
    void applyColorScheme(Qt::ColorScheme scheme);
    // Gives the application back the style that Fusion replaced, and its palette.
    void restoreStyle();

    // The name of the style that Fusion replaces while the scheme is dark; empty while none is replaced.
    QString m_replacedStyle;
    domain::ColorScheme m_colorSchemeSetting{domain::ColorScheme::System};
};

/**
 * Returns how the log names scheme: "the light colour scheme", "the dark
 * colour scheme", or "a contrast theme" for Qt::ColorScheme::Unknown.
 */
[[nodiscard]] std::string_view colorSchemeName(Qt::ColorScheme scheme);

} // namespace verdandi::ui
