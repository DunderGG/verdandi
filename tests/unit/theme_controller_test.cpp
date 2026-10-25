#include <array>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>
#include <QApplication>
#include <QCheckBox>
#include <QColor>
#include <QCoreApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QObject>
#include <QPalette>
#include <QStyle>
#include <QStyleHints>

#include "domain/settings.h"
#include "domain/theme.h"
#include "support/appearance.h"
#include "ui/theme/theme_colors.h"
#include "ui/theme_controller.h"

using verdandi::domain::ColorScheme;
using verdandi::domain::kDarkTheme;
using verdandi::domain::kFollowWindowsTheme;
using verdandi::domain::kLightTheme;
using verdandi::domain::LogLevel;
using verdandi::domain::NamedTheme;
using verdandi::domain::RgbColor;
using verdandi::domain::Settings;
using verdandi::domain::Theme;
using verdandi::domain::ThemeColorRole;
using verdandi::domain::ThemeColors;
using verdandi::tests::ScopedColorScheme;
using verdandi::tests::ScopedStyle;
using verdandi::ui::colorSchemeName;
using verdandi::ui::ThemeController;
using verdandi::ui::theme::toQColor;

namespace
{

constexpr RgbColor kWindow{.red = 0xe0, .green = 0xe8, .blue = 0xf0};
constexpr RgbColor kDarkWindow{.red = 0x10, .green = 0x18, .blue = 0x20};
constexpr RgbColor kPage{.red = 0xf8, .green = 0xf8, .blue = 0xf0};
constexpr RgbColor kText{.red = 0x20, .green = 0x30, .blue = 0x40};
constexpr RgbColor kAccent{.red = 0x80, .green = 0x20, .blue = 0x90};

// The colours of Ocean, a dark theme of the themes folder.
ThemeColors oceanColors()
{
    ThemeColors colors;
    colors[ThemeColorRole::Window] = kDarkWindow;
    colors[ThemeColorRole::Accent] = RgbColor{.red = 0xf4, .green = 0xb7, .blue = 0x40};
    return colors;
}

// A dark theme and a light one, as the themes folder holds them.
std::vector<NamedTheme> themesOfTheFolder()
{
    return {
        NamedTheme{.name = "Ocean", .theme = {.scheme = ColorScheme::Dark, .colors = oceanColors()}},
        NamedTheme{.name = "Paper", .theme = {.scheme = ColorScheme::Light}},
    };
}

bool isStyle(const char* name)
{
    return QApplication::style()->name().compare(name, Qt::CaseInsensitive) == 0;
}

bool isDark(const QColor& color)
{
    return color.lightnessF() < 0.5f;
}

Qt::ColorScheme drawnScheme()
{
    return QGuiApplication::styleHints()->colorScheme();
}

// Counts the theme changes that Qt sends the application, each of which gives
// every widget its style again.
class ThemeChangeCounter : public QObject
{
public:
    ThemeChangeCounter()
    {
        QCoreApplication::instance()->installEventFilter(this);
    }

    ~ThemeChangeCounter() override
    {
        QCoreApplication::instance()->removeEventFilter(this);
    }

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == QCoreApplication::instance() && event->type() == QEvent::ThemeChange) {
            ++m_count;
        }
        return false;
    }

    [[nodiscard]] int count() const
    {
        return m_count;
    }

private:
    int m_count{0};
};

} // namespace

TEST(ThemeController, ColorScheme_TheSchemeTheApplicationDrawsIn)
{
    for (const auto scheme : {Qt::ColorScheme::Light, Qt::ColorScheme::Dark}) {
        const ScopedColorScheme colorScheme(scheme);

        const ThemeController controller;

        EXPECT_EQ(controller.colorScheme(), scheme);
    }
}

TEST(ThemeController, SchemeChanges_EmitsTheNewSchemeOnce)
{
    const ScopedColorScheme light(Qt::ColorScheme::Light);
    ThemeController controller;
    std::vector<Qt::ColorScheme> emitted;
    QObject::connect(&controller, &ThemeController::colorSchemeChanged, &controller,
                     [&emitted](Qt::ColorScheme scheme) { emitted.push_back(scheme); });

    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);

    EXPECT_EQ(emitted, std::vector{Qt::ColorScheme::Dark});
    EXPECT_EQ(controller.colorScheme(), Qt::ColorScheme::Dark);
}

