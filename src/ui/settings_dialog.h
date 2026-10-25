#pragma once

#include <array>
#include <chrono>
#include <cstddef>

#include <QDialog>

#include "domain/settings.h"
#include "domain/theme.h"

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
 * scrolls, and Reset Colours, which sets every colour to its default
 * (DC-151). OK accepts, Cancel and Escape reject, and Restore Defaults sets
 * every control to the defaults without closing the dialog. The dialog
 * changes nothing itself: whoever opened it reads settings() once it is
 * accepted.
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
    [[nodiscard]] QPushButton* resetColorsButton() const;
    [[nodiscard]] QDialogButtonBox* buttonBox() const;

private:
    // The group of the Appearance tab with the list of colours and Reset Colours.
    QGroupBox* createColorsGroup();
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
    QPushButton* m_resetColorsButton{nullptr};
    QDialogButtonBox* m_buttonBox{nullptr};
};

} // namespace verdandi::ui
