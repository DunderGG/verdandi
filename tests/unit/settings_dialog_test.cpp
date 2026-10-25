#include <array>
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>

#include <gtest/gtest.h>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLocale>
#include <QMessageBox>
#include <QPoint>
#include <QPushButton>
#include <QString>
#include <QTabWidget>

#include "domain/settings.h"
#include "domain/theme.h"
#include "support/settings_samples.h"
#include "ui/color_button.h"
#include "ui/settings_dialog.h"
#include "ui/theme/theme_colors.h"
#include "ui/theme_files.h"

using namespace verdandi::domain;
using std::chrono::milliseconds;
using verdandi::tests::nonDefaultSettings;
using verdandi::ui::ColorButton;
using verdandi::ui::SettingsDialog;
using verdandi::ui::ThemeFiles;
using verdandi::ui::ThemeImport;

namespace
{

// Selects the item of comboBox that stands for value.
template <typename Enum> void selectItem(QComboBox* comboBox, Enum value)
{
    comboBox->setCurrentIndex(comboBox->findData(static_cast<int>(value)));
}

template <typename Enum> Enum selectedItem(const QComboBox* comboBox)
{
    return static_cast<Enum>(comboBox->currentData().toInt());
}

// Clicks one of the standard buttons of dialog's button box.
void clickButton(const SettingsDialog& dialog, QDialogButtonBox::StandardButton button)
{
    dialog.buttonBox()->button(button)->click();
}

} // namespace

TEST(SettingsDialog, Construction_ControlsShowTheSettings)
{
    const SettingsDialog dialog(nonDefaultSettings());

    EXPECT_DOUBLE_EQ(dialog.refreshIntervalSpinBox()->value(), 2.5);
    EXPECT_EQ(selectedItem<LogLevel>(dialog.logLevelComboBox()), LogLevel::Debug);
    EXPECT_EQ(selectedItem<ColorScheme>(dialog.colorSchemeComboBox()), ColorScheme::Dark);
    EXPECT_EQ(selectedItem<ByteUnits>(dialog.byteUnitsComboBox()), ByteUnits::Decimal);
    EXPECT_EQ(selectedItem<NetworkRateUnits>(dialog.networkRateUnitsComboBox()), NetworkRateUnits::BitsPerSecond);
    EXPECT_FALSE(dialog.cpuPanelCheckBox()->isChecked());
    EXPECT_FALSE(dialog.memoryPanelCheckBox()->isChecked());
    EXPECT_FALSE(dialog.diskPanelCheckBox()->isChecked());
    EXPECT_FALSE(dialog.networkPanelCheckBox()->isChecked());
    EXPECT_FALSE(dialog.uptimePanelCheckBox()->isChecked());
    EXPECT_EQ(dialog.colorButton(ThemeColorRole::Page, false)->color(), QColor(0xf0, 0xf0, 0xf0));
    EXPECT_EQ(dialog.colorButton(ThemeColorRole::Card, true)->color(), QColor(0x20, 0x20, 0x20));
    EXPECT_FALSE(dialog.colorButton(ThemeColorRole::Page, true)->color().has_value());
    EXPECT_FALSE(dialog.colorButton(ThemeColorRole::Card, false)->color().has_value());
}

TEST(SettingsDialog, ControlsEdited_SettingsReturnsWhatTheyShow)
{
    const SettingsDialog dialog(Settings{});

    dialog.refreshIntervalSpinBox()->setValue(2.5);
    selectItem(dialog.logLevelComboBox(), LogLevel::Debug);
    selectItem(dialog.colorSchemeComboBox(), ColorScheme::Dark);
    selectItem(dialog.byteUnitsComboBox(), ByteUnits::Decimal);
    selectItem(dialog.networkRateUnitsComboBox(), NetworkRateUnits::BitsPerSecond);
    dialog.cpuPanelCheckBox()->setChecked(false);
    dialog.memoryPanelCheckBox()->setChecked(false);
    dialog.diskPanelCheckBox()->setChecked(false);
    dialog.networkPanelCheckBox()->setChecked(false);
    dialog.uptimePanelCheckBox()->setChecked(false);
    dialog.colorButton(ThemeColorRole::Page, false)->setColor(QColor(0xf0, 0xf0, 0xf0));
    dialog.colorButton(ThemeColorRole::Card, true)->setColor(QColor(0x20, 0x20, 0x20));

    EXPECT_EQ(dialog.settings(), nonDefaultSettings());
}

