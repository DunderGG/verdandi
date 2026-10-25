#pragma once

#include <QColor>
#include <QIcon>

#include "domain/process_info.h"

namespace verdandi::ui
{

/**
 * Returns the small icon shown before a process's name for its details
 * status, for rows on a dark or a light surface: an amber padlock when Windows
 * denied access, a blue shield for a process the monitor never opens, and a
 * grey circle with a bar when there are no details to read. A process whose
 * details were read gets a transparent icon of the same size, so all names
 * line up.
 *
 * The icons are painted on a 16-pixel grid at 1x, 1.5x, and 2x scale, in the
 * theme's access colours for the surface (DC-150), which reach 3:1 on it.
 * Each call paints new pixmaps, so callers keep the icons. Requires a
 * QGuiApplication.
 */
[[nodiscard]] QIcon detailsStatusIcon(domain::ProcessDetailsStatus status, bool darkSurface);

/** Returns the icon for status, as detailsStatusIcon() paints it, in color, such as one that the user has not applied
 * yet. */
[[nodiscard]] QIcon detailsStatusIcon(domain::ProcessDetailsStatus status, const QColor& color);

} // namespace verdandi::ui
