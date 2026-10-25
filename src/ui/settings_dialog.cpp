#include "ui/settings_dialog.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QVBoxLayout>

#include "domain/theme.h"
#include "ui/color_button.h"
#include "ui/theme/theme_colors.h"
#include "ui/view_metrics.h"

namespace verdandi::ui
{

namespace
{

// A combo box item: the text it shows and the enumerator it stands for.
template <typename Enum> struct ComboItem
{
    const char* text{};
    Enum value{};
};

constexpr std::array kLogLevelItems{
    ComboItem<domain::LogLevel>{"Trace", domain::LogLevel::Trace},
    ComboItem<domain::LogLevel>{"Debug", domain::LogLevel::Debug},
    ComboItem<domain::LogLevel>{"Info", domain::LogLevel::Info},
    ComboItem<domain::LogLevel>{"Warning", domain::LogLevel::Warn},
    ComboItem<domain::LogLevel>{"Error", domain::LogLevel::Error},
    ComboItem<domain::LogLevel>{"Critical", domain::LogLevel::Critical},
    ComboItem<domain::LogLevel>{"Off", domain::LogLevel::Off},
};

constexpr std::array kColorSchemeItems{
    ComboItem<domain::ColorScheme>{"Follow Windows", domain::ColorScheme::System},
    ComboItem<domain::ColorScheme>{"Light", domain::ColorScheme::Light},
    ComboItem<domain::ColorScheme>{"Dark", domain::ColorScheme::Dark},
};

constexpr std::array kByteUnitsItems{
    ComboItem<domain::ByteUnits>{"Binary (KiB, MiB, GiB)", domain::ByteUnits::Binary},
    ComboItem<domain::ByteUnits>{"Decimal (KB, MB, GB)", domain::ByteUnits::Decimal},
};

constexpr std::array kNetworkRateUnitsItems{
    ComboItem<domain::NetworkRateUnits>{"Bytes per second", domain::NetworkRateUnits::BytesPerSecond},
    ComboItem<domain::NetworkRateUnits>{"Bits per second (Kbps, Mbps, Gbps)", domain::NetworkRateUnits::BitsPerSecond},
};

// A row of the colour list: the theme colour that it chooses, with its name and
// what it colours, and the heading of a group of rows that it begins.
struct ColorRow
{
    const char* heading{};
    domain::ThemeColorRole role{};
    const char* label{};
    const char* toolTip{};
};

constexpr std::array kColorRows{
    ColorRow{"Window and text", domain::ThemeColorRole::Window, "Window",
             "Behind the tabs and the menu bar, and in dialogs. Windows' colour unless chosen."},
    ColorRow{nullptr, domain::ThemeColorRole::Page, "Pages and lists",
             "Tab pages, tables, lists and text boxes. Windows' colour unless chosen."},
    ColorRow{nullptr, domain::ThemeColorRole::Card, "Cards",
             "The dashboard's and the Network tab's cards. The pages' colour unless chosen."},
    ColorRow{nullptr, domain::ThemeColorRole::Text, "Text", "Windows' colour unless chosen."},
    ColorRow{nullptr, domain::ThemeColorRole::Accent, "Accent",
             "Selections, check boxes and the current tab. Windows' accent colour unless chosen."},
    ColorRow{"Charts", domain::ThemeColorRole::ChartLine1, "Line 1",
             "Every chart's first line: CPU, memory, receive, and the first disk volume"},
    ColorRow{nullptr, domain::ThemeColorRole::ChartLine2, "Line 2", "Send, and the second disk volume"},
    ColorRow{nullptr, domain::ThemeColorRole::ChartLine3, "Line 3", "The third disk volume"},
    ColorRow{nullptr, domain::ThemeColorRole::ChartLine4, "Line 4", "The fourth disk volume"},
    ColorRow{nullptr, domain::ThemeColorRole::ChartLine5, "Line 5", "The fifth disk volume"},
    ColorRow{nullptr, domain::ThemeColorRole::ChartGrid, "Grid", "The charts' grid and frame, drawn translucent"},
    ColorRow{"Status", domain::ThemeColorRole::Good, "Good", "Healthy, and adapters that are Up"},
    ColorRow{nullptr, domain::ThemeColorRole::Warning, "Warning", "A resource that needs attention"},
    ColorRow{nullptr, domain::ThemeColorRole::Critical, "Critical", "A resource in a critical state"},
    ColorRow{"Processes", domain::ThemeColorRole::ProcessStarted, "Started",
             "The row of a process that started in the last few seconds, drawn translucent"},
    ColorRow{nullptr, domain::ThemeColorRole::ProcessExited, "Exited",
             "The row of a process that has exited, until it is removed, drawn translucent"},
    ColorRow{nullptr, domain::ThemeColorRole::AccessDenied, "Access denied",
             "The padlock before a process whose details Windows denied"},
    ColorRow{nullptr, domain::ThemeColorRole::AccessNeverOpened, "Never opened",
             "The shield before a process that Verdandi never opens"},
    ColorRow{nullptr, domain::ThemeColorRole::AccessUnavailable, "No details",
             "The barred circle before a process that has no details to read"},
};
static_assert(kColorRows.size() == domain::kThemeColorRoleCount, "the colour list has a row for every theme colour");

// The height of the colour list, which scrolls, in device-independent pixels:
// about a dozen rows, so the dialog stays as tall as its General tab.
constexpr int kColorListHeight = 320;

// What the file dialogs of Import... and Export... offer: theme files, which
// are JSON, as settings files are (DC-152).
constexpr const char* kThemeFileFilter = "Themes (*.json);;All files (*)";
const QString kDefaultThemeFileName = "Verdandi theme.json";

// The folder that the file dialogs open in: the user's documents.
QString themeFolder()
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

// Opens a window-modal warning over parent that says text, then details, such
// as a file's path and why, all as plain text.
void openWarning(QWidget* parent, const QString& title, const QString& text, const QString& details)
{
    auto* box = new QMessageBox(QMessageBox::Warning, title, text, QMessageBox::Ok, parent);
    box->setAttribute(Qt::WA_DeleteOnClose);
    // The path and the problems come from the file and the system; neither is rich text.
    box->setTextFormat(Qt::PlainText);
    box->setInformativeText(details);
    box->open();
}

constexpr double kMillisecondsPerSecond = 1000.0;
// One decimal shows every half-second step, which reaches each of Process
// Explorer's update speeds.
constexpr int kRefreshIntervalDecimals = 1;

double toSeconds(std::chrono::milliseconds interval)
{
    return static_cast<double>(interval.count()) / kMillisecondsPerSecond;
}

template <typename Enum, std::size_t Count>
QComboBox* createComboBox(const std::array<ComboItem<Enum>, Count>& items, QWidget* parent)
{
    auto* comboBox = new QComboBox(parent);
    for (const auto& item : items) {
        comboBox->addItem(item.text, static_cast<int>(item.value));
    }
    return comboBox;
}

template <typename Enum> void selectValue(QComboBox* comboBox, Enum value)
{
    comboBox->setCurrentIndex(comboBox->findData(static_cast<int>(value)));
}

template <typename Enum> Enum selectedValue(const QComboBox* comboBox)
{
    return static_cast<Enum>(comboBox->currentData().toInt());
}

// A heading above a column of the colour list.
QLabel* columnHeading(const QString& text, QWidget* parent)
{
    auto* heading = new QLabel(text, parent);
    QFont headingFont = heading->font();
    headingFont.setBold(true);
    heading->setFont(headingFont);
    heading->setAlignment(Qt::AlignCenter);
    return heading;
}

// Widens the label column of each form to the widest of them, so that the
// fields of one group box line up with those of the next.
void alignLabelColumns(std::initializer_list<const QFormLayout*> forms)
{
    std::vector<QWidget*> labels;
    for (const QFormLayout* form : forms) {
        for (int row = 0; row < form->rowCount(); ++row) {
            const QLayoutItem* item = form->itemAt(row, QFormLayout::LabelRole);
            if (item != nullptr && item->widget() != nullptr) {
                labels.push_back(item->widget());
            }
        }
    }

    int width = 0;
    for (const QWidget* label : labels) {
        width = std::max(width, label->sizeHint().width());
    }
    for (QWidget* label : labels) {
        label->setMinimumWidth(width);
    }
}

} // namespace

SettingsDialog::SettingsDialog(const domain::Settings& settings, QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Settings");

    m_refreshIntervalSpinBox = new QDoubleSpinBox(this);
    // A decimal point whatever the system's locale, as everywhere else in the UI (DC-57).
    m_refreshIntervalSpinBox->setLocale(QLocale::c());
    m_refreshIntervalSpinBox->setDecimals(kRefreshIntervalDecimals);
    m_refreshIntervalSpinBox->setRange(toSeconds(domain::kMinRefreshInterval), toSeconds(domain::kMaxRefreshInterval));
    m_refreshIntervalSpinBox->setSingleStep(toSeconds(domain::kRefreshIntervalStep));
    m_refreshIntervalSpinBox->setSuffix(" s");
    m_refreshIntervalSpinBox->setToolTip(
        "How often CPU, memory, disk and network use are read and shown, in steps of half a second");
    // A value typed between two steps shows the step that OK would save.
    connect(m_refreshIntervalSpinBox, &QDoubleSpinBox::editingFinished, this,
            [this] { m_refreshIntervalSpinBox->setValue(toSeconds(shownRefreshInterval())); });
    m_logLevelComboBox = createComboBox(kLogLevelItems, this);
    m_logLevelComboBox->setToolTip("The least severe messages that the log file keeps");

    auto* generalGroup = new QGroupBox("General", this);
    auto* generalLayout = new QFormLayout(generalGroup);
    generalLayout->addRow("&Refresh interval:", m_refreshIntervalSpinBox);
    generalLayout->addRow("&Log level:", m_logLevelComboBox);

    m_byteUnitsComboBox = createComboBox(kByteUnitsItems, this);
    m_byteUnitsComboBox->setToolTip("The units of memory, disk space, and I/O and network rates in bytes");
    m_networkRateUnitsComboBox = createComboBox(kNetworkRateUnitsItems, this);
    m_networkRateUnitsComboBox->setToolTip("Bits per second are decimal, as link speeds are");

    auto* unitsGroup = new QGroupBox("Units", this);
    auto* unitsLayout = new QFormLayout(unitsGroup);
    unitsLayout->addRow("&Byte units:", m_byteUnitsComboBox);
    unitsLayout->addRow("&Network throughput:", m_networkRateUnitsComboBox);
    alignLabelColumns({generalLayout, unitsLayout});

    // Named as the dashboard's cards are.
    m_cpuPanelCheckBox = new QCheckBox("&CPU", this);
    m_memoryPanelCheckBox = new QCheckBox("&Memory", this);
    m_diskPanelCheckBox = new QCheckBox("&Disk", this);
    m_networkPanelCheckBox = new QCheckBox("N&etwork", this);
    m_uptimePanelCheckBox = new QCheckBox("&Uptime", this);

    auto* dashboardGroup = new QGroupBox("Dashboard cards", this);
    auto* dashboardLayout = new QVBoxLayout(dashboardGroup);
    dashboardLayout->addWidget(m_cpuPanelCheckBox);
    dashboardLayout->addWidget(m_memoryPanelCheckBox);
    dashboardLayout->addWidget(m_diskPanelCheckBox);
    dashboardLayout->addWidget(m_networkPanelCheckBox);
    dashboardLayout->addWidget(m_uptimePanelCheckBox);

    auto* generalPage = new QWidget(this);
    auto* generalPageLayout = new QVBoxLayout(generalPage);
    generalPageLayout->setSpacing(kSectionSpacing);
    generalPageLayout->addWidget(generalGroup);
    generalPageLayout->addWidget(unitsGroup);
    generalPageLayout->addWidget(dashboardGroup);
    generalPageLayout->addStretch();

    m_colorSchemeComboBox = createComboBox(kColorSchemeItems, this);
    m_colorSchemeComboBox->setToolTip(
        "Light or dark, or the one Windows sets for apps. A Windows contrast theme replaces both.");
    auto* colorSchemeLayout = new QFormLayout();
    colorSchemeLayout->addRow("Colour &scheme:", m_colorSchemeComboBox);

    auto* appearancePage = new QWidget(this);
    auto* appearancePageLayout = new QVBoxLayout(appearancePage);
    appearancePageLayout->setSpacing(kSectionSpacing);
    appearancePageLayout->addLayout(colorSchemeLayout);
    appearancePageLayout->addWidget(createColorsGroup());

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(generalPage, "General");
    m_tabWidget->addTab(appearancePage, "Appearance");

    m_buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
            [this] { setSettings(domain::Settings{}); });

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(kSectionSpacing);
    layout->addWidget(m_tabWidget);
    layout->addWidget(m_buttonBox);
    // The list of colours scrolls, and nothing else gains from more room, so
    // the dialog keeps its size.
    layout->setSizeConstraint(QLayout::SetFixedSize);

