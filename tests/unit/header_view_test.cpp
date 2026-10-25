#include <gtest/gtest.h>
#include <QColor>
#include <QHeaderView>
#include <QImage>
#include <QPalette>
#include <QPoint>
#include <QRegion>
#include <QStandardItemModel>
#include <QStringList>

#include "domain/theme.h"
#include "support/appearance.h"
#include "ui/header_view.h"

using verdandi::domain::RgbColor;
using verdandi::domain::ThemeColorRole;
using verdandi::tests::kAppearances;
using verdandi::tests::ScopedAppearance;
using verdandi::tests::ScopedCustomColors;
using verdandi::tests::themeWith;
using verdandi::ui::HeaderView;

namespace
{

constexpr RgbColor kGold{.red = 0xf4, .green = 0xb7, .blue = 0x40};

// A model with two columns, whose names the header shows.
class TwoColumns
{
public:
    TwoColumns()
    {
        m_model.setHorizontalHeaderLabels({"Name", "CPU"});
    }

    QStandardItemModel* model()
    {
        return &m_model;
    }

private:
    QStandardItemModel m_model{0, 2};
};

QImage render(QHeaderView& header)
{
    header.resize(240, header.sizeHint().height());
    QImage image(header.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    header.render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
    return image;
}

} // namespace

TEST(HeaderView, NoColourChosen_DrawnAsTheStyleDrawsAHeader)
{
    TwoColumns columns;
    QHeaderView plain(Qt::Horizontal);
    plain.setModel(columns.model());
    HeaderView header(Qt::Horizontal);
    header.setModel(columns.model());

    EXPECT_FALSE(header.sectionColor().has_value());
    EXPECT_EQ(render(header), render(plain));
}

TEST(HeaderView, ColourChosenForTheScheme_SectionsFilledWithItInEveryStyle)
{
    for (const auto& appearance : kAppearances) {
        SCOPED_TRACE(appearance.name);
        const ScopedAppearance drawn(appearance);
        const ScopedCustomColors chosen(
            themeWith(ThemeColorRole::TableHeader, appearance.scheme == Qt::ColorScheme::Dark, kGold));
        TwoColumns columns;
        HeaderView header(Qt::Horizontal);
        header.setModel(columns.model());

        const QImage image = render(header);

        EXPECT_EQ(header.sectionColor(), QColor(0xf4, 0xb7, 0x40));
        EXPECT_EQ(image.pixelColor(2, image.height() / 2), QColor(0xf4, 0xb7, 0x40));
        // Gold is light, so the names take the darker of the text and the window colours.
        const QColor text = header.palette().color(QPalette::WindowText);
        const QColor window = header.palette().color(QPalette::Window);
        EXPECT_EQ(header.sectionTextColor(), text.lightnessF() < window.lightnessF() ? text : window);
    }
}

TEST(HeaderView, ColourChosenForTheOtherScheme_DrawnAsTheStyleDrawsAHeader)
{
    const ScopedAppearance light(kAppearances.front());
    const ScopedCustomColors chosen(themeWith(ThemeColorRole::TableHeader, true, kGold));
    HeaderView header(Qt::Horizontal);

    EXPECT_FALSE(header.sectionColor().has_value());
}
