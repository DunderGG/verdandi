#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>
#include <QAbstractButton>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
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
#include "ui/theme_sources.h"

using namespace verdandi::domain;
using std::chrono::milliseconds;
using verdandi::tests::nonDefaultSettings;
using verdandi::ui::ColorButton;
using verdandi::ui::SettingsDialog;
using verdandi::ui::ThemeImport;
using verdandi::ui::ThemeSources;

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

// Chooses the theme named name in dialog's theme list, as the user does.
void selectTheme(const SettingsDialog& dialog, const QString& name)
{
    dialog.themeComboBox()->setCurrentIndex(dialog.themeComboBox()->findData(name));
}

// Clicks one of the standard buttons of dialog's button box.
void clickButton(const SettingsDialog& dialog, QDialogButtonBox::StandardButton button)
{
    dialog.buttonBox()->button(button)->click();
}

// Sources whose themes are themes, which addTheme adds to, in place of the
// one of the same name, as the themes folder does.
ThemeSources sourcesWith(std::vector<NamedTheme>& themes)
{
    return ThemeSources{
        .themes = [&themes] { return themes; },
        .readTheme = [](const std::filesystem::path&) { return ThemeImport{}; },
        .addTheme =
            [&themes](const NamedTheme& theme) {
                std::erase_if(themes, [&theme](const NamedTheme& existing) { return existing.name == theme.name; });
                themes.push_back(theme);
                return std::optional<std::string>{};
            },
    };
}

// A theme of the dark scheme that chooses its accent.
Theme oceanTheme()
{
    Theme theme{.scheme = ColorScheme::Dark};
    theme.colors[ThemeColorRole::Accent] = RgbColor{.red = 0xf4, .green = 0xb7, .blue = 0x40};
    return theme;
}

} // namespace

TEST(SettingsDialog, Construction_ControlsShowTheSettings)
{
    const SettingsDialog dialog(nonDefaultSettings());

    EXPECT_DOUBLE_EQ(dialog.refreshIntervalSpinBox()->value(), 2.5);
    EXPECT_EQ(selectedItem<LogLevel>(dialog.logLevelComboBox()), LogLevel::Debug);
    EXPECT_EQ(dialog.themeComboBox()->currentData().toString(), "Dark");
    EXPECT_EQ(selectedItem<ByteUnits>(dialog.byteUnitsComboBox()), ByteUnits::Decimal);
    EXPECT_EQ(selectedItem<NetworkRateUnits>(dialog.networkRateUnitsComboBox()), NetworkRateUnits::BitsPerSecond);
    EXPECT_FALSE(dialog.cpuPanelCheckBox()->isChecked());
    EXPECT_FALSE(dialog.memoryPanelCheckBox()->isChecked());
    EXPECT_FALSE(dialog.diskPanelCheckBox()->isChecked());
    EXPECT_FALSE(dialog.networkPanelCheckBox()->isChecked());
    EXPECT_FALSE(dialog.uptimePanelCheckBox()->isChecked());
    // The colours of the theme chosen, Dark, and not those chosen in Light.
    EXPECT_EQ(dialog.shownThemeName(), "Dark");
    EXPECT_EQ(dialog.colorsGroup()->title(), "Colours of Dark");
    EXPECT_EQ(dialog.colorButton(ThemeColorRole::Card)->color(), QColor(0x20, 0x20, 0x20));
    EXPECT_FALSE(dialog.colorButton(ThemeColorRole::Page)->color().has_value());
}

TEST(SettingsDialog, ControlsEdited_SettingsReturnsWhatTheyShow)
{
    const SettingsDialog dialog(Settings{});

    dialog.refreshIntervalSpinBox()->setValue(2.5);
    selectItem(dialog.logLevelComboBox(), LogLevel::Debug);
    selectItem(dialog.byteUnitsComboBox(), ByteUnits::Decimal);
    selectItem(dialog.networkRateUnitsComboBox(), NetworkRateUnits::BitsPerSecond);
    dialog.cpuPanelCheckBox()->setChecked(false);
    dialog.memoryPanelCheckBox()->setChecked(false);
    dialog.diskPanelCheckBox()->setChecked(false);
    dialog.networkPanelCheckBox()->setChecked(false);
    dialog.uptimePanelCheckBox()->setChecked(false);
    // The colour chosen in Light stays when another theme is chosen.
    selectTheme(dialog, "Light");
    dialog.colorButton(ThemeColorRole::Page)->setColor(QColor(0xf0, 0xf0, 0xf0));
    selectTheme(dialog, "Dark");
    dialog.colorButton(ThemeColorRole::Card)->setColor(QColor(0x20, 0x20, 0x20));

    EXPECT_EQ(dialog.settings(), nonDefaultSettings());
}

