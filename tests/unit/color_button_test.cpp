#include <optional>
#include <vector>

#include <gtest/gtest.h>
#include <QAction>
#include <QColor>
#include <QColorDialog>
#include <QObject>

#include "ui/color_button.h"

using verdandi::ui::ColorButton;

namespace
{

const QColor kChosen(0x12, 0x34, 0x56);

// Records each colorChanged() of button in emitted.
void recordColorChanges(ColorButton& button, std::vector<std::optional<QColor>>& emitted)
{
    QObject::connect(&button, &ColorButton::colorChanged,
                     [&emitted](const std::optional<QColor>& color) { emitted.push_back(color); });
}

} // namespace

TEST(ColorButton, NoColourChosen_SaysDefaultAndOffersNoUseDefault)
{
    ColorButton button;
    button.setDefaultColor(QColor(0x2a, 0x78, 0xd6));

    EXPECT_FALSE(button.color().has_value());
    EXPECT_EQ(button.text(), "Default");
    EXPECT_FALSE(button.icon().isNull());
    EXPECT_FALSE(button.useDefaultAction()->isEnabled());
}

TEST(ColorButton, SetColor_ShowsItsNameAndKeepsItOpaque)
{
    ColorButton button;

    button.setColor(QColor(0x12, 0x34, 0x56, 0x80));

    EXPECT_EQ(button.color(), kChosen);
    EXPECT_EQ(button.color()->alpha(), 255);
    EXPECT_EQ(button.text(), "#123456");
    EXPECT_TRUE(button.useDefaultAction()->isEnabled());
}

TEST(ColorButton, Click_OpensAWindowModalColourDialogTitledForTheColour)
{
    ColorButton button;
    button.setDialogTitle("Cards in the dark scheme");
    button.setDefaultColor(kChosen);

    button.click();

    auto* dialog = button.findChild<QColorDialog*>();
    ASSERT_NE(dialog, nullptr);
    EXPECT_TRUE(dialog->isVisible());
    EXPECT_EQ(dialog->windowModality(), Qt::WindowModal);
    EXPECT_EQ(dialog->windowTitle(), "Cards in the dark scheme");
    EXPECT_EQ(dialog->currentColor(), kChosen);
    EXPECT_EQ(button.accessibleName(), "Cards in the dark scheme");
    dialog->reject();
}

TEST(ColorButton, DialogAccepted_ChoosesTheColourAndEmitsIt)
{
    ColorButton button;
    std::vector<std::optional<QColor>> emitted;
    recordColorChanges(button, emitted);
    QColorDialog* dialog = button.openColorDialog();

    dialog->setCurrentColor(kChosen);
    dialog->accept();

    EXPECT_EQ(button.color(), kChosen);
    EXPECT_EQ(emitted, (std::vector<std::optional<QColor>>{kChosen}));
}

TEST(ColorButton, DialogCancelled_KeepsTheColourAndEmitsNothing)
{
    ColorButton button;
    button.setColor(kChosen);
    std::vector<std::optional<QColor>> emitted;
    recordColorChanges(button, emitted);
    QColorDialog* dialog = button.openColorDialog();

    dialog->setCurrentColor(Qt::red);
    dialog->reject();

    EXPECT_EQ(button.color(), kChosen);
    EXPECT_TRUE(emitted.empty());
}

TEST(ColorButton, UseDefault_ClearsTheColourAndEmits)
{
    ColorButton button;
    button.setColor(kChosen);
    std::vector<std::optional<QColor>> emitted;
    recordColorChanges(button, emitted);

    button.useDefaultAction()->trigger();

    EXPECT_FALSE(button.color().has_value());
    EXPECT_EQ(button.text(), "Default");
    EXPECT_EQ(emitted, (std::vector<std::optional<QColor>>{std::nullopt}));
}
