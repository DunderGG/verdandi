#include "ui/color_sample.h"

#include <cstddef>

#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSizePolicy>
#include <QString>

#include "domain/health_status.h"
#include "domain/process_info.h"
#include "ui/dashboard_formatting.h"
#include "ui/process_icons.h"
#include "ui/theme/theme_colors.h"

namespace verdandi::ui
{

namespace
{

using domain::ThemeColorRole;

// The sample's width, and the room above and below its text, in
// device-independent pixels.
constexpr int kWidth = 132;
constexpr int kVerticalPadding = 4;
// A surface's rounded corners and its hairline edge, the text colour at this
// opacity, as a card's.
constexpr qreal kCornerRadius = 3.0;
constexpr float kEdgeAlpha = 0.15f;
// How far a card, a selection, a row or a chart lies inside the surface behind it.
constexpr qreal kInset = 3.0;
constexpr int kTextMargin = 6;
// A chart's line and the top of the fill below it, as SparklineWidget draws them.
constexpr qreal kLineWidth = 1.5;
constexpr int kFillTopAlpha = 90;
constexpr qreal kGridSpacing = 12.0;
// The access icons' design size.
constexpr int kIconSize = 16;

// The text of the samples, which stand for where the colours are drawn.
const QString kMenuText = "File   Help";
const QString kPageText = "Text on a page";
const QString kCardText = "Text on a card";
const QString kSampleText = "Sample text";
const QString kSelectedText = "Selected";
const QString kProcessText = "process.exe";

// Draws a rounded surface filled with fill and edged with text.
void drawSurface(QPainter& painter, const QRectF& rect, const QColor& fill, const QColor& text)
{
    QColor edge = text;
    edge.setAlphaF(kEdgeAlpha);
    painter.setPen(QPen(edge, 1.0));
    painter.setBrush(fill);
    painter.drawRoundedRect(rect, kCornerRadius, kCornerRadius);
}

void drawText(QPainter& painter, const QRectF& rect, const QColor& color, const QString& text)
{
    painter.setPen(color);
    painter.drawText(rect.adjusted(kTextMargin, 0, -kTextMargin, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
}

// A value that rises and falls across rect, as a chart's line draws it.
QPainterPath wave(const QRectF& rect)
{
    const qreal low = rect.bottom() - rect.height() * 0.2;
    const qreal high = rect.top() + rect.height() * 0.2;
    const qreal middle = rect.center().y();
    QPainterPath path;
    path.moveTo(rect.left(), low);
    path.cubicTo(rect.left() + rect.width() * 0.3, low, rect.left() + rect.width() * 0.3, high,
                 rect.left() + rect.width() * 0.5, high);
    path.cubicTo(rect.left() + rect.width() * 0.7, high, rect.left() + rect.width() * 0.7, middle, rect.right(),
                 middle);
    return path;
}

// Draws a chart's line in line, with the fill below it, over a grid in grid
// if it is valid.
void drawChart(QPainter& painter, const QRectF& rect, const QColor& line, const QColor& grid)
{
    if (grid.isValid()) {
        painter.setPen(QPen(grid, 1.0));
        painter.setBrush(Qt::NoBrush);
        for (qreal x = rect.left() + kGridSpacing; x < rect.right(); x += kGridSpacing) {
            painter.drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
        }
        painter.drawLine(QPointF(rect.left(), rect.center().y()), QPointF(rect.right(), rect.center().y()));
        painter.drawRect(rect);
    }

    const QPainterPath path = wave(rect);
    QPainterPath fill = path;
    fill.lineTo(rect.bottomRight());
    fill.lineTo(rect.bottomLeft());
    fill.closeSubpath();
    QColor fillTop = line;
    fillTop.setAlpha(kFillTopAlpha);
    QColor fillBottom = line;
    fillBottom.setAlpha(0);
    QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
    gradient.setColorAt(0.0, fillTop);
    gradient.setColorAt(1.0, fillBottom);
    painter.fillPath(fill, gradient);

    painter.setPen(QPen(line, kLineWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

domain::HealthLevel healthLevelOf(ThemeColorRole role)
{
    switch (role) {
        case ThemeColorRole::Good:
            return domain::HealthLevel::Healthy;
        case ThemeColorRole::Warning:
            return domain::HealthLevel::Warning;
        case ThemeColorRole::Critical:
        default:
            return domain::HealthLevel::Critical;
    }
}

domain::ProcessDetailsStatus detailsStatusOf(ThemeColorRole role)
{
    switch (role) {
        case ThemeColorRole::AccessDenied:
            return domain::ProcessDetailsStatus::AccessDenied;
        case ThemeColorRole::AccessNeverOpened:
            return domain::ProcessDetailsStatus::NotQueried;
        case ThemeColorRole::AccessUnavailable:
        default:
            return domain::ProcessDetailsStatus::Unavailable;
    }
}

} // namespace

ColorSample::ColorSample(domain::ThemeColorRole role, QWidget* parent) : QWidget(parent), m_role(role)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

domain::ThemeColorRole ColorSample::role() const
{
    return m_role;
}

void ColorSample::setColors(const SampleColors& colors)
{
    m_colors = colors;
    update();
}

const SampleColors& ColorSample::colors() const
{
    return m_colors;
}

QSize ColorSample::sizeHint() const
{
    return {kWidth, fontMetrics().height() + 2 * kVerticalPadding};
}

const QColor& ColorSample::colorOf(domain::ThemeColorRole role) const
{
    return m_colors[static_cast<std::size_t>(role)];
}

void ColorSample::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF area = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const QRectF inner = area.adjusted(kInset, kInset, -kInset, -kInset);
    const QColor& text = colorOf(ThemeColorRole::Text);
    const QColor& color = colorOf(m_role);

    switch (m_role) {
        case ThemeColorRole::Window:
            drawSurface(painter, area, color, text);
            drawText(painter, area, text, kMenuText);
            break;
        case ThemeColorRole::Page:
            drawSurface(painter, area, color, text);
            drawText(painter, area, text, kPageText);
            break;
        case ThemeColorRole::Card:
            drawSurface(painter, area, colorOf(ThemeColorRole::Page), text);
            drawSurface(painter, inner, color, text);
            drawText(painter, inner, text, kCardText);
            break;
        case ThemeColorRole::Text:
            drawSurface(painter, area, colorOf(ThemeColorRole::Page), text);
            drawText(painter, area, color, kSampleText);
            break;
        case ThemeColorRole::Accent:
            drawSurface(painter, area, colorOf(ThemeColorRole::Page), text);
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawRoundedRect(inner, kCornerRadius, kCornerRadius);
            // The selected text that ThemeController sets with the accent.
            drawText(painter, inner, theme::isDark(color) ? QColor(Qt::white) : QColor(Qt::black), kSelectedText);
            break;
        case ThemeColorRole::ChartLine1:
        case ThemeColorRole::ChartLine2:
        case ThemeColorRole::ChartLine3:
        case ThemeColorRole::ChartLine4:
        case ThemeColorRole::ChartLine5:
            drawSurface(painter, area, colorOf(ThemeColorRole::Card), text);
            drawChart(painter, inner, color, QColor{});
            break;
        case ThemeColorRole::ChartGrid:
            drawSurface(painter, area, colorOf(ThemeColorRole::Card), text);
            drawChart(painter, inner, colorOf(ThemeColorRole::ChartLine1), color);
            break;
        case ThemeColorRole::Good:
        case ThemeColorRole::Warning:
        case ThemeColorRole::Critical: {
            drawSurface(painter, area, colorOf(ThemeColorRole::Card), text);
            // Bold, as the dashboard's cards show a status.
            QFont bold = font();
            bold.setBold(true);
            painter.setFont(bold);
            drawText(painter, area, color, healthLevelText(healthLevelOf(m_role)));
            break;
        }
        case ThemeColorRole::ProcessStarted:
        case ThemeColorRole::ProcessExited: {
            drawSurface(painter, area, colorOf(ThemeColorRole::Page), text);
            painter.fillRect(inner, color);
            // An exited process's row is in italics, as the process list shows it.
            QFont rowFont = font();
            rowFont.setItalic(m_role == ThemeColorRole::ProcessExited);
            painter.setFont(rowFont);
            drawText(painter, inner, text, kProcessText);
            break;
        }
        case ThemeColorRole::AccessDenied:
        case ThemeColorRole::AccessNeverOpened:
        case ThemeColorRole::AccessUnavailable:
        default: {
            drawSurface(painter, area, colorOf(ThemeColorRole::Page), text);
            const QRect icon(kTextMargin, (height() - kIconSize) / 2, kIconSize, kIconSize);
            detailsStatusIcon(detailsStatusOf(m_role), color).paint(&painter, icon);
            drawText(painter, area.adjusted(kIconSize + kTextMargin / 2.0, 0, 0, 0), text, kProcessText);
            break;
        }
    }
}

} // namespace verdandi::ui