TEST(ThemeController, LightOnlyStyleInTheDarkScheme_ReplacedByFusionWithTheDarkPalette)
{
    const ScopedStyle vista("windowsvista");
    const ScopedColorScheme dark(Qt::ColorScheme::Dark);

    const ThemeController controller;

    EXPECT_TRUE(isStyle("fusion"));
    const QPalette palette = QApplication::palette();
    EXPECT_TRUE(isDark(palette.color(QPalette::Window)));
    EXPECT_TRUE(isDark(palette.color(QPalette::Base)));
    EXPECT_FALSE(isDark(palette.color(QPalette::WindowText)));
}

TEST(ThemeController, FusionInTheDarkScheme_CheckBoxesTakeTheApplicationPalette)
{
    const ScopedStyle vista("windowsvista");
    const ScopedColorScheme dark(Qt::ColorScheme::Dark);
    const ThemeController controller;

    const QCheckBox checkBox;

    // Fusion fills a check box with its base colour, which the Windows
    // theme's check box palette makes the accent colour in the dark scheme.
    EXPECT_EQ(checkBox.palette().color(QPalette::Base), QApplication::palette().color(QPalette::Base));
}

TEST(ThemeController, SchemeChanges_LightOnlyStyleReplacedOnlyWhileDark)
{
    const ScopedStyle vista("windowsvista");
    const ScopedColorScheme light(Qt::ColorScheme::Light);
    const ThemeController controller;
    ASSERT_TRUE(isStyle("windowsvista"));

    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    EXPECT_TRUE(isStyle("fusion"));
    EXPECT_TRUE(isDark(QApplication::palette().color(QPalette::Window)));

    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    EXPECT_TRUE(isStyle("windowsvista"));
    EXPECT_FALSE(isDark(QApplication::palette().color(QPalette::Window)));
    // No role of the application palette is set, so the Windows theme's per-class palettes apply again.
    EXPECT_FALSE(QCoreApplication::testAttribute(Qt::AA_SetPalette));
}

TEST(ThemeController, Destroyed_GivesBackTheReplacedStyleAndPalette)
{
    const ScopedStyle vista("windowsvista");
    const ScopedColorScheme dark(Qt::ColorScheme::Dark);
    {
        const ThemeController controller;
        ASSERT_TRUE(isStyle("fusion"));
    }

    EXPECT_TRUE(isStyle("windowsvista"));
    EXPECT_FALSE(QCoreApplication::testAttribute(Qt::AA_SetPalette));
}

TEST(ThemeController, StyleThatDrawsTheDarkScheme_Kept)
{
    const ScopedStyle windows11("windows11");
    const ScopedColorScheme dark(Qt::ColorScheme::Dark);

    const ThemeController controller;

    EXPECT_TRUE(isStyle("windows11"));
    EXPECT_FALSE(QCoreApplication::testAttribute(Qt::AA_SetPalette));
}

TEST(ThemeController, SetSettings_EachTheme_DrawsInItsScheme)
{
    const Qt::ColorScheme windows = drawnScheme();
    struct Case
    {
        const char* name{};
        const char* theme{};
        Qt::ColorScheme expected{};
    };
    // In this order, so that each case changes the scheme, and Follow Windows
    // follows another theme.
    const std::array cases{
        Case{"Light", "Light", Qt::ColorScheme::Light},
        Case{"Dark", "Dark", Qt::ColorScheme::Dark},
        Case{"A light theme of the folder", "Paper", Qt::ColorScheme::Light},
        Case{"A dark theme of the folder", "Ocean", Qt::ColorScheme::Dark},
        Case{"Follow Windows", "Follow Windows", windows},
    };
    ThemeController controller;
    controller.setThemes(themesOfTheFolder());

    for (const auto& [name, theme, expected] : cases) {
        SCOPED_TRACE(name);
        controller.setSettings(Settings{.theme = theme});
        EXPECT_EQ(controller.colorScheme(), expected);
        EXPECT_EQ(controller.themeName(), theme);
    }
}