    setSettings(settings);
}

QGroupBox* SettingsDialog::createColorsGroup()
{
    auto* list = new QWidget(this);
    auto* grid = new QGridLayout(list);
    grid->setHorizontalSpacing(kItemSpacing);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    int row = 0;
    grid->addWidget(columnHeading("Light", list), row, 1);
    grid->addWidget(columnHeading("Dark", list), row, 2);
    ++row;

    for (const ColorRow& entry : kColorRows) {
        if (entry.heading != nullptr) {
            auto* heading = new QLabel(entry.heading, list);
            QFont headingFont = heading->font();
            headingFont.setBold(true);
            heading->setFont(headingFont);
            grid->addWidget(heading, row, 0, 1, 3);
            ++row;
        }
        auto* label = new QLabel(entry.label, list);
        label->setToolTip(entry.toolTip);
        grid->addWidget(label, row, 0);
        for (const bool isDark : {false, true}) {
            auto* button = new ColorButton(list);
            button->setDefaultColor(theme::defaultColor(entry.role, isDark));
            button->setDialogTitle(QString("%1 in the %2 scheme").arg(entry.label, isDark ? "dark" : "light"));
            button->setToolTip(entry.toolTip);
            grid->addWidget(button, row, isDark ? 2 : 1);
            m_colorButtons[isDark ? 1 : 0][static_cast<std::size_t>(entry.role)] = button;
        }
        ++row;
    }

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(list);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFixedHeight(kColorListHeight);
    // As wide as the list and the scroll bar beside it, so nothing is cut off.
    scrollArea->setMinimumWidth(list->sizeHint().width() + scrollArea->verticalScrollBar()->sizeHint().width());

    m_importThemeButton = new QPushButton("&Import...", this);
    m_importThemeButton->setToolTip("Show the colours of a theme file, or of a settings file");
    m_importThemeButton->setEnabled(false);
    connect(m_importThemeButton, &QPushButton::clicked, this, &SettingsDialog::chooseThemeToImport);
    m_exportThemeButton = new QPushButton("E&xport...", this);
    m_exportThemeButton->setToolTip("Write the colours shown here to a theme file, to share them or keep a copy");
    m_exportThemeButton->setEnabled(false);
    connect(m_exportThemeButton, &QPushButton::clicked, this, &SettingsDialog::chooseThemeFileToExport);
    m_resetColorsButton = new QPushButton("Reset C&olours", this);
    m_resetColorsButton->setToolTip("Every colour back to its default, in both schemes");
    connect(m_resetColorsButton, &QPushButton::clicked, this, [this] { setTheme(domain::Theme{}); });
    auto* buttonRow = new QHBoxLayout();
    buttonRow->addWidget(m_importThemeButton);
    buttonRow->addWidget(m_exportThemeButton);
    buttonRow->addStretch();
    buttonRow->addWidget(m_resetColorsButton);

    auto* group = new QGroupBox("Colours", this);
    auto* groupLayout = new QVBoxLayout(group);
    groupLayout->addWidget(scrollArea);
    groupLayout->addLayout(buttonRow);
    return group;
}

