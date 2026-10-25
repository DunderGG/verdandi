#pragma once

#include <QColor>
#include <QFrame>
#include <QPointer>

class QEvent;
class QObject;
class QPaintEvent;

namespace verdandi::ui
{

/**
 * A card, as the dashboard's resource cards and the Network tab's adapter
 * cards are: a rounded panel with a hairline border, painted the same in
 * every style (DC-151). It is the colour of the page it is on, unless the
 * user chose a card colour for the page's scheme, which the widgets on it
 * then inherit as their window and base colours, so that they pick their own
 * colours by it. It takes the chosen colour again whenever its palette, the
 * page's or the user's colours change.
 *
 * Not thread-safe; use only on the UI thread.
 */
class CardFrame : public QFrame
{
    Q_OBJECT

public:
    explicit CardFrame(QWidget* parent = nullptr);
    ~CardFrame() override = default;

    /** Returns the colour that the card is filled with. */
    [[nodiscard]] QColor cardColor() const;

    bool eventFilter(QObject* watched, QEvent* event) override;

protected:
    void changeEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    // Watches the parent's palette, whose base colour is the page's.
    void watchPage();
    void applyCardColor();

    QPointer<QWidget> m_page;
};

} // namespace verdandi::ui
