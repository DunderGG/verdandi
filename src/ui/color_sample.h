#pragma once

#include <array>

#include <QColor>
#include <QSize>
#include <QWidget>

#include "domain/theme.h"

class QPaintEvent;

namespace verdandi::ui
{

/**
 * The colours that a ColorSample draws with, indexed by each role's value:
 * every role's colour as a theme has it, the translucent ones with their
 * opacity.
 */
using SampleColors = std::array<QColor, domain::kThemeColorRoleCount>;

/**
 * A sample of one colour of a theme, drawn as the monitor draws it (DC-154):
 * the window behind a menu's names, text on a page, a card on a page, a
 * selection in the accent, a table header, a chart's line or grid on a card,
 * a status on a card, a process row's highlight, or an access icon. It draws
 * with the colours it is given rather than the application's, so that the
 * settings dialog can show colours before they are applied.
 *
 * Not thread-safe; use only on the UI thread.
 */
class ColorSample : public QWidget
{
    Q_OBJECT

public:
    explicit ColorSample(domain::ThemeColorRole role, QWidget* parent = nullptr);
    ~ColorSample() override = default;

    /** Returns the colour that the sample shows. */
    [[nodiscard]] domain::ThemeColorRole role() const;

    /** Sets the colours to draw with, and draws again. Each is black until set. */
    void setColors(const SampleColors& colors);
    [[nodiscard]] const SampleColors& colors() const;

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    [[nodiscard]] const QColor& colorOf(domain::ThemeColorRole role) const;

    domain::ThemeColorRole m_role;
    SampleColors m_colors{};
};

} // namespace verdandi::ui