TEST(SettingsDialog, ThemeList_TheBuiltInThemesThenTheThemesFolders)
{
    std::vector<NamedTheme> themes{{.name = "Ocean"}, {.name = "Verdandi"}};
    SettingsDialog dialog(Settings{});

    dialog.setThemeSources(sourcesWith(themes));

    const QComboBox* list = dialog.themeComboBox();
    EXPECT_TRUE(dialog.tabWidget()->widget(1)->isAncestorOf(list));
    EXPECT_EQ(dialog.tabWidget()->tabText(1), "Appearance");
    // A separator, which has no text, comes before the themes folder's.
    const std::array<QString, 6> expected{"Follow Windows", "Light", "Dark", "", "Ocean", "Verdandi"};
    ASSERT_EQ(list->count(), static_cast<int>(expected.size()));
    for (std::size_t index = 0; index < expected.size(); ++index) {
        SCOPED_TRACE(index);
        EXPECT_EQ(list->itemText(static_cast<int>(index)), expected[index]);
        EXPECT_EQ(list->itemData(static_cast<int>(index)).toString(), expected[index]);
    }
}

TEST(SettingsDialog, ColorButtons_ShowTheThemesOwnColoursAsTheirDefaults)
{
    std::vector<NamedTheme> themes{{.name = "Ocean", .theme = oceanTheme()}};
    SettingsDialog dialog(Settings{});
    dialog.setThemeSources(sourcesWith(themes));
    const QWidget* appearance = dialog.tabWidget()->widget(1);

    for (const std::string_view theme : {"Light", "Dark", "Ocean"}) {
        SCOPED_TRACE(theme);
        selectTheme(dialog, QString::fromUtf8(theme.data(), static_cast<qsizetype>(theme.size())));
        const bool isDark = theme != "Light";
        for (std::size_t index = 0; index < kThemeColorRoleCount; ++index) {
            const auto role = static_cast<ThemeColorRole>(index);
            SCOPED_TRACE(index);
            const ColorButton* button = dialog.colorButton(role);
            ASSERT_NE(button, nullptr);
            EXPECT_TRUE(appearance->isAncestorOf(button));
            const std::optional<QColor> expected =
                theme == "Ocean" && role == ThemeColorRole::Accent
                    ? std::optional{verdandi::ui::theme::toQColor(*oceanTheme().colors[ThemeColorRole::Accent])}
                    : verdandi::ui::theme::defaultColor(role, isDark);
            EXPECT_EQ(button->defaultColor(), expected);
            EXPECT_FALSE(button->color().has_value());
        }
    }
}

TEST(SettingsDialog, FollowWindows_ShowsTheColoursOfTheThemeOfWindowsScheme)
{
    for (const ColorScheme scheme : {ColorScheme::Light, ColorScheme::Dark}) {
        const std::string expected{builtInThemeName(scheme)};
        SCOPED_TRACE(expected);
        Settings settings;
        settings.themeColors[expected][ThemeColorRole::Accent] = RgbColor{.red = 0x12, .green = 0x34, .blue = 0x56};
        SettingsDialog dialog(settings);

        dialog.setThemeSources(ThemeSources{.windowsColorScheme = [scheme] { return scheme; }});

        EXPECT_EQ(dialog.themeComboBox()->currentData().toString(), "Follow Windows");
        EXPECT_EQ(dialog.shownThemeName(), expected);
        EXPECT_EQ(dialog.colorButton(ThemeColorRole::Accent)->color(), QColor(0x12, 0x34, 0x56));
        EXPECT_EQ(dialog.settings(), settings);
    }
}

TEST(SettingsDialog, ThemeOfTheFolderChosen_SelectedOnceTheThemesAreSet)
{
    std::vector<NamedTheme> themes{{.name = "Ocean", .theme = oceanTheme()}};
    SettingsDialog dialog(Settings{.theme = "Ocean"});

    dialog.setThemeSources(sourcesWith(themes));

    EXPECT_EQ(dialog.themeComboBox()->currentData().toString(), "Ocean");
    EXPECT_EQ(dialog.shownThemeName(), "Ocean");
    EXPECT_EQ(dialog.settings().theme, "Ocean");
}

TEST(SettingsDialog, ThemeNotInTheList_FollowWindowsSelected)
{
    const SettingsDialog dialog(Settings{.theme = "Removed"});

    EXPECT_EQ(dialog.themeComboBox()->currentData().toString(), "Follow Windows");
    EXPECT_EQ(dialog.settings().theme, kFollowWindowsTheme);
}

TEST(SettingsDialog, ResetColours_EveryColourOfTheThemeShownToItsOwn)
{
    SettingsDialog dialog(nonDefaultSettings());
    Settings expected = nonDefaultSettings();
    expected.themeColors.erase(std::string{kDarkTheme});

    dialog.resetColorsButton()->click();

    EXPECT_EQ(dialog.settings(), expected);
}

