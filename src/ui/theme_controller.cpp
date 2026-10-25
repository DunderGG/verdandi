#include "ui/theme_controller.h"

#include <QGuiApplication>
#include <QStyleHints>

namespace verdandi::ui
{

ThemeController::ThemeController(QObject* parent) : QObject(parent)
{
    // QStyleHints emits only when the scheme changes, whether Windows changed
    // it or a contrast theme was turned on or off.
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            &ThemeController::colorSchemeChanged);
}

Qt::ColorScheme ThemeController::colorScheme() const
{
    return QGuiApplication::styleHints()->colorScheme();
}

std::string_view colorSchemeName(Qt::ColorScheme scheme)
{
    switch (scheme) {
        case Qt::ColorScheme::Light:
            return "the light colour scheme";
        case Qt::ColorScheme::Dark:
            return "the dark colour scheme";
        case Qt::ColorScheme::Unknown:
        default:
            // Qt's Windows theme reports Unknown only while a contrast theme is on.
            return "a contrast theme";
    }
}

} // namespace verdandi::ui