void SettingsDialog::setSettings(const domain::Settings& settings)
{
    m_refreshIntervalSpinBox->setValue(toSeconds(domain::nearestRefreshInterval(settings.refreshInterval)));

    selectValue(m_logLevelComboBox, settings.logLevel);
    selectValue(m_colorSchemeComboBox, settings.colorScheme);
    selectValue(m_byteUnitsComboBox, settings.displayUnits.bytes);
    selectValue(m_networkRateUnitsComboBox, settings.displayUnits.networkRates);

    m_cpuPanelCheckBox->setChecked(settings.dashboardPanels.cpu);
    m_memoryPanelCheckBox->setChecked(settings.dashboardPanels.memory);
    m_diskPanelCheckBox->setChecked(settings.dashboardPanels.disk);
    m_networkPanelCheckBox->setChecked(settings.dashboardPanels.network);
    m_uptimePanelCheckBox->setChecked(settings.dashboardPanels.uptime);

    setTheme(settings.theme);
}

void SettingsDialog::setThemeFiles(ThemeFiles files)
{
    m_themeFiles = std::move(files);
    m_importThemeButton->setEnabled(static_cast<bool>(m_themeFiles.importTheme));
    m_exportThemeButton->setEnabled(static_cast<bool>(m_themeFiles.exportTheme));
}