TEST(SettingsDialog, ColorButtons_OnTheAppearanceTabWithTheDefaultsOfTheirScheme)
{
    const SettingsDialog dialog(Settings{});
    const QWidget* appearance = dialog.tabWidget()->widget(1);

    EXPECT_EQ(dialog.tabWidget()->tabText(1), "Appearance");
    EXPECT_TRUE(appearance->isAncestorOf(dialog.colorSchemeComboBox()));
    for (std::size_t index = 0; index < kThemeColorRoleCount; ++index) {
        const auto role = static_cast<ThemeColorRole>(index);
        SCOPED_TRACE(index);
        for (const bool isDark : {false, true}) {
            const ColorButton* button = dialog.colorButton(role, isDark);
            ASSERT_NE(button, nullptr);
            EXPECT_TRUE(appearance->isAncestorOf(button));
            EXPECT_EQ(button->defaultColor(), verdandi::ui::theme::defaultColor(role, isDark));
        }
    }
}

TEST(SettingsDialog, ResetColours_EveryColourToItsDefaultAndTheRestAsShown)
{
    SettingsDialog dialog(nonDefaultSettings());
    Settings expected = nonDefaultSettings();
    expected.theme = Theme{};

    dialog.resetColorsButton()->click();

    EXPECT_EQ(dialog.settings(), expected);
}

TEST(SettingsDialog, SetSettings_EachLogLevelColorSchemeAndUnit_SettingsReturnsIt)
{
    struct Case
    {
        const char* name{};
        Settings settings{};
    };
    const std::array cases{
        Case{"Trace", {.logLevel = LogLevel::Trace}},
        Case{"Debug", {.logLevel = LogLevel::Debug}},
        Case{"Info", {.logLevel = LogLevel::Info}},
        Case{"Warn", {.logLevel = LogLevel::Warn}},
        Case{"Error", {.logLevel = LogLevel::Error}},
        Case{"Critical", {.logLevel = LogLevel::Critical}},
        Case{"Off", {.logLevel = LogLevel::Off}},
        Case{"System", {.colorScheme = ColorScheme::System}},
        Case{"Light", {.colorScheme = ColorScheme::Light}},
        Case{"Dark", {.colorScheme = ColorScheme::Dark}},
        Case{"Binary", {.displayUnits = {.bytes = ByteUnits::Binary}}},
        Case{"Decimal", {.displayUnits = {.bytes = ByteUnits::Decimal}}},
        Case{"BytesPerSecond", {.displayUnits = {.networkRates = NetworkRateUnits::BytesPerSecond}}},
        Case{"BitsPerSecond", {.displayUnits = {.networkRates = NetworkRateUnits::BitsPerSecond}}},
    };
    // Starts from settings that differ in every value, so each case's value is set, not kept.
    SettingsDialog dialog(nonDefaultSettings());

    for (const auto& [name, settings] : cases) {
        SCOPED_TRACE(name);
        dialog.setSettings(settings);
        EXPECT_EQ(dialog.settings(), settings);
        // A value without an item would leave the combo box with none selected.
        EXPECT_NE(dialog.logLevelComboBox()->currentIndex(), -1);
        EXPECT_NE(dialog.colorSchemeComboBox()->currentIndex(), -1);
        EXPECT_NE(dialog.byteUnitsComboBox()->currentIndex(), -1);
        EXPECT_NE(dialog.networkRateUnitsComboBox()->currentIndex(), -1);
    }
}

TEST(SettingsDialog, IntervalTypedBetweenSteps_NearerStepReturnedAndShownWhenEditingEnds)
{
    const SettingsDialog dialog(Settings{});
    auto* spinBox = dialog.refreshIntervalSpinBox();

    spinBox->setValue(1.3);
    const milliseconds beforeEditingEnds = dialog.settings().refreshInterval;
    emit spinBox->editingFinished();

    EXPECT_EQ(beforeEditingEnds, milliseconds{1500});
    EXPECT_DOUBLE_EQ(spinBox->value(), 1.5);
    EXPECT_EQ(dialog.settings().refreshInterval, milliseconds{1500});
    EXPECT_DOUBLE_EQ(spinBox->singleStep(), 0.5);
}

