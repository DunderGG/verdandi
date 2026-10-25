#include "ui/header_view.h"

#include <initializer_list>

#include <QPainter>
#include <QPalette>
#include <QRect>
#include <QStyle>
#include <QStyleOptionHeaderV2>

#include "domain/theme.h"
#include "ui/theme/theme_colors.h"

namespace verdandi::ui
{

namespace
{

// The dividers between the sections, and below them, are the text colour at
// this opacity, so they show on any colour.
constexpr float kDividerAlpha = 0.25f;

} // namespace

HeaderView::HeaderView(Qt::Orientation orientation, QWidget* parent) : QHeaderView(orientation, parent)
{
    connect(theme::ThemeNotifier::instance(), &theme::ThemeNotifier::colorsChanged, this,
            [this] { viewport()->update(); });
}

std::optional<QColor> HeaderView::sectionColor() const
{
    return theme::customColor(domain::ThemeColorRole::TableHeader, theme::isDark(palette().color(QPalette::Window)));
}

QColor HeaderView::sectionTextColor() const
{
    return theme::contrastingColor(sectionColor().value_or(palette().color(QPalette::Button)),
                                   palette().color(QPalette::WindowText), palette().color(QPalette::Window));
}

void HeaderView::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const
{
    const std::optional<QColor> fill = sectionColor();
    if (!fill || !rect.isValid()) {
        QHeaderView::paintSection(painter, rect, logicalIndex);
        return;
    }

    QStyleOptionHeaderV2 option;
    initStyleOption(&option);
    initStyleOptionForIndex(&option, logicalIndex);
    option.rect = rect;
    // The styles draw the name in the button text colour, and windows11 the
    // sort arrow in the text colour.
    const QColor text = sectionTextColor();
    for (const QPalette::ColorRole role : {QPalette::ButtonText, QPalette::Text, QPalette::WindowText}) {
        option.palette.setColor(role, text);
    }

    painter->save();
    painter->setClipRect(rect);
    painter->fillRect(rect, *fill);
    QColor divider = text;
    divider.setAlphaF(kDividerAlpha);
    painter->setPen(divider);
    painter->drawLine(rect.topRight(), rect.bottomRight());
    painter->drawLine(rect.bottomLeft(), rect.bottomRight());

    // As QCommonStyle draws CE_Header, but for the section's background.
    QStyleOptionHeaderV2 part = option;
    part.rect = style()->subElementRect(QStyle::SE_HeaderLabel, &option, this);
    if (part.rect.isValid()) {
        style()->drawControl(QStyle::CE_HeaderLabel, &part, painter, this);
    }
    if (option.sortIndicator != QStyleOptionHeader::None) {
        part.rect = style()->subElementRect(QStyle::SE_HeaderArrow, &option, this);
        style()->drawPrimitive(QStyle::PE_IndicatorHeaderArrow, &part, painter, this);
    }
    painter->restore();
}

} // namespace verdandi::ui