TEST(SettingsDialog, SetSettings_EachLogLevelThemeAndUnit_SettingsReturnsIt)
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
        Case{"Follow Windows", {.theme = std::string{kFollowWindowsTheme}}},
        Case{"Light", {.theme = std::string{kLightTheme}}},
        Case{"Dark", {.theme = std::string{kDarkTheme}}},
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
        EXPECT_NE(dialog.themeComboBox()->currentIndex(), -1);
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

TEST(SettingsDialog, ThemeFileButtons_DisabledUntilTheFunctionsTheyNeedAreSet)
{
    std::vector<NamedTheme> themes;
    SettingsDialog dialog(Settings{});
    ASSERT_FALSE(dialog.importThemeButton()->isEnabled());
    ASSERT_FALSE(dialog.exportThemeButton()->isEnabled());

    ThemeSources sources = sourcesWith(themes);
    sources.addTheme = {};
    dialog.setThemeSources(sources);
    EXPECT_FALSE(dialog.importThemeButton()->isEnabled());
    EXPECT_FALSE(dialog.exportThemeButton()->isEnabled());

    dialog.setThemeSources(sourcesWith(themes));
    EXPECT_TRUE(dialog.importThemeButton()->isEnabled());
    EXPECT_FALSE(dialog.exportThemeButton()->isEnabled());
}

TEST(SettingsDialog, ImportTheme_ThemeRead_AddedToTheFolderAndChosenWithItsOwnColours)
{
    std::vector<NamedTheme> themes;
    SettingsDialog dialog(Settings{});
    ThemeSources sources = sourcesWith(themes);
    std::filesystem::path read;
    sources.readTheme = [&read](const std::filesystem::path& path) {
        read = path;
        return ThemeImport{.theme = oceanTheme()};
    };
    dialog.setThemeSources(sources);

    dialog.importTheme("C:\\Themes\\Ocean.json");

    EXPECT_EQ(read, std::filesystem::path{"C:\\Themes\\Ocean.json"});
    EXPECT_EQ(themes, (std::vector<NamedTheme>{{.name = "Ocean", .theme = oceanTheme()}}));
    EXPECT_EQ(dialog.themeComboBox()->currentData().toString(), "Ocean");
    EXPECT_EQ(dialog.colorButton(ThemeColorRole::Accent)->defaultColor(), QColor(0xf4, 0xb7, 0x40));
    EXPECT_EQ(dialog.settings(), Settings{.theme = "Ocean"});
    EXPECT_EQ(dialog.findChild<QMessageBox*>(), nullptr);
}

TEST(SettingsDialog, ImportTheme_SomeValuesNotUsed_AddedAndAWarningSaysWhichAsPlainText)
{
    std::vector<NamedTheme> themes;
    SettingsDialog dialog(Settings{});
    ThemeSources sources = sourcesWith(themes);
    sources.readTheme = [](const std::filesystem::path&) {
        return ThemeImport{.theme = oceanTheme(),
                           .problems = {"colors.page is \"<b>black</b>\", not a colour written as #rrggbb"}};
    };
    dialog.setThemeSources(sources);

    dialog.importTheme("C:\\Themes\\Ocean.json");

    EXPECT_EQ(dialog.settings().theme, "Ocean");
    const auto* box = dialog.findChild<QMessageBox*>();
    ASSERT_NE(box, nullptr);
    EXPECT_TRUE(box->isVisible());
    EXPECT_EQ(box->windowModality(), Qt::WindowModal);
    EXPECT_EQ(box->icon(), QMessageBox::Warning);
    EXPECT_EQ(box->textFormat(), Qt::PlainText);
    EXPECT_TRUE(box->informativeText().contains("C:\\Themes\\Ocean.json"));
    EXPECT_TRUE(box->informativeText().contains("<b>black</b>"));
}

TEST(SettingsDialog, ImportTheme_NotImported_NothingAddedAndAWarningSaysWhy)
{
    struct Case
    {
        const char* name{};
        const char* path{};
        ThemeImport imported{};
        const char* reason{};
    };
    const std::array cases{
        Case{"No theme read", "C:\\Themes\\Ocean.json",
             ThemeImport{.problems = {"The file names no colour scheme, light, dark; no theme read"}},
             "names no colour scheme"},
        Case{"A built-in theme's name", "C:\\Themes\\dark.json", ThemeImport{.theme = oceanTheme()},
             "dark is the name of a built-in theme"},
    };

    for (const auto& [name, path, imported, reason] : cases) {
        SCOPED_TRACE(name);
        std::vector<NamedTheme> themes;
        SettingsDialog dialog(nonDefaultSettings());
        ThemeSources sources = sourcesWith(themes);
        sources.readTheme = [result = imported](const std::filesystem::path&) { return result; };
        dialog.setThemeSources(sources);

        dialog.importTheme(path);

        EXPECT_TRUE(themes.empty());
        EXPECT_EQ(dialog.settings(), nonDefaultSettings());
        const auto* box = dialog.findChild<QMessageBox*>();
        ASSERT_NE(box, nullptr);
        EXPECT_EQ(box->text(), "No theme was imported.");
        EXPECT_TRUE(box->informativeText().contains(path));
        EXPECT_TRUE(box->informativeText().contains(reason));
    }
}

