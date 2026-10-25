#include "ui/color_button.h"

#include <array>

#include <QAction>
#include <QBrush>
#include <QColorDialog>
#include <QEvent>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizePolicy>

namespace verdandi::ui
{

namespace
{

// The swatch's design grid, in device-independent pixels, as an icon's.
constexpr int kSwatchSize = 16;
constexpr std::array kDevicePixelRatios{1.0, 1.5, 2.0};
constexpr qreal kSwatchInset = 1.5;
constexpr qreal kSwatchCornerRadius = 2.0;
// The swatch's outline is the button's text colour at this opacity, so that
// a swatch of the button's own colour still shows.
constexpr float kOutlineAlpha = 0.45f;

// Returns a swatch of color, or a crossed-out empty swatch for std::nullopt.
QIcon swatchIcon(const std::optional<QColor>& color, const QColor& outline)
{
    QIcon icon;
    for (const qreal ratio : kDevicePixelRatios) {
        QPixmap pixmap(QSize(kSwatchSize, kSwatchSize) * ratio);
        pixmap.setDevicePixelRatio(ratio);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(outline, 1.0));
        painter.setBrush(color ? QBrush(*color) : QBrush(Qt::NoBrush));
        const QRectF square(kSwatchInset, kSwatchInset, kSwatchSize - 2 * kSwatchInset, kSwatchSize - 2 * kSwatchInset);
        painter.drawRoundedRect(square, kSwatchCornerRadius, kSwatchCornerRadius);
        if (!color) {
            painter.drawLine(square.bottomLeft(), square.topRight());
        }
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
}

} // namespace

ColorButton::ColorButton(QWidget* parent) : QToolButton(parent)
{
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setPopupMode(QToolButton::MenuButtonPopup);
    // Fills its column of the colour grid, so the buttons line up.
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* menu = new QMenu(this);
    m_useDefaultAction = menu->addAction("Use &Default");
    setMenu(menu);

    connect(this, &QToolButton::clicked, this, &ColorButton::openColorDialog);
    connect(m_useDefaultAction, &QAction::triggered, this, [this] {
        setColor(std::nullopt);
        emit colorChanged(m_color);
    });
    updateFace();
}

void ColorButton::setColor(const std::optional<QColor>& color)
{
    m_color = color;
    // A chosen colour is opaque; a translucent role keeps its own opacity.
    if (m_color) {
        m_color->setAlpha(255);
    }
    updateFace();
}

const std::optional<QColor>& ColorButton::color() const
{
    return m_color;
}

void ColorButton::setDefaultColor(const std::optional<QColor>& color)
{
    m_defaultColor = color;
    updateFace();
}

const std::optional<QColor>& ColorButton::defaultColor() const
{
    return m_defaultColor;
}

void ColorButton::setDialogTitle(const QString& title)
{
    m_dialogTitle = title;
    setAccessibleName(title);
}

QAction* ColorButton::useDefaultAction() const
{
    return m_useDefaultAction;
}

QColorDialog* ColorButton::openColorDialog()
{
    const QColor initial = m_color.value_or(m_defaultColor.value_or(palette().color(QPalette::Button)));
    // Qt's own dialog, which follows the application's colours and the
    // tests can drive, rather than Windows' common dialog.
    auto* dialog = new QColorDialog(initial, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setOption(QColorDialog::DontUseNativeDialog);
    dialog->setWindowTitle(m_dialogTitle);
    connect(dialog, &QColorDialog::colorSelected, this, [this](const QColor& color) {
        setColor(color);
        emit colorChanged(m_color);
    });
    dialog->open();
    return dialog;
}

void ColorButton::changeEvent(QEvent* event)
{
    QToolButton::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) {
        updateFace();
    }
}

void ColorButton::updateFace()
{
    QColor outline = palette().color(QPalette::ButtonText);
    outline.setAlphaF(kOutlineAlpha);
    setIcon(swatchIcon(m_color ? m_color : m_defaultColor, outline));
    setText(m_color ? m_color->name() : QString("Default"));
    m_useDefaultAction->setEnabled(m_color.has_value());
}

} // namespace verdandi::ui