TEST(ThemeController, SetSettings_ThemeThatTheThemesLack_DrawsAsFollowWindows)
{
    const Qt::ColorScheme windows = drawnScheme();
    ThemeController controller;
    controller.setThemes(themesOfTheFolder());

    controller.setSettings(Settings{.theme = "Removed"});

    EXPECT_EQ(controller.themeName(), kFollowWindowsTheme);
    EXPECT_EQ(controller.colorScheme(), windows);
}

TEST(ThemeController, SetThemes_OrderedByNameWhateverItsCase)
{
    ThemeController controller;

    controller.setThemes({NamedTheme{.name = "ocean"}, NamedTheme{.name = "Verdandi"}, NamedTheme{.name = "Paper"}});
    controller.addTheme(NamedTheme{.name = "Autumn"});

    std::vector<std::string> names;
    for (const NamedTheme& theme : controller.themes()) {
        names.push_back(theme.name);
    }
    EXPECT_EQ(names, (std::vector<std::string>{"Autumn", "ocean", "Paper", "Verdandi"}));
}

TEST(ThemeController, SetSettings_DarkWithTheLightOnlyStyle_DrawsWithFusionUntilLight)
{
    const ScopedStyle vista("windowsvista");
    ThemeController controller;

    controller.setSettings(Settings{.theme = std::string{kDarkTheme}});
    EXPECT_TRUE(isStyle("fusion"));

    controller.setSettings(Settings{.theme = std::string{kLightTheme}});
    EXPECT_TRUE(isStyle("windowsvista"));
}

TEST(ThemeController, SetSettings_AnotherThemeOfTheSameScheme_AsksQtNoMore)
{
    ThemeController controller;
    controller.setThemes(themesOfTheFolder());
    const ThemeChangeCounter themeChanges;

    controller.setSettings(Settings{.theme = std::string{kDarkTheme}});
    ASSERT_EQ(themeChanges.count(), 1);
    controller.setSettings(Settings{.theme = std::string{kDarkTheme}, .logLevel = LogLevel::Debug});
    controller.setSettings(Settings{.theme = "Ocean"});

    EXPECT_EQ(themeChanges.count(), 1);
}

TEST(ThemeController, DestroyedAfterChoosingATheme_GivesBackWindowsScheme)
{
    const Qt::ColorScheme windows = drawnScheme();
    const std::string_view other = windows == Qt::ColorScheme::Dark ? kLightTheme : kDarkTheme;
    {
        ThemeController controller;
        controller.setSettings(Settings{.theme = std::string{other}});
        ASSERT_NE(drawnScheme(), windows);
    }

    EXPECT_EQ(drawnScheme(), windows);
}

TEST(ThemeController, WindowsColorScheme_FollowedOnlyWhileTheApplicationFollowsWindows)
{
    const ScopedColorScheme light(Qt::ColorScheme::Light);
    ThemeController controller;
    EXPECT_EQ(controller.windowsColorScheme(), ColorScheme::Light);

    // As Qt reports a change of Windows' scheme.
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    EXPECT_EQ(controller.windowsColorScheme(), ColorScheme::Dark);

    controller.setSettings(Settings{.theme = std::string{kLightTheme}});
    EXPECT_EQ(controller.windowsColorScheme(), ColorScheme::Dark);

    controller.setSettings(Settings{});
    EXPECT_EQ(controller.windowsColorScheme(),
              drawnScheme() == Qt::ColorScheme::Dark ? ColorScheme::Dark : ColorScheme::Light);
}

