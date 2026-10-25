#include "ui/card_frame.h"

#include <optional>

#include <QEvent>
#include <QGuiApplication>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QRectF>
#include <QStyleHints>

#include "domain/theme.h"
#include "ui/theme/theme_colors.h"

namespace verdandi::ui
{

namespace
{

// As the windows11 style rounds a panel.
constexpr qreal kCornerRadius = 4.0;
// The border is the text colour at this opacity, as faint as the windows11
// style's, except in a contrast theme, where it is the text colour.
constexpr qreal kBorderAlpha = 0.1;

} // namespace

CardFrame::CardFrame(QWidget* parent) : QFrame(parent)
{
    setFrameShape(QFrame::NoFrame);
    watchPage();
    applyCardColor();
    connect(theme::ThemeNotifier::instance(), &theme::ThemeNotifier::colorsChanged, this, &CardFrame::applyCardColor);
}

QColor CardFrame::cardColor() const
{
    return palette().color(QPalette::Base);
}

bool CardFrame::eventFilter(QObject* watched, QEvent* event)
{
    // A chosen card colour overrides the base colour that the card inherits,
    // so a change of the page's alone sends the card no PaletteChange.
    if (watched == m_page && event->type() == QEvent::PaletteChange) {
        applyCardColor();
    }
    return QFrame::eventFilter(watched, event);
}

void CardFrame::changeEvent(QEvent* event)
{
    QFrame::changeEvent(event);
    if (event->type() == QEvent::ParentChange) {
        watchPage();
    }
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ParentChange) {
        applyCardColor();
    }
}

void CardFrame::watchPage()
{
    if (m_page != nullptr) {
        m_page->removeEventFilter(this);
    }
    m_page = parentWidget();
    if (m_page != nullptr) {
        m_page->installEventFilter(this);
    }
}

void CardFrame::paintEvent(QPaintEvent* /*event*/)
{
    QColor border = palette().color(QPalette::WindowText);
    if (QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Unknown) {
        border.setAlphaF(static_cast<float>(kBorderAlpha));
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(border, 1.0));
    painter.setBrush(cardColor());
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), kCornerRadius, kCornerRadius);
}

void CardFrame::applyCardColor()
{
    // The colour of the page, which the card would otherwise inherit.
    const QColor page = parentWidget() != nullptr ? parentWidget()->palette().color(QPalette::Base)
                                                  : QGuiApplication::palette().color(QPalette::Base);
    const std::optional<QColor> chosen = theme::customColor(domain::ThemeColorRole::Card, theme::isDark(page));
    if (!chosen) {
        // A palette with no role set inherits every role again.
        if (testAttribute(Qt::WA_SetPalette)) {
            setPalette(QPalette());
        }
        return;
    }

    // Setting the palette sends another PaletteChange, which finds the colour
    // already set and stops.
    if (testAttribute(Qt::WA_SetPalette) && palette().color(QPalette::Base) == *chosen &&
        palette().color(QPalette::Window) == *chosen) {
        return;
    }
    QPalette card;
    card.setColor(QPalette::Window, *chosen);
    card.setColor(QPalette::Base, *chosen);
    setPalette(card);
}

} // namespace verdandi::ui
