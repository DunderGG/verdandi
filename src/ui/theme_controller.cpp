#include "ui/theme_controller.h"

#include <QApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleHints>

namespace verdandi::ui
{

namespace
{

// Qt's style for Windows 10, which draws only the light scheme: in the dark
// scheme it gives the application the light palette.
const QString kLightOnlyStyle = "windowsvista";
// Qt's built-in style that draws the dark palette.
const QString kDarkStyle = "fusion";

bool isStyle(const QString& name)
{
    return QApplication::style()->name().compare(name, Qt::CaseInsensitive) == 0;
}

} // namespace

ThemeController::ThemeController(QObject* parent) : QObject(parent)
{
    // QStyleHints emits only when the scheme changes, whether Windows changed
    // it or a contrast theme was turned on or off. It emits before Qt gives
    // the application the new scheme's palette, which Qt resolves against the
    // style that applyColorScheme() leaves.
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme scheme) {
        applyColorScheme(scheme);
        emit colorSchemeChanged(scheme);
    });
    applyColorScheme(colorScheme());
}

ThemeController::~ThemeController()
{
    // Gives back Windows' scheme without following it, and without saying so.
    disconnect(QGuiApplication::styleHints(), nullptr, this, nullptr);
    if (m_colorSchemeSetting != domain::ColorScheme::System) {
        QGuiApplication::styleHints()->unsetColorScheme();
    }
    restoreStyle();
}

Qt::ColorScheme ThemeController::colorScheme() const
{
    return QGuiApplication::styleHints()->colorScheme();
}

void ThemeController::setSettings(const domain::Settings& settings)
{
    if (settings.colorScheme == m_colorSchemeSetting) {
        return;
    }
    m_colorSchemeSetting = settings.colorScheme;
    // Qt emits colorSchemeChanged() at once if the scheme it draws in changes.
    switch (settings.colorScheme) {
        case domain::ColorScheme::Light:
            QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
            break;
        case domain::ColorScheme::Dark:
            QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
            break;
        case domain::ColorScheme::System:
        default:
            QGuiApplication::styleHints()->unsetColorScheme();
            break;
    }
}

void ThemeController::applyColorScheme(Qt::ColorScheme scheme)
{
    if (scheme != Qt::ColorScheme::Dark) {
        restoreStyle();
        return;
    }
    if (!m_replacedStyle.isEmpty() || !isStyle(kLightOnlyStyle)) {
        return;
    }

    m_replacedStyle = QApplication::style()->name();
    QApplication::setStyle(kDarkStyle);
    // Setting any role of the application palette drops the Windows theme's
    // per-class palettes, which are made for the Windows styles: Fusion would
    // fill unchecked check boxes with the check box palette's accent-coloured
    // base. The window and base colours are those of the dark palette that Qt
    // has just resolved for Fusion, which the text colours are checked against.
    const QPalette dark = QApplication::palette();
    QPalette surfaces;
    surfaces.setColor(QPalette::Window, dark.color(QPalette::Window));
    surfaces.setColor(QPalette::Base, dark.color(QPalette::Base));
    QApplication::setPalette(surfaces);
}

void ThemeController::restoreStyle()
{
    if (m_replacedStyle.isEmpty()) {
        return;
    }
    // A palette with no roles set gives back the Windows theme's palettes,
    // per-class palettes included, before the style returns.
    QApplication::setPalette(QPalette());
    QApplication::setStyle(m_replacedStyle);
    m_replacedStyle.clear();
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