void SettingsDialog::importTheme(const std::filesystem::path& path)
{
    if (!m_themeFiles.importTheme) {
        return;
    }
    const ThemeImport imported = m_themeFiles.importTheme(path);
    if (imported.theme) {
        setTheme(*imported.theme);
        if (imported.problems.empty()) {
            return;
        }
    }

    QStringList details{QDir::toNativeSeparators(QString::fromStdWString(path.wstring()))};
    for (const std::string& problem : imported.problems) {
        details.append(QString::fromStdString(problem));
    }
    openWarning(this, "Import Colours",
                imported.theme ? "The colours were imported, but some in the file were not used."
                               : "No colours were imported.",
                details.join('\n'));
}

void SettingsDialog::exportTheme(const std::filesystem::path& path)
{
    if (!m_themeFiles.exportTheme) {
        return;
    }
    if (const std::optional<std::string> error = m_themeFiles.exportTheme(path, shownTheme())) {
        openWarning(this, "Export Colours", "The colours could not be exported.",
                    QString("%1\n%2").arg(QDir::toNativeSeparators(QString::fromStdWString(path.wstring())),
                                          QString::fromStdString(*error)));
    }
}

void SettingsDialog::chooseThemeToImport()
{
    const QString path =
        QFileDialog::getOpenFileName(this, "Import Colours", themeFolder(), QString::fromLatin1(kThemeFileFilter));
    if (!path.isEmpty()) {
        importTheme(std::filesystem::path{path.toStdWString()});
    }
}

