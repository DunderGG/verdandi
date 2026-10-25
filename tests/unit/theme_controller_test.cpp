#include <array>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QStyleHints>

#include "support/color_scheme.h"
#include "ui/theme_controller.h"

using verdandi::tests::ScopedColorScheme;
using verdandi::ui::colorSchemeName;
using verdandi::ui::ThemeController;

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
