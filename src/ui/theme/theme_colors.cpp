#include "ui/theme/theme_colors.h"

#include <cassert>

#include <QCoreApplication>
#include <QPointer>

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
std::optional<Shades> shadesOf(ThemeColorRole role)
{
    switch (role) {
        // The palette and the style give these, and the cards take the pages' colour.
        case ThemeColorRole::Window:
        case ThemeColorRole::Page:
        case ThemeColorRole::Card:
        case ThemeColorRole::Text:
        case ThemeColorRole::Accent:
        case ThemeColorRole::TableHeader:
            return std::nullopt;
        // Categorical hues for a chart's series, in an order checked for
        // separation under colour-vision deficiency on both surfaces: blue,
        // orange, aqua, violet and magenta. Red, amber and green are left
        // out, because they are the status colours.
        case ThemeColorRole::ChartLine1:
            return Shades{QColor{0x2a, 0x78, 0xd6}, QColor{0x39, 0x87, 0xe5}};
        case ThemeColorRole::ChartLine2:
            return Shades{QColor{0xeb, 0x68, 0x34}, QColor{0xd9, 0x59, 0x26}};
        case ThemeColorRole::ChartLine3:
            return Shades{QColor{0x1b, 0xaf, 0x7a}, QColor{0x19, 0x9e, 0x70}};
        case ThemeColorRole::ChartLine4:
            return Shades{QColor{0x4a, 0x3a, 0xa7}, QColor{0x90, 0x85, 0xe9}};
        case ThemeColorRole::ChartLine5:
            return Shades{QColor{0xe8, 0x7b, 0xa4}, QColor{0xd5, 0x51, 0x81}};
        // The scheme's text colour at 15% opacity, so the grid stays recessive.
        case ThemeColorRole::ChartGrid:
            return Shades{QColor{0x00, 0x00, 0x00, 0x26}, QColor{0xff, 0xff, 0xff, 0x26}};
        // No green, amber or red reaches 4.5:1 on both white and #2d2d2d, so
        // each status has a darker shade for light surfaces and a lighter one
        // for dark surfaces.
        case ThemeColorRole::Good:
            return Shades{QColor{0x1e, 0x7b, 0x45}, QColor{0x3c, 0xb3, 0x71}};
        case ThemeColorRole::Warning:
            return Shades{QColor{0xa0, 0x5a, 0x00}, QColor{0xd9, 0x82, 0x00}};
        case ThemeColorRole::Critical:
            return Shades{QColor{0xc6, 0x28, 0x28}, QColor{0xff, 0x6b, 0x6b}};
        // Translucent, so that they tint the rows of either scheme and keep
        // the text on them readable.
        case ThemeColorRole::ProcessStarted:
            return Shades{QColor{46, 160, 67, 90}, QColor{46, 160, 67, 90}};
        case ThemeColorRole::ProcessExited:
            return Shades{QColor{218, 54, 51, 90}, QColor{218, 54, 51, 90}};
        // Mid tones that reach 3:1 on both schemes, except the padlock's
        // amber, which needs the darker Warning shade on light surfaces.
        case ThemeColorRole::AccessDenied:
            return Shades{QColor{0xa0, 0x5a, 0x00}, QColor{208, 138, 16}};
        case ThemeColorRole::AccessNeverOpened:
            return Shades{QColor{61, 127, 201}, QColor{61, 127, 201}};
        case ThemeColorRole::AccessUnavailable:
        default:
            return Shades{QColor{140, 140, 140}, QColor{140, 140, 140}};
    }
}

// The theme's and the user's colours for the scheme that the application draws
// in, for the whole application, like its palette.
domain::Theme& storedCustomColors()
{
    static domain::Theme colors;
    return colors;
}

} // namespace

bool isDark(const QColor& background)
{
    return background.lightnessF() < 0.5f;
}

QColor contrastingColor(const QColor& background, const QColor& first, const QColor& second)
{
    const bool firstIsLighter = first.lightnessF() > second.lightnessF();
    return isDark(background) == firstIsLighter ? first : second;
}

std::optional<QColor> defaultColor(domain::ThemeColorRole role, bool darkSurface)
{
    const std::optional<Shades> shades = shadesOf(role);
    if (!shades) {
        return std::nullopt;
    }
    return darkSurface ? shades->onDark : shades->onLight;
}

std::optional<QColor> customColor(domain::ThemeColorRole role, bool darkScheme)
{
    const domain::Theme& theme = storedCustomColors();
    if ((theme.scheme == domain::ColorScheme::Dark) != darkScheme) {
        return std::nullopt;
    }
    const std::optional<domain::RgbColor>& chosen = theme.colors[role];
    return chosen ? std::optional{toQColor(*chosen)} : std::nullopt;
}

QColor color(domain::ThemeColorRole role, bool darkSurface)
{
    const std::optional<QColor> builtIn = defaultColor(role, darkSurface);
    assert(builtIn && "color() takes a role that has a built-in colour");
    const QColor fallback = builtIn.value_or(QColor{});
    std::optional<QColor> chosen = customColor(role, darkSurface);
    if (!chosen) {
        return fallback;
    }
    // A translucent role, such as the grid or a highlight, stays as translucent.
    chosen->setAlpha(fallback.alpha());
    return *chosen;
}

QColor color(domain::ThemeColorRole role, const QColor& background)
{
    return color(role, isDark(background));
}

const domain::Theme& customColors()
{
    return storedCustomColors();
}

void setCustomColors(const domain::Theme& theme)
{
    if (theme == storedCustomColors()) {
        return;
    }
    storedCustomColors() = theme;
    emit ThemeNotifier::instance() -> colorsChanged();
}

QColor toQColor(domain::RgbColor color)
{
    return QColor{color.red, color.green, color.blue};
}

domain::RgbColor toRgbColor(const QColor& color)
{
    const QColor rgb = color.toRgb();
    return domain::RgbColor{
        .red = static_cast<std::uint8_t>(rgb.red()),
        .green = static_cast<std::uint8_t>(rgb.green()),
        .blue = static_cast<std::uint8_t>(rgb.blue()),
    };
}

ThemeNotifier::ThemeNotifier(QObject* parent) : QObject(parent) {}

ThemeNotifier* ThemeNotifier::instance()
{
    // The application owns it, so it is destroyed with the application, and
    // a later application gets a new one.
    static QPointer<ThemeNotifier> notifier;
    if (notifier.isNull()) {
        notifier = new ThemeNotifier(QCoreApplication::instance());
    }
    return notifier.data();
}

} // namespace verdandi::ui::theme