TEST(ThemeController, SetSettings_ChosenColours_PaletteRolesAndTheThemeOfTheDrawnScheme)
{
    const ScopedColorScheme light(Qt::ColorScheme::Light);
    ThemeController controller;
    Settings settings;
    ThemeColors& lightColors = settings.themeColors[std::string{kLightTheme}];
    lightColors[ThemeColorRole::Window] = kWindow;
    lightColors[ThemeColorRole::Page] = kPage;
    lightColors[ThemeColorRole::Text] = kText;
    lightColors[ThemeColorRole::Accent] = kAccent;
    settings.themeColors[std::string{kDarkTheme}][ThemeColorRole::Window] = kDarkWindow;

    controller.setSettings(settings);

    const QPalette palette = QApplication::palette();
    EXPECT_EQ(palette.color(QPalette::Window), toQColor(kWindow));
    EXPECT_EQ(palette.color(QPalette::Base), toQColor(kPage));
    EXPECT_EQ(palette.color(QPalette::Active, QPalette::WindowText), toQColor(kText));
    EXPECT_EQ(palette.color(QPalette::Active, QPalette::Text), toQColor(kText));
    EXPECT_EQ(palette.color(QPalette::Active, QPalette::Accent), toQColor(kAccent));
    EXPECT_EQ(palette.color(QPalette::Active, QPalette::Highlight), toQColor(kAccent));
    EXPECT_EQ(verdandi::ui::theme::customColors(), (Theme{.scheme = ColorScheme::Light, .colors = lightColors}));
}

TEST(ThemeController, SchemeChanges_TheNewSchemesChosenColours)
{
    const ScopedColorScheme light(Qt::ColorScheme::Light);
    ThemeController controller;
    Settings settings;
    settings.themeColors[std::string{kLightTheme}][ThemeColorRole::Window] = kWindow;
    settings.themeColors[std::string{kDarkTheme}][ThemeColorRole::Window] = kDarkWindow;
    controller.setSettings(settings);
    ASSERT_EQ(QApplication::palette().color(QPalette::Window), toQColor(kWindow));

    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);

    EXPECT_EQ(QApplication::palette().color(QPalette::Window), toQColor(kDarkWindow));
}

TEST(ThemeController, SetSettings_ThemeOfTheFolder_ItsColoursWithTheUsersInPlaceOfItsOwn)
{
    ThemeController controller;
    controller.setThemes(themesOfTheFolder());
    Settings settings{.theme = "Ocean"};
    settings.themeColors["Ocean"][ThemeColorRole::Accent] = kAccent;
    // Light's colours are not Ocean's.
    settings.themeColors[std::string{kLightTheme}][ThemeColorRole::Text] = kText;

    controller.setSettings(settings);

    ThemeColors expected = oceanColors();
    expected[ThemeColorRole::Accent] = kAccent;
    const QPalette palette = QApplication::palette();
    EXPECT_EQ(palette.color(QPalette::Window), toQColor(kDarkWindow));
    EXPECT_EQ(palette.color(QPalette::Active, QPalette::Accent), toQColor(kAccent));
    EXPECT_EQ(verdandi::ui::theme::customColors(), (Theme{.scheme = ColorScheme::Dark, .colors = expected}));
}

TEST(ThemeController, ThemeInUseChanges_DrawsWithItsNewColours)
{
    ThemeController controller;
    controller.setThemes(themesOfTheFolder());
    controller.setSettings(Settings{.theme = "Ocean"});
    Theme changed{.scheme = ColorScheme::Dark};
    changed.colors[ThemeColorRole::Window] = kWindow;

    controller.addTheme(NamedTheme{.name = "Ocean", .theme = changed});

    EXPECT_EQ(QApplication::palette().color(QPalette::Window), toQColor(kWindow));
    EXPECT_EQ(controller.themes().size(), themesOfTheFolder().size());
}

TEST(ThemeController, SetSettings_ColoursNoLongerChosen_TheSchemesOwnPalette)
{
    const ScopedColorScheme light(Qt::ColorScheme::Light);
    ThemeController controller;
    const QColor ownWindow = QApplication::palette().color(QPalette::Window);
    Settings settings;
    settings.themeColors[std::string{kLightTheme}][ThemeColorRole::Window] = kWindow;
    controller.setSettings(settings);

    controller.setSettings(Settings{});

    EXPECT_EQ(QApplication::palette().color(QPalette::Window), ownWindow);
    EXPECT_FALSE(QCoreApplication::testAttribute(Qt::AA_SetPalette));
    EXPECT_EQ(verdandi::ui::theme::customColors(), Theme{});
}

