#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <QDialog>

#include "domain/settings.h"
#include "domain/theme.h"
#include "ui/theme_sources.h"

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QGroupBox;
class QPushButton;
class QTabWidget;

namespace verdandi::ui
{

class ColorButton;
class ColorSample;

/**
 * Settings dialog: edits a copy of the user's preferences (DC-124). Its
 * General tab shows the refresh interval in seconds, the log level, the byte
 * and network throughput units, and a check box for each dashboard card. Its
 * Appearance tab shows the theme, from the built-in ones and the themes
 * folder's (DC-149, DC-153), and a list of the colours of the theme that it
 * draws with, Light or Dark for Follow Windows, with a ColorButton for each,
 * which scrolls (DC-151). The buttons show the user's colour, or the theme's
 * own as the default, and a ColorSample beside each shows its colour as the
 * monitor draws it, with the colours that the list shows, or Windows' for the
 * palette's roles that neither the theme nor the user chose. Choosing
 * another theme keeps the colours chosen in the one before. Below the list, Import... adds a theme file to the themes
 * folder and chooses it, Export... writes the theme shown, with the colours
 * shown, to a theme file (DC-152), and Reset Colours sets every colour of the
 * theme shown to the theme's own. OK accepts, Cancel and Escape reject, and
 * Restore Defaults sets every control to the defaults without closing the
 * dialog. The dialog changes no settings itself: whoever opened it reads
 * settings() once it is accepted. Import... writes to the themes folder at
 * once, and Export... writes only the file chosen.
 *
 * The interval is chosen in steps of half a second
 * (domain::nearestRefreshInterval()): setSettings() shows a given interval as
 * the nearest step, a value typed between two steps moves to the nearer one
 * when editing ends, and settings() returns the step the spin box is nearest.
 *
 * Each combo box's items hold their enumerator, cast to int, as item data,
 * except the theme list's, which hold the theme's name.
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /** Opens with the controls showing settings. */
    explicit SettingsDialog(const domain::Settings& settings, QWidget* parent = nullptr);
    ~SettingsDialog() override = default;

    /** Sets every control to show settings, as Restore Defaults does with the defaults. */
    void setSettings(const domain::Settings& settings);

    /** Returns the settings that the controls show. */
    [[nodiscard]] domain::Settings settings() const;

    /**
     * Sets where the dialog finds the themes of the themes folder and the
     * scheme that Windows sets, and the functions that Import... and
     * Export... read and write theme files with. The theme list offers the
     * themes then, and each button is disabled while a function it needs is
     * empty, as until this is called.
     */
    void setThemeSources(ThemeSources sources);

    /**
     * Imports the theme file at path, as Import... does with the file that
     * its dialog chose: adds its theme to the themes folder, named after the
     * file, and chooses it, with the theme's own colours. A theme of that
     * name is replaced if the user agrees in a window-modal question. A
     * window-modal message box says why no theme was imported, as when the
     * file holds none or its name is a built-in theme's, or which values in
     * the file were not used.
     */
    void importTheme(const std::filesystem::path& path);

    /**
     * Writes the theme shown, with the colours that the colour buttons show,
     * to the theme file at path, as Export... does with the file that its
     * dialog chose. When it cannot, a window-modal message box says why.
     */
    void exportTheme(const std::filesystem::path& path);

    /** Returns the name of the theme whose colours the colour buttons show: the theme chosen, or Light or Dark for
     * Follow Windows. */
    [[nodiscard]] const std::string& shownThemeName() const;

