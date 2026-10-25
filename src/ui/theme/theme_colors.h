#pragma once

#include <optional>

#include <QColor>
#include <QObject>

#include "domain/theme.h"

namespace verdandi::ui::theme
{

/** Returns whether background is a dark surface: whether its lightness is below half. */
[[nodiscard]] bool isDark(const QColor& background);

/**
 * Returns the one of first and second that stands out more on background:
 * the lighter on a dark background, and the darker on a light one, by
 * lightness. Text on a chosen colour, such as a table header's, takes the
 * theme's text or window colour this way.
 */
[[nodiscard]] QColor contrastingColor(const QColor& background, const QColor& first, const QColor& second);

/**
 * Returns the built-in colour of role on a dark or a light surface (DC-150),
 * or std::nullopt for Window, Page, Card, Text, Accent and TableHeader, which
 * the palette and the style give. Each role that has one has a shade for each surface: the status
 * colours reach WCAG's 4.5:1 for text, and the access icons its 3:1 for
 * graphics, on the window and base colours of the scheme that the surface
 * belongs to.
 */
[[nodiscard]] std::optional<QColor> defaultColor(domain::ThemeColorRole role, bool darkSurface);

/**
 * Returns the colour that the theme in use, or the user, chose for role, if
 * any (setCustomColors()), when darkScheme says the scheme that the theme
 * draws in, and std::nullopt for the other scheme.
 */
[[nodiscard]] std::optional<QColor> customColor(domain::ThemeColorRole role, bool darkScheme);

/**
 * Returns the colour of role on a dark or a light surface: the chosen colour
 * (customColor()), with the built-in colour's opacity, or the built-in
 * colour. role must have a built-in colour (defaultColor()).
 */
[[nodiscard]] QColor color(domain::ThemeColorRole role, bool darkSurface);

/**
 * Returns the colour of role on background, by whether background is dark.
 * A widget calls it when it paints, or whenever its palette changes or
 * ThemeNotifier::colorsChanged() is emitted, with the colour it is drawn on,
 * so its colours follow a change of scheme and of the chosen colours.
 */
[[nodiscard]] QColor color(domain::ThemeColorRole role, const QColor& background);

/** Returns the chosen colours, which customColor() and color() give. The Light theme's, none, until set. */
[[nodiscard]] const domain::Theme& customColors();

/**
 * Makes theme's colours, in its scheme, the chosen colours for the whole
 * application (DC-151): the colours of the theme in use, with the user's in
 * place of its own. Emits ThemeNotifier::colorsChanged() if they differ from
 * the previous ones. ui::ThemeController sets them. Use only on the UI thread.
 */
void setCustomColors(const domain::Theme& theme);

/** Returns color as Qt names it. */
[[nodiscard]] QColor toQColor(domain::RgbColor color);

/** Returns color without its opacity. */
[[nodiscard]] domain::RgbColor toRgbColor(const QColor& color);

/**
 * Tells the widgets that keep a colour of the theme, or need painting again,
 * that the chosen colours have changed (DC-151). One notifier serves the
 * application, which owns it, as it owns QStyleHints.
 *
 * Not thread-safe; use only on the UI thread.
 */
class ThemeNotifier : public QObject
{
    Q_OBJECT

public:
    /** Returns the application's notifier. Requires a QCoreApplication. */
    [[nodiscard]] static ThemeNotifier* instance();

signals:
    /** Emitted after setCustomColors() changes the chosen colours. */
    void colorsChanged();

private:
    explicit ThemeNotifier(QObject* parent);
};

} // namespace verdandi::ui::theme
