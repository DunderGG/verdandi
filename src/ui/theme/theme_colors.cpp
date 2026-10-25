#include "ui/theme/theme_colors.h"

namespace verdandi::ui::theme
{

namespace
{

using domain::ThemeColorRole;

// A role's colour on a light surface and on a dark surface.
struct Shades
{
    QColor onLight;
    QColor onDark;
};

// The surfaces are the window and base colours of Qt's light palette with the
// windows11 style (#f3f3f3 and #ffffff) and of its dark palette (#1e1e1e and
// #2d2d2d), which Fusion draws too (docs/coding_guidelines.md, "Text colours").
Shades shadesOf(ThemeColorRole role)
{
    switch (role) {
        // Categorical hues for a chart's series, in an order checked for
        // separation under colour-vision deficiency on both surfaces: blue,
        // orange, aqua, violet and magenta. Red, amber and green are left
        // out, because they are the status colours.
        case ThemeColorRole::ChartLine1:
            return {QColor{0x2a, 0x78, 0xd6}, QColor{0x39, 0x87, 0xe5}};
        case ThemeColorRole::ChartLine2:
            return {QColor{0xeb, 0x68, 0x34}, QColor{0xd9, 0x59, 0x26}};
        case ThemeColorRole::ChartLine3:
            return {QColor{0x1b, 0xaf, 0x7a}, QColor{0x19, 0x9e, 0x70}};
        case ThemeColorRole::ChartLine4:
            return {QColor{0x4a, 0x3a, 0xa7}, QColor{0x90, 0x85, 0xe9}};
        case ThemeColorRole::ChartLine5:
            return {QColor{0xe8, 0x7b, 0xa4}, QColor{0xd5, 0x51, 0x81}};
        // The scheme's text colour at 15% opacity, so the grid stays recessive.
        case ThemeColorRole::ChartGrid:
            return {QColor{0x00, 0x00, 0x00, 0x26}, QColor{0xff, 0xff, 0xff, 0x26}};
        // No green, amber or red reaches 4.5:1 on both white and #2d2d2d, so
        // each status has a darker shade for light surfaces and a lighter one
        // for dark surfaces.
        case ThemeColorRole::Good:
            return {QColor{0x1e, 0x7b, 0x45}, QColor{0x3c, 0xb3, 0x71}};
        case ThemeColorRole::Warning:
            return {QColor{0xa0, 0x5a, 0x00}, QColor{0xd9, 0x82, 0x00}};
        case ThemeColorRole::Critical:
            return {QColor{0xc6, 0x28, 0x28}, QColor{0xff, 0x6b, 0x6b}};
        // Translucent, so that they tint the rows of either scheme and keep
        // the text on them readable.
        case ThemeColorRole::ProcessStarted:
            return {QColor{46, 160, 67, 90}, QColor{46, 160, 67, 90}};
        case ThemeColorRole::ProcessExited:
            return {QColor{218, 54, 51, 90}, QColor{218, 54, 51, 90}};
        // Mid tones that reach 3:1 on both schemes, except the padlock's
        // amber, which needs the darker Warning shade on light surfaces.
        case ThemeColorRole::AccessDenied:
            return {QColor{0xa0, 0x5a, 0x00}, QColor{208, 138, 16}};
        case ThemeColorRole::AccessNeverOpened:
            return {QColor{61, 127, 201}, QColor{61, 127, 201}};
        case ThemeColorRole::AccessUnavailable:
        default:
            return {QColor{140, 140, 140}, QColor{140, 140, 140}};
    }
}

} // namespace

bool isDark(const QColor& background)
{
    return background.lightnessF() < 0.5f;
}

QColor color(domain::ThemeColorRole role, bool darkSurface)
{
    const Shades shades = shadesOf(role);
    return darkSurface ? shades.onDark : shades.onLight;
}

QColor color(domain::ThemeColorRole role, const QColor& background)
{
    return color(role, isDark(background));
}

} // namespace verdandi::ui::theme
