#include "ui/text_tone.h"

#include <QEvent>
#include <QFont>
#include <QPalette>

#include "domain/theme.h"
#include "ui/theme/theme_colors.h"

namespace verdandi::ui
{

namespace
{

// The label keeps its tone's colour in this role, which a label does not
// otherwise use, and draws its text with it. It never sets WindowText or
// Window, so they stay inherited and describe the current colour scheme
// whenever the palette changes.
constexpr QPalette::ColorRole kToneRole = QPalette::PlaceholderText;

bool isBold(TextTone tone)
{
    return tone == TextTone::Good || tone == TextTone::Warning || tone == TextTone::Critical;
}

} // namespace

QColor dimmedTextColor(const QColor& textColor)
{
    QColor color = textColor;
    color.setAlphaF(static_cast<float>(kDimmedTextAlpha));
    return color;
}

QColor textToneColor(TextTone tone, const QColor& textColor, const QColor& background)
{
    switch (tone) {
        case TextTone::Dimmed:
            return dimmedTextColor(textColor);
        case TextTone::Good:
            return theme::color(domain::ThemeColorRole::Good, background);
        case TextTone::Warning:
            return theme::color(domain::ThemeColorRole::Warning, background);
        case TextTone::Critical:
            return theme::color(domain::ThemeColorRole::Critical, background);
        case TextTone::Normal:
        default:
            return textColor;
    }
}

TonedLabel::TonedLabel(TextTone tone, QWidget* parent) : QLabel(parent), m_tone(tone)
{
    setForegroundRole(kToneRole);
    applyTone();
    connect(theme::ThemeNotifier::instance(), &theme::ThemeNotifier::colorsChanged, this, &TonedLabel::applyTone);
}

void TonedLabel::setTone(TextTone tone)
{
    if (tone == m_tone) {
        return;
    }
    m_tone = tone;
    applyTone();
}

TextTone TonedLabel::tone() const
{
    return m_tone;
}

void TonedLabel::changeEvent(QEvent* event)
{
    QLabel::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) {
        applyTone();
    }
}

void TonedLabel::applyTone()
{
    if (font().bold() != isBold(m_tone)) {
        QFont toneFont = font();
        toneFont.setBold(isBold(m_tone));
        setFont(toneFont);
    }

    // Setting the palette sends another PaletteChange, which finds the colour
    // already set and stops.
    const QPalette current = palette();
    const QColor color = textToneColor(m_tone, current.color(QPalette::WindowText), current.color(backgroundRole()));
    if (current.color(kToneRole) != color) {
        QPalette toned = current;
        toned.setColor(kToneRole, color);
        setPalette(toned);
    }
}

} // namespace verdandi::ui
