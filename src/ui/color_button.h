#pragma once

#include <optional>

#include <QColor>
#include <QString>
#include <QToolButton>

class QAction;
class QColorDialog;
class QEvent;

namespace verdandi::ui
{

/**
 * A button for one colour of the theme that the user can choose (DC-151). It
 * shows a swatch and the chosen colour's name, such as "#2a78d6", or, while
 * none is chosen, "Default" beside the default colour's swatch, or beside a
 * crossed-out swatch when the default is not one colour, such as Windows'
 * own. Clicking it opens a window-modal colour dialog, whose OK chooses the
 * colour; its menu's Use Default clears the choice.
 *
 * Not thread-safe; use only on the UI thread.
 */
class ColorButton : public QToolButton
{
    Q_OBJECT

public:
    explicit ColorButton(QWidget* parent = nullptr);
    ~ColorButton() override = default;

    /** Sets the colour, or std::nullopt for the default. */
    void setColor(const std::optional<QColor>& color);
    /** Returns the chosen colour, or std::nullopt while the default applies. */
    [[nodiscard]] const std::optional<QColor>& color() const;

    /** Sets the default colour that the button shows while none is chosen; std::nullopt when it is not one colour. */
    void setDefaultColor(const std::optional<QColor>& color);
    [[nodiscard]] const std::optional<QColor>& defaultColor() const;

    /** Sets the title of the colour dialog, which names what the colour colours. */
    void setDialogTitle(const QString& title);

    /** Returns the menu's action that clears the chosen colour, which is enabled while one is chosen. */
    [[nodiscard]] QAction* useDefaultAction() const;

    /** Opens the colour dialog, as a click does, and returns it. It deletes itself when it closes. */
    QColorDialog* openColorDialog();

signals:
    /** Emitted when the user chooses a colour, or the default, in the dialog or the menu. */
    void colorChanged(const std::optional<QColor>& color);

protected:
    void changeEvent(QEvent* event) override;

private:
    void updateFace();

    std::optional<QColor> m_color;
    std::optional<QColor> m_defaultColor;
    QString m_dialogTitle;
    QAction* m_useDefaultAction{nullptr};
};

} // namespace verdandi::ui