TEST(SettingsDialog, IntervalUnderACommaLocale_ShownWithADecimalPoint)
{
    const QLocale previous;
    QLocale::setDefault(QLocale(QLocale::Swedish, QLocale::Sweden));
    const SettingsDialog dialog(Settings{.refreshInterval = milliseconds{2500}});
    const QString text = dialog.refreshIntervalSpinBox()->text();
    QLocale::setDefault(previous);

    EXPECT_EQ(text, "2.5 s");
}

TEST(SettingsDialog, SetSettings_IntervalThatCannotBeChosen_ShowsTheNearestThatCan)
{
    struct Case
    {
        const char* name{};
        milliseconds interval{};
        milliseconds expected{};
    };
    const std::array cases{
        Case{"BelowMinimum", milliseconds{100}, kMinRefreshInterval},
        Case{"AboveMaximum", milliseconds{20'000}, kMaxRefreshInterval},
        Case{"BetweenSteps", milliseconds{1234}, milliseconds{1000}},
    };
    SettingsDialog dialog(Settings{});

    for (const auto& [name, interval, expected] : cases) {
        SCOPED_TRACE(name);
        dialog.setSettings(Settings{.refreshInterval = interval});
        EXPECT_EQ(dialog.settings().refreshInterval, expected);
        EXPECT_DOUBLE_EQ(dialog.refreshIntervalSpinBox()->value(), static_cast<double>(expected.count()) / 1000.0);
    }
}

TEST(SettingsDialog, RestoreDefaults_ShowsTheDefaultsAndStaysOpen)
{
    SettingsDialog dialog(nonDefaultSettings());
    dialog.setAttribute(Qt::WA_DontShowOnScreen);
    dialog.show();

    clickButton(dialog, QDialogButtonBox::RestoreDefaults);

    EXPECT_EQ(dialog.settings(), Settings{});
    EXPECT_TRUE(dialog.isVisible());
}

TEST(SettingsDialog, Shown_FieldsOfGeneralAndUnitsLineUp)
{
    SettingsDialog dialog(Settings{});
    dialog.setAttribute(Qt::WA_DontShowOnScreen);

    dialog.show();

    const int intervalLeft = dialog.refreshIntervalSpinBox()->mapTo(&dialog, QPoint{}).x();
    EXPECT_EQ(dialog.byteUnitsComboBox()->mapTo(&dialog, QPoint{}).x(), intervalLeft);
    EXPECT_EQ(dialog.networkRateUnitsComboBox()->mapTo(&dialog, QPoint{}).x(), intervalLeft);
}

TEST(SettingsDialog, OkAndCancel_AcceptAndReject)
{
    struct Case
    {
        const char* name{};
        QDialogButtonBox::StandardButton button{};
        QDialog::DialogCode expected{};
    };
    const std::array cases{
        Case{"Ok", QDialogButtonBox::Ok, QDialog::Accepted},
        Case{"Cancel", QDialogButtonBox::Cancel, QDialog::Rejected},
    };

    for (const auto& [name, button, expected] : cases) {
        SCOPED_TRACE(name);
        SettingsDialog dialog(Settings{});
        dialog.setAttribute(Qt::WA_DontShowOnScreen);
        dialog.show();

        clickButton(dialog, button);

        EXPECT_EQ(dialog.result(), expected);
        EXPECT_FALSE(dialog.isVisible());
    }
}

TEST(SettingsDialog, ThemeFileButtons_DisabledUntilTheirFunctionsAreSet)
{
    SettingsDialog dialog(Settings{});
    ASSERT_FALSE(dialog.importThemeButton()->isEnabled());
    ASSERT_FALSE(dialog.exportThemeButton()->isEnabled());

    dialog.setThemeFiles(ThemeFiles{.importTheme = [](const std::filesystem::path&) { return ThemeImport{}; }});

    EXPECT_TRUE(dialog.importThemeButton()->isEnabled());
    EXPECT_FALSE(dialog.exportThemeButton()->isEnabled());
}

TEST(SettingsDialog, ImportTheme_ColoursRead_ShownAndNothingElseChanged)
{
    SettingsDialog dialog(Settings{.colorScheme = ColorScheme::Light});
    std::filesystem::path read;
    dialog.setThemeFiles(ThemeFiles{.importTheme = [&read](const std::filesystem::path& path) {
        read = path;
        return ThemeImport{.theme = nonDefaultSettings().theme};
    }});

    dialog.importTheme("C:\\Themes\\Ocean.json");

    EXPECT_EQ(read, std::filesystem::path{"C:\\Themes\\Ocean.json"});
    EXPECT_EQ(dialog.settings(), (Settings{.colorScheme = ColorScheme::Light, .theme = nonDefaultSettings().theme}));
    EXPECT_EQ(dialog.findChild<QMessageBox*>(), nullptr);
}

TEST(SettingsDialog, ImportTheme_SomeColoursNotUsed_ShownAndAWarningSaysWhichAsPlainText)
{
    SettingsDialog dialog(Settings{});
    dialog.setThemeFiles(ThemeFiles{.importTheme = [](const std::filesystem::path&) {
        return ThemeImport{.theme = nonDefaultSettings().theme,
                           .problems = {"theme.dark.page is \"<b>black</b>\", not a colour written as #rrggbb"}};
    }});

    dialog.importTheme("C:\\Themes\\Ocean.json");

    EXPECT_EQ(dialog.settings().theme, nonDefaultSettings().theme);
    const auto* box = dialog.findChild<QMessageBox*>();
    ASSERT_NE(box, nullptr);
    EXPECT_TRUE(box->isVisible());
    EXPECT_EQ(box->windowModality(), Qt::WindowModal);
    EXPECT_EQ(box->icon(), QMessageBox::Warning);
    EXPECT_EQ(box->textFormat(), Qt::PlainText);
    EXPECT_TRUE(box->informativeText().contains("C:\\Themes\\Ocean.json"));
    EXPECT_TRUE(box->informativeText().contains("<b>black</b>"));
}

TEST(SettingsDialog, ImportTheme_NoColoursRead_KeptAndAWarningSaysWhy)
{
    SettingsDialog dialog(nonDefaultSettings());
    dialog.setThemeFiles(ThemeFiles{.importTheme = [](const std::filesystem::path&) {
        return ThemeImport{.problems = {"The file holds no theme; no colours read"}};
    }});

    dialog.importTheme("C:\\Themes\\settings.json");

    EXPECT_EQ(dialog.settings(), nonDefaultSettings());
    const auto* box = dialog.findChild<QMessageBox*>();
    ASSERT_NE(box, nullptr);
    EXPECT_TRUE(box->informativeText().contains("The file holds no theme"));
}

TEST(SettingsDialog, ExportTheme_WritesTheColoursShown)
{
    SettingsDialog dialog(Settings{});
    dialog.colorButton(ThemeColorRole::Card, true)->setColor(QColor(0x20, 0x20, 0x20));
    std::filesystem::path written;
    Theme writtenTheme;
    dialog.setThemeFiles(
        ThemeFiles{.exportTheme = [&written, &writtenTheme](const std::filesystem::path& path, const Theme& theme) {
            written = path;
            writtenTheme = theme;
            return std::optional<std::string>{};
        }});

    dialog.exportTheme("C:\\Themes\\Mine.json");

    EXPECT_EQ(written, std::filesystem::path{"C:\\Themes\\Mine.json"});
    EXPECT_EQ(writtenTheme, dialog.settings().theme);
    EXPECT_TRUE(writtenTheme.dark[ThemeColorRole::Card].has_value());
    EXPECT_EQ(dialog.findChild<QMessageBox*>(), nullptr);
}

TEST(SettingsDialog, ExportTheme_NotWritten_AWarningSaysWhy)
{
    SettingsDialog dialog(Settings{});
    dialog.setThemeFiles(ThemeFiles{.exportTheme = [](const std::filesystem::path&, const Theme&) {
        return std::optional<std::string>{"Cannot replace the file: Access is denied."};
    }});

    dialog.exportTheme("C:\\Themes\\Mine.json");

    const auto* box = dialog.findChild<QMessageBox*>();
    ASSERT_NE(box, nullptr);
    EXPECT_TRUE(box->isVisible());
    EXPECT_EQ(box->windowModality(), Qt::WindowModal);
    EXPECT_TRUE(box->informativeText().contains("C:\\Themes\\Mine.json"));
    EXPECT_TRUE(box->informativeText().contains("Access is denied."));
}
