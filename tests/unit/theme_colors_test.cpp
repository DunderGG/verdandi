#include <array>

#include <gtest/gtest.h>
#include <QColor>

#include "domain/theme.h"
#include "support/text_contrast.h"
#include "ui/theme/theme_colors.h"

using verdandi::domain::ThemeColorRole;
using verdandi::tests::blendedOver;
using verdandi::tests::contrastRatio;
using verdandi::tests::kDarkScheme;
using verdandi::tests::kLightScheme;
using verdandi::tests::SchemeColors;
using verdandi::ui::theme::color;
using verdandi::ui::theme::isDark;

namespace
{

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

TEST(ThemeColors, Color_OnABackground_TheShadeOfItsLightness)
{
    ASSERT_NE(color(ThemeColorRole::Good, false), color(ThemeColorRole::Good, true));

    EXPECT_EQ(color(ThemeColorRole::Good, kLightScheme.base), color(ThemeColorRole::Good, false));
    EXPECT_EQ(color(ThemeColorRole::Good, kDarkScheme.base), color(ThemeColorRole::Good, true));
}