TEST(SettingsDialog, ImportTheme_ThemeOfThatNameExists_ReplacedOnlyIfTheUserAgrees)
{
    std::vector<NamedTheme> themes{{.name = "Ocean", .theme = {.scheme = ColorScheme::Light}}};
    Settings settings{.theme = "Ocean"};
    settings.themeColors["Ocean"][ThemeColorRole::Card] = RgbColor{.red = 0x20, .green = 0x20, .blue = 0x20};
    SettingsDialog dialog(settings);
    ThemeSources sources = sourcesWith(themes);
    sources.readTheme = [](const std::filesystem::path&) { return ThemeImport{.theme = oceanTheme()}; };
    dialog.setThemeSources(sources);

    for (const QMessageBox::StandardButton answer : {QMessageBox::No, QMessageBox::Yes}) {
        SCOPED_TRACE(answer == QMessageBox::Yes ? "Yes" : "No");
        // The theme keeps the name it has, whatever the case of the file's.
        dialog.importTheme("C:\\Themes\\ocean.json");
        auto* box = dialog.findChild<QMessageBox*>();
        ASSERT_NE(box, nullptr);
        EXPECT_EQ(box->icon(), QMessageBox::Question);
        EXPECT_EQ(box->windowModality(), Qt::WindowModal);
        EXPECT_TRUE(box->text().contains("Ocean"));
        box->button(answer)->click();
        delete box;
    }

    EXPECT_EQ(themes, (std::vector<NamedTheme>{{.name = "Ocean", .theme = oceanTheme()}}));
    // The theme shows the file's colours, without those chosen in it before.
    EXPECT_EQ(dialog.settings(), Settings{.theme = "Ocean"});
}

TEST(SettingsDialog, ImportTheme_NotAdded_AWarningSaysWhy)
{
    SettingsDialog dialog(Settings{});
    std::vector<NamedTheme> themes;
    ThemeSources sources = sourcesWith(themes);
    sources.readTheme = [](const std::filesystem::path&) { return ThemeImport{.theme = oceanTheme()}; };
    sources.addTheme = [](const NamedTheme&) {
        return std::optional<std::string>{"Cannot replace the file: Access is denied."};
    };
    dialog.setThemeSources(sources);

    dialog.importTheme("C:\\Themes\\Ocean.json");

    EXPECT_EQ(dialog.settings(), Settings{});
    const auto* box = dialog.findChild<QMessageBox*>();
    ASSERT_NE(box, nullptr);
    EXPECT_TRUE(box->informativeText().contains("C:\\Themes\\Ocean.json"));
    EXPECT_TRUE(box->informativeText().contains("Access is denied."));
}

TEST(SettingsDialog, ExportTheme_WritesTheThemeShownWithTheColoursShown)
{
    std::vector<NamedTheme> themes{{.name = "Ocean", .theme = oceanTheme()}};
    SettingsDialog dialog(Settings{.theme = "Ocean"});
    ThemeSources sources = sourcesWith(themes);
    std::filesystem::path written;
    Theme writtenTheme;
    sources.exportTheme = [&written, &writtenTheme](const std::filesystem::path& path, const Theme& theme) {
        written = path;
        writtenTheme = theme;
        return std::optional<std::string>{};
    };
    dialog.setThemeSources(sources);
    dialog.colorButton(ThemeColorRole::Card)->setColor(QColor(0x20, 0x20, 0x20));

    dialog.exportTheme("C:\\Themes\\Mine.json");

    Theme expected = oceanTheme();
    expected.colors[ThemeColorRole::Card] = RgbColor{.red = 0x20, .green = 0x20, .blue = 0x20};
    EXPECT_EQ(written, std::filesystem::path{"C:\\Themes\\Mine.json"});
    EXPECT_EQ(writtenTheme, expected);
    EXPECT_EQ(dialog.findChild<QMessageBox*>(), nullptr);
}

TEST(SettingsDialog, ExportTheme_NotWritten_AWarningSaysWhy)
{
    SettingsDialog dialog(Settings{});
    dialog.setThemeSources(ThemeSources{.exportTheme = [](const std::filesystem::path&, const Theme&) {
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