    [[nodiscard]] QTabWidget* tabWidget() const;
    [[nodiscard]] QDoubleSpinBox* refreshIntervalSpinBox() const;
    [[nodiscard]] QComboBox* logLevelComboBox() const;
    /** Returns the list of themes, whose items hold each theme's name as item data. */
    [[nodiscard]] QComboBox* themeComboBox() const;
    [[nodiscard]] QComboBox* byteUnitsComboBox() const;
    [[nodiscard]] QComboBox* networkRateUnitsComboBox() const;
    [[nodiscard]] QCheckBox* cpuPanelCheckBox() const;
    [[nodiscard]] QCheckBox* memoryPanelCheckBox() const;
    [[nodiscard]] QCheckBox* diskPanelCheckBox() const;
    [[nodiscard]] QCheckBox* networkPanelCheckBox() const;
    [[nodiscard]] QCheckBox* uptimePanelCheckBox() const;
    /** Returns the button that chooses role's colour in the theme shown. */
    [[nodiscard]] ColorButton* colorButton(domain::ThemeColorRole role) const;
    /** Returns the sample of role's colour beside its button. */
    [[nodiscard]] ColorSample* colorSample(domain::ThemeColorRole role) const;
    [[nodiscard]] QGroupBox* colorsGroup() const;
    [[nodiscard]] QPushButton* importThemeButton() const;
    [[nodiscard]] QPushButton* exportThemeButton() const;
    [[nodiscard]] QPushButton* resetColorsButton() const;
    [[nodiscard]] QDialogButtonBox* buttonBox() const;

private:
    // The group of the Appearance tab with the list of colours and its buttons.
    QGroupBox* createColorsGroup();
    // Ask for a file in a file dialog, then import or export it.
    void chooseThemeToImport();
    void chooseThemeFileToExport();
    // Adds theme, read from the file at path with problems, to the themes
    // folder, and chooses it.
    void addImportedTheme(const domain::NamedTheme& theme, const std::filesystem::path& path,
                          const std::vector<std::string>& problems);
    // Fills the theme list with the built-in themes and m_themes, and selects
    // m_chosenTheme, or Follow Windows if the list has no theme of that name.
    void fillThemeList();
    // Makes the colour buttons show the colours of the theme named name, and
    // the user's colours in it.
    void showTheme(const std::string& name);
    // Keeps the colours that the colour buttons show as the user's colours in the theme shown.
    void keepShownColors();
    // Gives the samples the colours that the colour buttons show.
    void showSampleColors();
    // The theme that the colour buttons show for the theme chosen.
    [[nodiscard]] std::string themeToShow() const;
    // The theme named name, with its own colours: a built-in one, or one of m_themes.
    [[nodiscard]] domain::Theme themeNamed(std::string_view name) const;
    // The colours that the colour buttons show the user chose.
    [[nodiscard]] domain::ThemeColors shownColors() const;
    // The interval that can be chosen nearest to the one the spin box shows.
    [[nodiscard]] std::chrono::milliseconds shownRefreshInterval() const;

    QTabWidget* m_tabWidget{nullptr};
    QDoubleSpinBox* m_refreshIntervalSpinBox{nullptr};
    QComboBox* m_logLevelComboBox{nullptr};
    QComboBox* m_themeComboBox{nullptr};
    QComboBox* m_byteUnitsComboBox{nullptr};
    QComboBox* m_networkRateUnitsComboBox{nullptr};
    QCheckBox* m_cpuPanelCheckBox{nullptr};
    QCheckBox* m_memoryPanelCheckBox{nullptr};
    QCheckBox* m_diskPanelCheckBox{nullptr};
    QCheckBox* m_networkPanelCheckBox{nullptr};
    QCheckBox* m_uptimePanelCheckBox{nullptr};
    // The colour buttons, each indexed by its role's value.
    std::array<ColorButton*, domain::kThemeColorRoleCount> m_colorButtons{};
    std::array<ColorSample*, domain::kThemeColorRoleCount> m_colorSamples{};
    QGroupBox* m_colorsGroup{nullptr};
    QPushButton* m_importThemeButton{nullptr};
    QPushButton* m_exportThemeButton{nullptr};
    QPushButton* m_resetColorsButton{nullptr};
    QDialogButtonBox* m_buttonBox{nullptr};
    ThemeSources m_themeSources{};
    // The themes of the themes folder, from m_themeSources.
    std::vector<domain::NamedTheme> m_themes;
    // The theme chosen, which the theme list selects when it has it.
    std::string m_chosenTheme{domain::kFollowWindowsTheme};
    // The theme whose colours the colour buttons show.
    std::string m_shownTheme{domain::kLightTheme};
    // The colours that the user chose in each theme, except in the theme shown, whose the colour buttons hold.
    std::map<std::string, domain::ThemeColors, std::less<>> m_userColors;
};

} // namespace verdandi::ui
