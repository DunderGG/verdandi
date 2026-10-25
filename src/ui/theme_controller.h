#pragma once

#include <array>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <QObject>
#include <QPalette>
#include <QString>

#include "domain/settings.h"
#include "domain/theme.h"

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
 * The settings choose a theme by name (DC-149): Follow Windows, the default,
 * draws with Light or Dark as Windows sets, and Light, Dark or a theme of the
 * themes folder (DC-153) keeps its scheme whatever Windows sets. For those,
 * setSettings() asks Qt for the theme's scheme, which Qt then draws in as if
 * Windows had set it, until the settings choose Follow Windows again or the
 * controller is destroyed. A contrast theme still replaces it. A theme that
 * the themes hold no theme of that name for draws as Follow Windows.
 *
 * Qt's windowsvista style, which it picks on Windows 10, draws only the light
 * scheme. While the scheme is dark, the controller replaces it with Fusion,
 * which draws the dark palette, and makes the palette's window and base
 * colours explicit, so that Fusion draws the widgets with the application's
 * palette rather than the Windows theme's per-class palettes (DC-148).
 *
 * A theme of the themes folder chooses colours of its own, and the settings
 * hold the colours that the user chose in each theme in place of its own
 * (DC-151). For the scheme it draws in, the controller sets the palette's
 * window, base, text and accent colours that they choose, and gives
 * ui::theme every chosen colour, which the widgets that draw their own take.
 * A contrast theme replaces them all. When the scheme changes, it starts
 * again from the new scheme's palette.
 *
 * When it is destroyed, the controller gives the application back Windows'
 * scheme, its style and its palette, and clears the chosen colours. One
 * controller serves the whole application: main.cpp creates it before the
 * main window, so that it outlives every window.
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
     * Returns the scheme that Windows sets for apps, as Qt last reported it
     * while the application followed it: when the controller was created,
     * and at each change since while the settings chose Follow Windows. Qt
     * reports none while another theme is drawn. Light until a contrast
     * theme that was on at the start is turned off.
     */
    [[nodiscard]] domain::ColorScheme windowsColorScheme() const;

    /**
     * Returns Windows' own palette for scheme, without a theme's or the
     * user's colours: as the application last drew it in that scheme
     * without a contrast theme. For a scheme it has not drawn in, it is the
     * palette of the one it has, or the application's, with the window, base
     * and text colours of Qt's palette for scheme with the windows11 style,
     * which stand for Windows'.
     */
    [[nodiscard]] QPalette windowsPalette(domain::ColorScheme scheme) const;

    /**
     * Makes themes, from the themes folder, the themes that the settings can
     * choose besides the built-in ones, ordered by name whatever its case,
     * and draws with the theme in use again if it changed. None until set.
     */
    void setThemes(std::vector<domain::NamedTheme> themes);

    /** Adds theme to the themes, in place of the one of the same name if there is one, as setThemes() would. */
    void addTheme(const domain::NamedTheme& theme);

    /** Returns the themes, ordered by name whatever its case. */
    [[nodiscard]] const std::vector<domain::NamedTheme>& themes() const;

    /**
     * Returns the name of the theme that the application draws with: the
     * settings' theme, or Follow Windows while the themes hold none of that
     * name.
     */
    [[nodiscard]] const std::string& themeName() const;

    /**
     * Draws the application with the theme that settings choose, in its
     * colour scheme and its colours with the user's in place of its own.
     * Asks Qt for the scheme only when it differs from the previous
     * settings', since Qt then gives every widget its style again, and sets
     * the colours only when they differ. Follow Windows until set.
     */
    void setSettings(const domain::Settings& settings);

signals:
    /** Emitted with the new scheme when the scheme that the application draws in changes. */
    void colorSchemeChanged(Qt::ColorScheme scheme);

private:
    // Draws the application in scheme: Fusion in place of a style that draws
    // only light while scheme is dark, and the application's own style
    // otherwise, and the colours chosen for scheme.
    void applyColorScheme(Qt::ColorScheme scheme);
    // Gives the application back the style that Fusion replaced.
    void restoreStyle();
    // Sets the application palette from the scheme's own and the colours
    // chosen for it, and gives ui::theme the chosen colours.
    void applyColors();
    // Draws with the theme that the settings choose, from the themes.
    void applyTheme();
    // Keeps scheme, which Qt reports while the application follows Windows, as Windows' scheme.
    void noteWindowsColorScheme(Qt::ColorScheme scheme);

    // The name of the style that Fusion replaces while the scheme is dark; empty while none is replaced.
    QString m_replacedStyle;
    std::vector<domain::NamedTheme> m_themes;
    // The theme that the settings choose, and the colours that the user chose in each theme.
    std::string m_chosenTheme{domain::kFollowWindowsTheme};
    std::map<std::string, domain::ThemeColors, std::less<>> m_userColors;
    // What applyTheme() resolved: the theme drawn with, the scheme asked of
    // Qt, std::nullopt while following Windows, and the colours chosen in
    // each scheme.
    std::string m_themeName{domain::kFollowWindowsTheme};
    std::optional<domain::ColorScheme> m_requestedScheme;
    domain::ThemeColors m_lightColors{};
    domain::ThemeColors m_darkColors{};
    domain::ColorScheme m_windowsColorScheme{domain::ColorScheme::Light};
    // Windows' own palettes of the light and the dark scheme, once drawn in.
    std::array<std::optional<QPalette>, 2> m_windowsPalettes{};
};

/**
 * Returns how the log names scheme: "the light colour scheme", "the dark
 * colour scheme", or "a contrast theme" for Qt::ColorScheme::Unknown.
 */
[[nodiscard]] std::string_view colorSchemeName(Qt::ColorScheme scheme);

} // namespace verdandi::ui
