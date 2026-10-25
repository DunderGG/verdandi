#include <cstddef>

#include <gtest/gtest.h>
#include <QColor>
#include <QImage>
#include <QWidget>

#include "domain/theme.h"
#include "ui/color_sample.h"

using verdandi::domain::kThemeColorRoleCount;
using verdandi::domain::ThemeColorRole;
using verdandi::ui::ColorSample;
using verdandi::ui::SampleColors;

namespace
{

// A light page and card, dark text, and a mid grey for the other roles.
SampleColors sampleColors()
{
    SampleColors colors;
    colors.fill(QColor(0x80, 0x80, 0x80));
    colors[static_cast<std::size_t>(ThemeColorRole::Window)] = QColor(0xe0, 0xe0, 0xe0);
    colors[static_cast<std::size_t>(ThemeColorRole::Page)] = QColor(0xff, 0xff, 0xff);
    colors[static_cast<std::size_t>(ThemeColorRole::Card)] = QColor(0xf0, 0xf0, 0xf0);
    colors[static_cast<std::size_t>(ThemeColorRole::Text)] = QColor(0x00, 0x00, 0x00);
    return colors;
}

QImage render(ColorSample& sample)
{
    sample.resize(sample.sizeHint());
    QImage image(sample.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    sample.render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
    return image;
}

} // namespace

TEST(ColorSample, EachRole_DrawsItsOwnColour)
{
    for (std::size_t index = 0; index < kThemeColorRoleCount; ++index) {
        const auto role = static_cast<ThemeColorRole>(index);
        SCOPED_TRACE(index);
        ColorSample sample(role);
        SampleColors colors = sampleColors();
        sample.setColors(colors);
        const QImage before = render(sample);

        colors[index] = QColor(0xc0, 0x20, 0x60);
        sample.setColors(colors);
        const QImage after = render(sample);

        EXPECT_EQ(sample.role(), role);
        EXPECT_EQ(sample.colors(), colors);
        EXPECT_NE(before, after);
    }
}

TEST(ColorSample, SizeHint_TallerThanALineOfText)
{
    const ColorSample sample(ThemeColorRole::Text);

    EXPECT_GT(sample.sizeHint().height(), sample.fontMetrics().height());
    EXPECT_GT(sample.sizeHint().width(), sample.fontMetrics().horizontalAdvance("Sample text"));
}
