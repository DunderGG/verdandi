#include "ui/theme_controller.h"

#include <algorithm>
#include <initializer_list>
#include <utility>

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
        if (!m_requestedScheme) {
            noteWindowsColorScheme(scheme);
        }
        applyColorScheme(scheme);
        emit colorSchemeChanged(scheme);
    });
    // Before any settings ask Qt for a scheme, Qt reports Windows' own.
    noteWindowsColorScheme(colorScheme());
    applyColorScheme(colorScheme());
}

ThemeController::~ThemeController()
{
    // Gives back Windows' scheme without following it, and without saying so.
    disconnect(QGuiApplication::styleHints(), nullptr, this, nullptr);
    if (m_requestedScheme) {
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

domain::ColorScheme ThemeController::windowsColorScheme() const
{
    return m_windowsColorScheme;
}

void ThemeController::setThemes(std::vector<domain::NamedTheme> themes)
{
    m_themes = std::move(themes);
    std::ranges::sort(m_themes, [](const domain::NamedTheme& first, const domain::NamedTheme& second) {
        return QString::fromStdString(first.name).compare(QString::fromStdString(second.name), Qt::CaseInsensitive) < 0;
    });
    applyTheme();
}

void ThemeController::addTheme(const domain::NamedTheme& theme)
{
    std::vector<domain::NamedTheme> themes = m_themes;
    std::erase_if(themes, [&theme](const domain::NamedTheme& existing) { return existing.name == theme.name; });
    themes.push_back(theme);
    setThemes(std::move(themes));
}

const std::vector<domain::NamedTheme>& ThemeController::themes() const
{
    return m_themes;
}

const std::string& ThemeController::themeName() const
{
    return m_themeName;
}

void ThemeController::setSettings(const domain::Settings& settings)
{
    m_chosenTheme = settings.theme;
    m_userColors = settings.themeColors;
    applyTheme();
}

void ThemeController::applyTheme()
{
    const auto userColors = [this](std::string_view theme) {
        const auto colors = m_userColors.find(theme);
        return colors != m_userColors.end() ? colors->second : domain::ThemeColors{};
    };
    // Follow Windows and the built-in themes draw with the user's colours in
    // Light and Dark, since Windows can change between them.
    std::string themeName{domain::kFollowWindowsTheme};
    std::optional<domain::ColorScheme> scheme;
    domain::ThemeColors lightColors = userColors(domain::kLightTheme);
    domain::ThemeColors darkColors = userColors(domain::kDarkTheme);
    if (m_chosenTheme == domain::kLightTheme || m_chosenTheme == domain::kDarkTheme) {
        themeName = m_chosenTheme;
        scheme = m_chosenTheme == domain::kDarkTheme ? domain::ColorScheme::Dark : domain::ColorScheme::Light;
    } else if (const auto theme = std::ranges::find(m_themes, m_chosenTheme, &domain::NamedTheme::name);
               theme != m_themes.end()) {
        themeName = theme->name;
        scheme = theme->theme.scheme;
        const domain::ThemeColors colors = theme->theme.colors.withChanges(userColors(theme->name));
        lightColors = *scheme == domain::ColorScheme::Light ? colors : domain::ThemeColors{};
        darkColors = *scheme == domain::ColorScheme::Dark ? colors : domain::ThemeColors{};
    }

    m_themeName = std::move(themeName);
    const bool colorsChanged = lightColors != m_lightColors || darkColors != m_darkColors;
    m_lightColors = lightColors;
    m_darkColors = darkColors;

    const Qt::ColorScheme drawn = colorScheme();
    if (scheme != m_requestedScheme) {
        m_requestedScheme = scheme;
        // Qt emits colorSchemeChanged() at once if the scheme it draws in changes.
        if (!scheme) {
            QGuiApplication::styleHints()->unsetColorScheme();
            noteWindowsColorScheme(colorScheme());
        } else {
            QGuiApplication::styleHints()->setColorScheme(
                *scheme == domain::ColorScheme::Dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
        }
    }
    // A change of scheme has applied the colours already.
    if (colorsChanged && colorScheme() == drawn) {
        applyColors();
    }
}

void ThemeController::noteWindowsColorScheme(Qt::ColorScheme scheme)
{
    // A contrast theme hides which scheme Windows sets for apps.
    if (scheme != Qt::ColorScheme::Unknown) {
        m_windowsColorScheme = scheme == Qt::ColorScheme::Dark ? domain::ColorScheme::Dark : domain::ColorScheme::Light;
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
    const bool isDark = colorScheme() == Qt::ColorScheme::Dark;
    const domain::ThemeColors& colors = isDark ? m_darkColors : m_lightColors;
    // A contrast theme's colours are the user's too, and replace any others.
    if (!isContrastTheme) {
        setChosenColors(palette, colors);
    }
    QApplication::setPalette(palette);
    // After the palette, so that the widgets that take the chosen colours
    // again pick them by the new palette.
    theme::setCustomColors(
        isContrastTheme ? domain::Theme{}
                        : domain::Theme{.scheme = isDark ? domain::ColorScheme::Dark : domain::ColorScheme::Light,
                                        .colors = colors});
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
