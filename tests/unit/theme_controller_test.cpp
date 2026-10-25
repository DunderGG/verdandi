#include <array>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>
#include <QApplication>
#include <QCheckBox>
#include <QColor>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleHints>

#include "support/appearance.h"
#include "ui/theme_controller.h"

using verdandi::tests::ScopedColorScheme;
using verdandi::tests::ScopedStyle;
using verdandi::ui::colorSchemeName;
using verdandi::ui::ThemeController;

namespace
{

bool isStyle(const char* name)
{
    return QApplication::style()->name().compare(name, Qt::CaseInsensitive) == 0;
}

bool isDark(const QColor& color)
{
    return color.lightnessF() < 0.5f;
}

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