void SettingsDialog::chooseThemeFileToExport()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Export Colours", themeFolder() + "/" + kDefaultThemeFileName, QString::fromLatin1(kThemeFileFilter));
    if (!path.isEmpty()) {
        exportTheme(std::filesystem::path{path.toStdWString()});
    }
}

void SettingsDialog::setTheme(const domain::Theme& colors)
{
    for (const bool isDark : {false, true}) {
        const domain::ThemeColors& schemeColors = colors.colors(isDark);
        for (std::size_t role = 0; role < domain::kThemeColorRoleCount; ++role) {
            const std::optional<domain::RgbColor>& chosen = schemeColors.colors[role];
            m_colorButtons[isDark ? 1 : 0][role]->setColor(chosen ? std::optional{theme::toQColor(*chosen)}
                                                                  : std::nullopt);
        }
    }
}

domain::Theme SettingsDialog::shownTheme() const
{
    domain::Theme shown;
    for (const bool isDark : {false, true}) {
        domain::ThemeColors& schemeColors = isDark ? shown.dark : shown.light;
        for (std::size_t role = 0; role < domain::kThemeColorRoleCount; ++role) {
            if (const std::optional<QColor>& chosen = m_colorButtons[isDark ? 1 : 0][role]->color()) {
                schemeColors.colors[role] = theme::toRgbColor(*chosen);
            }
        }
    }
    return shown;
}

