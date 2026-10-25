#include <gtest/gtest.h>
#include <QColor>
#include <QImage>
#include <QLabel>
#include <QPalette>
#include <QPoint>
#include <QRegion>
#include <QWidget>

#include "domain/theme.h"
#include "support/appearance.h"
#include "ui/card_frame.h"

using verdandi::domain::RgbColor;
using verdandi::domain::Theme;
using verdandi::domain::ThemeColorRole;
using verdandi::tests::ScopedCustomColors;
using verdandi::tests::themeWith;
using verdandi::ui::CardFrame;

namespace
{

const QColor kLightPage(0xff, 0xff, 0xff);
const QColor kDarkPage(0x2d, 0x2d, 0x2d);
constexpr RgbColor kDarkCard{.red = 0x20, .green = 0x26, .blue = 0x30};

void setPage(QWidget& page, const QColor& color)
{
    QPalette palette;
    palette.setColor(QPalette::Base, color);
    page.setPalette(palette);
}

} // namespace

TEST(CardFrame, NoCardColourChosen_ThePagesColourWhicheverScheme)
{
    QWidget page;
    setPage(page, kLightPage);
    auto* card = new CardFrame(&page);
    ASSERT_EQ(card->cardColor(), kLightPage);

    setPage(page, kDarkPage);

    EXPECT_EQ(card->cardColor(), kDarkPage);
}

TEST(CardFrame, CardColourChosenForThePagesScheme_TheWidgetsOnItTakeIt)
{
    const ScopedCustomColors chosen(themeWith(ThemeColorRole::Card, true, kDarkCard));
    QWidget page;
    setPage(page, kDarkPage);
    auto* card = new CardFrame(&page);
    auto* label = new QLabel(card);

    EXPECT_EQ(card->cardColor(), QColor(0x20, 0x26, 0x30));
    EXPECT_EQ(label->palette().color(QPalette::Base), QColor(0x20, 0x26, 0x30));
    EXPECT_EQ(label->palette().color(QPalette::Window), QColor(0x20, 0x26, 0x30));

    // Chosen for the dark scheme only, so a light page shows through.
    setPage(page, kLightPage);
    EXPECT_EQ(card->cardColor(), kLightPage);
}

TEST(CardFrame, ColoursChosenWhileShown_TakesTheNewCardColour)
{
    QWidget page;
    setPage(page, kDarkPage);
    auto* card = new CardFrame(&page);
    ASSERT_EQ(card->cardColor(), kDarkPage);

    {
        const ScopedCustomColors chosen(themeWith(ThemeColorRole::Card, true, kDarkCard));
        EXPECT_EQ(card->cardColor(), QColor(0x20, 0x26, 0x30));
    }

    EXPECT_EQ(card->cardColor(), kDarkPage);
}

TEST(CardFrame, Painted_ARoundedPanelInTheCardColour)
{
    const ScopedCustomColors chosen(themeWith(ThemeColorRole::Card, true, kDarkCard));
    QWidget page;
    setPage(page, kDarkPage);
    auto* card = new CardFrame(&page);
    card->resize(100, 60);
    QImage image(card->size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    // Without the background that a widget rendered alone would be given.
    card->render(&image, QPoint(), QRegion(), QWidget::DrawChildren);

    EXPECT_EQ(image.pixelColor(50, 30), QColor(0x20, 0x26, 0x30));
    // The corners are rounded off, so the page shows through them.
    EXPECT_EQ(image.pixelColor(0, 0).alpha(), 0);
}
