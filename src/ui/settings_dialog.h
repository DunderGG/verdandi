#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>

#include <QDialog>

#include "domain/settings.h"
#include "domain/theme.h"
#include "ui/theme_files.h"

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

/**
 * Settings dialog: edits a copy of the user's preferences (DC-124). Its
 * General tab shows the refresh interval in seconds, the log level, the byte
 * and network throughput units, and a check box for each dashboard card. Its
 * Appearance tab shows the colour scheme, and a list of the theme's colours
 * with a ColorButton for each in the light and in the dark scheme, which
 * scrolls (DC-151). Below it, Import... shows the colours of a theme file,
 * Export... writes the colours shown to one (DC-152), and Reset Colours sets
 * every colour to its default. OK accepts, Cancel and Escape reject, and
 * Restore Defaults sets every control to the defaults without closing the
 * dialog. The dialog changes nothing itself: whoever opened it reads
 * settings() once it is accepted, and Export... writes only the file chosen.
 *
 * The interval is chosen in steps of half a second
 * (domain::nearestRefreshInterval()): setSettings() shows a given interval as
 * the nearest step, a value typed between two steps moves to the nearer one
 * when editing ends, and settings() returns the step the spin box is nearest.
 *
 * Each combo box's items hold their enumerator, cast to int, as item data.
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
     * Sets the functions that Import... and Export... read and write theme
     * files with. Each one's button is disabled while its function is empty,
     * as until this is called.
     */
    void setThemeFiles(ThemeFiles files);

    /**
     * Reads the theme file at path, as Import... does with the file that its
     * dialog chose: the colour buttons show the colours read, and nothing
     * else changes. When some colours in the file were not used, or none
     * could be read, a window-modal message box says why.
     */
    void importTheme(const std::filesystem::path& path);

    /**
     * Writes the colours that the colour buttons show to the theme file at
     * path, as Export... does with the file that its dialog chose. When it
     * cannot, a window-modal message box says why.
     */
    void exportTheme(const std::filesystem::path& path);

    [[nodiscard]] QTabWidget* tabWidget() const;
    [[nodiscard]] QDoubleSpinBox* refreshIntervalSpinBox() const;
    [[nodiscard]] QComboBox* logLevelComboBox() const;
    [[nodiscard]] QComboBox* colorSchemeComboBox() const;
    [[nodiscard]] QComboBox* byteUnitsComboBox() const;
    [[nodiscard]] QComboBox* networkRateUnitsComboBox() const;
    [[nodiscard]] QCheckBox* cpuPanelCheckBox() const;
    [[nodiscard]] QCheckBox* memoryPanelCheckBox() const;
    [[nodiscard]] QCheckBox* diskPanelCheckBox() const;
    [[nodiscard]] QCheckBox* networkPanelCheckBox() const;
    [[nodiscard]] QCheckBox* uptimePanelCheckBox() const;
    /** Returns the button that chooses role's colour in the dark scheme, or in the light one if darkScheme is false. */
    [[nodiscard]] ColorButton* colorButton(domain::ThemeColorRole role, bool darkScheme) const;
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
    // Sets the colour buttons to show colors.
    void setTheme(const domain::Theme& colors);
    // The colours that the colour buttons show.
    [[nodiscard]] domain::Theme shownTheme() const;
    // The interval that can be chosen nearest to the one the spin box shows.
    [[nodiscard]] std::chrono::milliseconds shownRefreshInterval() const;

    QTabWidget* m_tabWidget{nullptr};
    QDoubleSpinBox* m_refreshIntervalSpinBox{nullptr};
    QComboBox* m_logLevelComboBox{nullptr};
    QComboBox* m_colorSchemeComboBox{nullptr};
    QComboBox* m_byteUnitsComboBox{nullptr};
    QComboBox* m_networkRateUnitsComboBox{nullptr};
    QCheckBox* m_cpuPanelCheckBox{nullptr};
    QCheckBox* m_memoryPanelCheckBox{nullptr};
    QCheckBox* m_diskPanelCheckBox{nullptr};
    QCheckBox* m_networkPanelCheckBox{nullptr};
    QCheckBox* m_uptimePanelCheckBox{nullptr};
    // The colour buttons of the light scheme, then of the dark one, each indexed by its role's value.
    std::array<std::array<ColorButton*, domain::kThemeColorRoleCount>, 2> m_colorButtons{};
    QPushButton* m_importThemeButton{nullptr};
    QPushButton* m_exportThemeButton{nullptr};
    QPushButton* m_resetColorsButton{nullptr};
    QDialogButtonBox* m_buttonBox{nullptr};
    ThemeFiles m_themeFiles{};
};

} // namespace verdandi::ui