domain::Settings SettingsDialog::settings() const
{
    return domain::Settings{
        .refreshInterval = shownRefreshInterval(),
        .displayUnits =
            {
                .bytes = selectedValue<domain::ByteUnits>(m_byteUnitsComboBox),
                .networkRates = selectedValue<domain::NetworkRateUnits>(m_networkRateUnitsComboBox),
            },
        .dashboardPanels =
            {
                .cpu = m_cpuPanelCheckBox->isChecked(),
                .memory = m_memoryPanelCheckBox->isChecked(),
                .disk = m_diskPanelCheckBox->isChecked(),
                .network = m_networkPanelCheckBox->isChecked(),
                .uptime = m_uptimePanelCheckBox->isChecked(),
            },
        .colorScheme = selectedValue<domain::ColorScheme>(m_colorSchemeComboBox),
        .theme = shownTheme(),
        .logLevel = selectedValue<domain::LogLevel>(m_logLevelComboBox),
    };
}

std::chrono::milliseconds SettingsDialog::shownRefreshInterval() const
{
    return domain::nearestRefreshInterval(
        std::chrono::milliseconds{std::lround(m_refreshIntervalSpinBox->value() * kMillisecondsPerSecond)});
}

QDoubleSpinBox* SettingsDialog::refreshIntervalSpinBox() const
{
    return m_refreshIntervalSpinBox;
}

QComboBox* SettingsDialog::logLevelComboBox() const
{
    return m_logLevelComboBox;
}

QComboBox* SettingsDialog::colorSchemeComboBox() const
{
    return m_colorSchemeComboBox;
}

ColorButton* SettingsDialog::colorButton(domain::ThemeColorRole role, bool darkScheme) const
{
    return m_colorButtons[darkScheme ? 1 : 0][static_cast<std::size_t>(role)];
}

QPushButton* SettingsDialog::resetColorsButton() const
{
    return m_resetColorsButton;
}

QPushButton* SettingsDialog::importThemeButton() const
{
    return m_importThemeButton;
}

QPushButton* SettingsDialog::exportThemeButton() const
{
    return m_exportThemeButton;
}

QTabWidget* SettingsDialog::tabWidget() const
{
    return m_tabWidget;
}

QComboBox* SettingsDialog::byteUnitsComboBox() const
{
    return m_byteUnitsComboBox;
}

QComboBox* SettingsDialog::networkRateUnitsComboBox() const
{
    return m_networkRateUnitsComboBox;
}

QCheckBox* SettingsDialog::cpuPanelCheckBox() const
{
    return m_cpuPanelCheckBox;
}

QCheckBox* SettingsDialog::memoryPanelCheckBox() const
{
    return m_memoryPanelCheckBox;
}

QCheckBox* SettingsDialog::diskPanelCheckBox() const
{
    return m_diskPanelCheckBox;
}

QCheckBox* SettingsDialog::networkPanelCheckBox() const
{
    return m_networkPanelCheckBox;
}

QCheckBox* SettingsDialog::uptimePanelCheckBox() const
{
    return m_uptimePanelCheckBox;
}

QDialogButtonBox* SettingsDialog::buttonBox() const
{
    return m_buttonBox;
}

} // namespace verdandi::ui
