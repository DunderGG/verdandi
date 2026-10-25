#include <array>

#include <gtest/gtest.h>
#include <QColor>
#include <QObject>

#include "domain/theme.h"
#include "support/appearance.h"
#include "support/text_contrast.h"
#include "ui/theme/theme_colors.h"

using namespace verdandi::ui::theme;
using verdandi::domain::RgbColor;
using verdandi::domain::Theme;
using verdandi::domain::ThemeColorRole;
using verdandi::tests::blendedOver;
using verdandi::tests::contrastRatio;
using verdandi::tests::kDarkScheme;
using verdandi::tests::kLightScheme;
using verdandi::tests::SchemeColors;
using verdandi::tests::ScopedCustomColors;
using verdandi::tests::themeWith;

namespace
{

constexpr RgbColor kChosen{.red = 0x12, .green = 0x34, .blue = 0x56};

struct NamedRole
{
    const char* name{};
    ThemeColorRole role{};
};

// Expects every role's colour to reach minimum on the window and base colours
// of both schemes, each in its own scheme's shade.
void expectContrastOnBothSchemes(const std::array<NamedRole, 3>& roles, double minimum)
{
    for (const auto& [name, role] : roles) {
        SCOPED_TRACE(name);
        for (const SchemeColors& scheme : {kLightScheme, kDarkScheme}) {
            for (const QColor& background : {scheme.window, scheme.base}) {
                const QColor shade = color(role, background);
                EXPECT_GE(contrastRatio(blendedOver(shade, background), background), minimum)
                    << shade.name().toStdString() << " on " << background.name().toStdString();
            }
        }
    }
}

} // namespace

TEST(ThemeColors, StatusRoles_ReadableOnTheWindowAndBaseOfTheirScheme)
{
    expectContrastOnBothSchemes({NamedRole{"Good", ThemeColorRole::Good}, NamedRole{"Warning", ThemeColorRole::Warning},
                                 NamedRole{"Critical", ThemeColorRole::Critical}},
                                verdandi::tests::kMinimumTextContrast);
}

TEST(ThemeColors, AccessIconRoles_VisibleOnTheWindowAndBaseOfTheirScheme)
{
    expectContrastOnBothSchemes({NamedRole{"AccessDenied", ThemeColorRole::AccessDenied},
                                 NamedRole{"AccessNeverOpened", ThemeColorRole::AccessNeverOpened},
                                 NamedRole{"AccessUnavailable", ThemeColorRole::AccessUnavailable}},
                                verdandi::tests::kMinimumGraphicsContrast);
}

TEST(ThemeColors, IsDark_LightnessBelowHalf)
{
    struct Case
    {
        const char* name{};
        QColor background{};
        bool expected{};
    };
    const std::array cases{
        Case{"white", QColor(0xff, 0xff, 0xff), false},
        Case{"light window", QColor(0xf3, 0xf3, 0xf3), false},
        Case{"mid grey, just light", QColor(0x80, 0x80, 0x80), false},
        Case{"mid grey, just dark", QColor(0x7f, 0x7f, 0x7f), true},
        Case{"dark base", QColor(0x2d, 0x2d, 0x2d), true},
        Case{"black", QColor(0x00, 0x00, 0x00), true},
    };

    for (const auto& [name, background, expected] : cases) {
        SCOPED_TRACE(name);
        EXPECT_EQ(isDark(background), expected);
    }
}

TEST(ThemeColors, DefaultColor_NoneForThePalettesRoles)
{
    for (std::size_t index = 0; index < verdandi::domain::kThemeColorRoleCount; ++index) {
        const auto role = static_cast<ThemeColorRole>(index);
        const bool isPaletteRole = role == ThemeColorRole::Window || role == ThemeColorRole::Page ||
                                   role == ThemeColorRole::Card || role == ThemeColorRole::Text ||
                                   role == ThemeColorRole::Accent;
        SCOPED_TRACE(index);
        EXPECT_EQ(defaultColor(role, false).has_value(), !isPaletteRole);
        EXPECT_EQ(defaultColor(role, true).has_value(), !isPaletteRole);
    }
}

TEST(ThemeColors, ChosenColour_ReplacesTheBuiltInOneOfItsSchemeOnly)
{
    const ScopedCustomColors chosen(themeWith(ThemeColorRole::Good, true, kChosen));

    EXPECT_EQ(color(ThemeColorRole::Good, true), QColor(0x12, 0x34, 0x56));
    EXPECT_EQ(color(ThemeColorRole::Good, false), defaultColor(ThemeColorRole::Good, false));
    EXPECT_EQ(customColor(ThemeColorRole::Good, true), QColor(0x12, 0x34, 0x56));
    EXPECT_FALSE(customColor(ThemeColorRole::Good, false).has_value());
}

TEST(ThemeColors, ChosenColourOfATranslucentRole_KeepsTheBuiltInOpacity)
{
    const ScopedCustomColors chosen(themeWith(ThemeColorRole::ProcessStarted, false, kChosen));

    const QColor started = color(ThemeColorRole::ProcessStarted, false);

    EXPECT_EQ(started.rgb(), QColor(0x12, 0x34, 0x56).rgb());
    EXPECT_EQ(started.alpha(), defaultColor(ThemeColorRole::ProcessStarted, false)->alpha());
}

TEST(ThemeColors, SetCustomColors_NotifiesOnlyOfAChange)
{
    int notified = 0;
    QObject context;
    QObject::connect(ThemeNotifier::instance(), &ThemeNotifier::colorsChanged, &context, [&notified] { ++notified; });
    const Theme chosen = themeWith(ThemeColorRole::Card, false, kChosen);

    setCustomColors(chosen);
    setCustomColors(chosen);
    EXPECT_EQ(notified, 1);
    EXPECT_EQ(customColors(), chosen);

    setCustomColors(Theme{});
    EXPECT_EQ(notified, 2);
}

TEST(ThemeColors, ToRgbColorAndBack_SameColourWithoutItsOpacity)
{
    const QColor translucent(0x12, 0x34, 0x56, 0x80);

    EXPECT_EQ(toRgbColor(translucent), kChosen);
    EXPECT_EQ(toQColor(kChosen), QColor(0x12, 0x34, 0x56));
}

TEST(ThemeColors, Color_OnABackground_TheShadeOfItsLightness)
{
    ASSERT_NE(color(ThemeColorRole::Good, false), color(ThemeColorRole::Good, true));

    EXPECT_EQ(color(ThemeColorRole::Good, kLightScheme.base), color(ThemeColorRole::Good, false));
    EXPECT_EQ(color(ThemeColorRole::Good, kDarkScheme.base), color(ThemeColorRole::Good, true));
}
