#include <array>

#include <gtest/gtest.h>
#include <QColor>
#include <QPalette>
#include <QWidget>

#include "domain/theme.h"
#include "support/text_contrast.h"
#include "ui/text_tone.h"
#include "ui/theme/theme_colors.h"

using verdandi::domain::ThemeColorRole;
using verdandi::tests::blendedOver;
using verdandi::tests::contrastRatio;
using verdandi::tests::kDarkScheme;
using verdandi::tests::kLightScheme;
using verdandi::tests::kMinimumTextContrast;
using verdandi::tests::SchemeColors;
using verdandi::ui::dimmedTextColor;
using verdandi::ui::TextTone;
using verdandi::ui::textToneColor;
using verdandi::ui::TonedLabel;

namespace
{

constexpr std::array kTones{TextTone::Normal, TextTone::Dimmed, TextTone::Good, TextTone::Warning, TextTone::Critical};

QPalette paletteOf(const SchemeColors& scheme)
{
    QPalette palette;
    palette.setColor(QPalette::WindowText, scheme.text);
    palette.setColor(QPalette::Window, scheme.window);
    return palette;
}

QColor drawnColor(const TonedLabel& label)
{
    return label.palette().color(label.foregroundRole());
}

} // namespace

TEST(TextTone, TextToneColor_NormalAndDimmedReadableOnWindowAndBaseOfBothSchemes)
{
    for (const SchemeColors& scheme : {kLightScheme, kDarkScheme}) {
        for (const TextTone tone : {TextTone::Normal, TextTone::Dimmed}) {
            const QColor color = textToneColor(tone, scheme.text, scheme.window);
            for (const QColor& background : {scheme.window, scheme.base}) {
                EXPECT_GE(contrastRatio(blendedOver(color, background), background), kMinimumTextContrast)
                    << "tone " << static_cast<int>(tone) << " on " << background.name().toStdString();
            }
        }
    }
}

TEST(TextTone, TextToneColor_StatusTones_TheThemesColoursForTheBackground)
{
    struct Case
    {
        const char* name{};
        TextTone tone{};
        ThemeColorRole role{};
    };
    const std::array cases{
        Case{"Good", TextTone::Good, ThemeColorRole::Good},
        Case{"Warning", TextTone::Warning, ThemeColorRole::Warning},
        Case{"Critical", TextTone::Critical, ThemeColorRole::Critical},
    };

    for (const auto& [name, tone, role] : cases) {
        SCOPED_TRACE(name);
        for (const SchemeColors& scheme : {kLightScheme, kDarkScheme}) {
            EXPECT_EQ(textToneColor(tone, scheme.text, scheme.window), verdandi::ui::theme::color(role, scheme.window));
        }
    }
}

TEST(TextTone, DimmedTextColor_TextColourAtReducedOpacity)
{
    const QColor dimmed = dimmedTextColor(QColor(0x12, 0x34, 0x56));

    EXPECT_EQ(dimmed.rgb(), QColor(0x12, 0x34, 0x56).rgb());
    EXPECT_NEAR(dimmed.alphaF(), verdandi::ui::kDimmedTextAlpha, 0.01);
}

TEST(TonedLabel, StatusTones_BoldOthersNot)
{
    TonedLabel label;

    EXPECT_FALSE(label.font().bold());
    for (const TextTone tone : kTones) {
        label.setTone(tone);
        const bool isStatus = tone == TextTone::Good || tone == TextTone::Warning || tone == TextTone::Critical;
        EXPECT_EQ(label.font().bold(), isStatus) << static_cast<int>(tone);
        EXPECT_EQ(label.tone(), tone);
    }
}

TEST(TonedLabel, ParentPaletteChanges_ColourFollowsTheNewScheme)
{
    QWidget parent;
    parent.setPalette(paletteOf(kLightScheme));
    auto* good = new TonedLabel(TextTone::Good, &parent);
    auto* dimmed = new TonedLabel(TextTone::Dimmed, &parent);
    ASSERT_EQ(drawnColor(*good), textToneColor(TextTone::Good, kLightScheme.text, kLightScheme.window));
    ASSERT_EQ(drawnColor(*dimmed), dimmedTextColor(kLightScheme.text));

    parent.setPalette(paletteOf(kDarkScheme));

    EXPECT_EQ(drawnColor(*good), textToneColor(TextTone::Good, kDarkScheme.text, kDarkScheme.window));
    EXPECT_EQ(drawnColor(*dimmed), dimmedTextColor(kDarkScheme.text));
}

TEST(TonedLabel, SetTone_ChangesTheColour)
{
    QWidget parent;
    parent.setPalette(paletteOf(kDarkScheme));
    auto* label = new TonedLabel(TextTone::Dimmed, &parent);

    label->setTone(TextTone::Critical);

    EXPECT_EQ(drawnColor(*label), textToneColor(TextTone::Critical, kDarkScheme.text, kDarkScheme.window));
}
