#pragma once

#include <QColor>

#include "domain/theme.h"

namespace verdandi::ui::theme
{

/** Returns whether background is a dark surface: whether its lightness is below half. */
[[nodiscard]] bool isDark(const QColor& background);

/**
 * Returns the colour of role on a dark or a light surface (DC-150). Each role
 * has a shade for each: the status colours reach WCAG's 4.5:1 for text, and
 * the access icons its 3:1 for graphics, on the window and base colours of
 * the scheme that the surface belongs to.
 */
[[nodiscard]] QColor color(domain::ThemeColorRole role, bool darkSurface);

/**
 * Returns the colour of role on background, by whether background is dark.
 * A widget calls it when it paints, or whenever its palette changes, with
 * the colour it is drawn on, so its colours follow a change of scheme.
 */
[[nodiscard]] QColor color(domain::ThemeColorRole role, const QColor& background);

} // namespace verdandi::ui::theme
