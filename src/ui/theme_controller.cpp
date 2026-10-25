#include "ui/theme_controller.h"

#include <initializer_list>

#include <QApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleHints>

#include "domain/theme.h"
#include "ui/theme/theme_colors.h"

namespace verdandi::ui
{

namespace
{

// Qt's style for Windows 10, which draws only the light scheme: in the dark
// scheme it gives the application the light palette.
const QString kLightOnlyStyle = "windowsvista";
// Qt's built-in style that draws the dark palette.
const QString kDarkStyle = "fusion";

// The placeholder text in text boxes is the text colour at this opacity, as
// in Qt's palettes for Windows.
constexpr float kPlaceholderTextAlpha = 0.5f;

bool isStyle(const QString& name)
{
    return QApplication::style()->name().compare(name, Qt::CaseInsensitive) == 0;
}

// Sets the palette roles of the colours that the user chose for a scheme
// (DC-151). The disabled colour group keeps the scheme's dimmed text and
// accent, and the inactive one its unfocused selection.
void setChosenColors(QPalette& palette, const domain::ThemeColors& colors)
{
    if (const auto& window = colors[domain::ThemeColorRole::Window]) {
        palette.setColor(QPalette::Window, theme::toQColor(*window));
    }
    if (const auto& page = colors[domain::ThemeColorRole::Page]) {
        palette.setColor(QPalette::Base, theme::toQColor(*page));
    }
    if (const auto& text = colors[domain::ThemeColorRole::Text]) {
        const QColor color = theme::toQColor(*text);
        QColor placeholder = color;
        placeholder.setAlphaF(kPlaceholderTextAlpha);
        for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
            for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
                palette.setColor(group, role, color);
            }
            palette.setColor(group, QPalette::PlaceholderText, placeholder);
        }
    }
    if (const auto& accent = colors[domain::ThemeColorRole::Accent]) {
        const QColor color = theme::toQColor(*accent);
        for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
            palette.setColor(group, QPalette::Accent, color);
        }
        palette.setColor(QPalette::Active, QPalette::Highlight, color);
        palette.setColor(QPalette::Active, QPalette::HighlightedText,
                         theme::isDark(color) ? QColor(Qt::white) : QColor(Qt::black));
    }
}

} // namespace

ThemeController::ThemeController(QObject* parent) : QObject(parent)
{
    // QStyleHints emits only when the scheme changes, whether Windows changed
    // it or a contrast theme was turned on or off. It emits before Qt gives
    // the application the new scheme's palette, which Qt resolves against the
    // style and the palette that applyColorScheme() leaves.
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
    // A palette with no roles set gives back the Windows theme's palettes,
    // per-class palettes included.
    QApplication::setPalette(QPalette());
    restoreStyle();
    theme::setCustomColors(domain::Theme{});
}

Qt::ColorScheme ThemeController::colorScheme() const
{
    return QGuiApplication::styleHints()->colorScheme();
}

void ThemeController::setSettings(const domain::Settings& settings)
{
    const bool colorsChanged = settings.theme != m_theme;
    m_theme = settings.theme;

    const Qt::ColorScheme drawn = colorScheme();
    if (settings.colorScheme != m_colorSchemeSetting) {
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
    // A change of scheme has applied the colours already.
    if (colorsChanged && colorScheme() == drawn) {
        applyColors();
    }
}

void ThemeController::applyColorScheme(Qt::ColorScheme scheme)
{
    if (scheme != Qt::ColorScheme::Dark) {
        restoreStyle();
    } else if (m_replacedStyle.isEmpty() && isStyle(kLightOnlyStyle)) {
        m_replacedStyle = QApplication::style()->name();
        QApplication::setStyle(kDarkStyle);
    }
    applyColors();
}

void ThemeController::restoreStyle()
{
    if (m_replacedStyle.isEmpty()) {
        return;
    }
    QApplication::setStyle(m_replacedStyle);
    m_replacedStyle.clear();
}

void ThemeController::applyColors()
{
    // Starts again from the scheme's own palette, so that every role that is
    // not set below, including one the user no longer chooses, takes the
    // scheme's colour.
    QApplication::setPalette(QPalette());
    const QPalette own = QApplication::palette();
    const bool isContrastTheme = colorScheme() == Qt::ColorScheme::Unknown;

    QPalette palette;
    if (!m_replacedStyle.isEmpty()) {
        // Setting any role of the application palette drops the Windows
        // theme's per-class palettes, which are made for the Windows styles:
        // Fusion would fill unchecked check boxes with the check box palette's
        // accent-coloured base. The window and base colours are those of the
        // dark palette that Qt resolved for Fusion (DC-148).
        palette.setColor(QPalette::Window, own.color(QPalette::Window));
        palette.setColor(QPalette::Base, own.color(QPalette::Base));
    }
    // A contrast theme's colours are the user's too, and replace any others.
    if (!isContrastTheme) {
        setChosenColors(palette, m_theme.colors(colorScheme() == Qt::ColorScheme::Dark));
    }
    QApplication::setPalette(palette);
    // After the palette, so that the widgets that take the user's colours
    // again pick them by the new palette.
    theme::setCustomColors(isContrastTheme ? domain::Theme{} : m_theme);
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