TEST(ThemeController, ChosenTextWithFusion_FusionsDarkSurfacesKept)
{
    const ScopedStyle vista("windowsvista");
    const ScopedColorScheme dark(Qt::ColorScheme::Dark);
    ThemeController controller;
    const QColor fusionBase = QApplication::palette().color(QPalette::Base);
    Settings settings;
    settings.themeColors[std::string{kDarkTheme}][ThemeColorRole::Text] = kText;

    controller.setSettings(settings);

    EXPECT_TRUE(isStyle("fusion"));
    EXPECT_EQ(QApplication::palette().color(QPalette::Base), fusionBase);
    EXPECT_EQ(QApplication::palette().color(QPalette::Active, QPalette::WindowText), toQColor(kText));
}

TEST(ThemeController, Destroyed_ClearsTheChosenColours)
{
    {
        ThemeController controller;
        controller.setThemes(themesOfTheFolder());
        controller.setSettings(Settings{.theme = "Ocean"});
        ASSERT_NE(verdandi::ui::theme::customColors(), Theme{});
    }

    EXPECT_EQ(verdandi::ui::theme::customColors(), Theme{});
    EXPECT_FALSE(QCoreApplication::testAttribute(Qt::AA_SetPalette));
}

TEST(ThemeController, WindowsPalette_SchemeDrawnIn_ItsOwnWithoutTheChosenColours)
{
    const ScopedColorScheme light(Qt::ColorScheme::Light);
    ThemeController controller;
    const QColor ownWindow = QApplication::palette().color(QPalette::Window);
    Settings settings{.theme = std::string{kLightTheme}};
    settings.themeColors[std::string{kLightTheme}][ThemeColorRole::Window] = kWindow;

    controller.setSettings(settings);

    ASSERT_EQ(QApplication::palette().color(QPalette::Window), toQColor(kWindow));
    EXPECT_EQ(controller.windowsPalette(ColorScheme::Light).color(QPalette::Window), ownWindow);
}

TEST(ThemeController, WindowsPalette_SchemeNotDrawnIn_QtsSurfacesAndTextForItWithTheAccentDrawn)
{
    struct Case
    {
        const char* name{};
        Qt::ColorScheme drawn{};
        ColorScheme other{};
        QColor window{};
        QColor base{};
        QColor text{};
    };
    const std::array cases{
        Case{"dark, while drawing light", Qt::ColorScheme::Light, ColorScheme::Dark, QColor(0x1e, 0x1e, 0x1e),
             QColor(0x2d, 0x2d, 0x2d), QColor(Qt::white)},
        Case{"light, while drawing dark", Qt::ColorScheme::Dark, ColorScheme::Light, QColor(0xf3, 0xf3, 0xf3),
             QColor(Qt::white), QColor(Qt::black)},
    };

    for (const auto& [name, drawn, other, window, base, text] : cases) {
        SCOPED_TRACE(name);
        const ScopedColorScheme scheme(drawn);
        const ThemeController controller;

        const QPalette palette = controller.windowsPalette(other);

        EXPECT_EQ(palette.color(QPalette::Window), window);
        EXPECT_EQ(palette.color(QPalette::Base), base);
        EXPECT_EQ(palette.color(QPalette::WindowText), text);
        EXPECT_EQ(palette.color(QPalette::Accent), QApplication::palette().color(QPalette::Accent));
    }
}

TEST(ThemeController, ColorSchemeName_EachScheme)
{
    struct Case
    {
        const char* name{};
        Qt::ColorScheme scheme{};
        std::string_view expected{};
    };
    const std::array cases{
        Case{"Light", Qt::ColorScheme::Light, "the light colour scheme"},
        Case{"Dark", Qt::ColorScheme::Dark, "the dark colour scheme"},
        Case{"Unknown, a contrast theme on Windows", Qt::ColorScheme::Unknown, "a contrast theme"},
    };

    for (const auto& [name, scheme, expected] : cases) {
        SCOPED_TRACE(name);
        EXPECT_EQ(colorSchemeName(scheme), expected);
    }
}
