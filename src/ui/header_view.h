#pragma once

#include <optional>

#include <QColor>
#include <QHeaderView>

class QPainter;
class QRect;
class QWidget;

namespace verdandi::ui
{

/**
 * A table's header, as the process list's is (DC-155). The style draws it,
 * unless the theme or the user chose a table header colour for the scheme of
 * the window it is in. Then it paints each section itself, the same in every
 * style: filled with that colour and edged with faint dividers, with the
 * style's name and sort arrow in the theme's text or window colour,
 * whichever stands out more on it. It paints again whenever the chosen
 * colours change.
 *
 * Not thread-safe; use only on the UI thread.
 */
class HeaderView : public QHeaderView
{
    Q_OBJECT

public:
    explicit HeaderView(Qt::Orientation orientation, QWidget* parent = nullptr);
    ~HeaderView() override = default;

    /** Returns the colour that the header fills its sections with, or std::nullopt while the style draws them. */
    [[nodiscard]] std::optional<QColor> sectionColor() const;

    /** Returns the colour of the sections' names and sort arrows while sectionColor() has one. */
    [[nodiscard]] QColor sectionTextColor() const;

protected:
    void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;
};

} // namespace verdandi::ui
