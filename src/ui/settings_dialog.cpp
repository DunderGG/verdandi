#include "ui/settings_dialog.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <QAbstractButton>
#include <QApplication>
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
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLocale>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QStyleHints>
#include <QTabWidget>
#include <QVBoxLayout>

#include "domain/theme.h"
#include "ui/color_button.h"
#include "ui/color_sample.h"
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
    ColorRow{nullptr, domain::ThemeColorRole::TableHeader, "Table headers",
             "The names above the process list's columns, in the text or the window colour. Windows' colour "
             "unless chosen."},
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

// The folder that the file dialogs open in: the user's documents.
QString themeFolder()
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

QString toQString(std::string_view text)
{
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

// The name that Export... offers for a file of the theme named name. Its
// name, which names the theme it imports as, unless a built-in theme's,
// which no theme file can take.
QString defaultThemeFileName(const std::string& name)
{
    const QString themeName = QString::fromStdString(name);
    return QString("%1.json").arg(domain::isBuiltInThemeName(name) ? QString("My %1 theme").arg(themeName.toLower())
                                                                   : themeName);
}

QString nativePath(const std::filesystem::path& path)
{
    return QDir::toNativeSeparators(QString::fromStdWString(path.wstring()));
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

    m_themeComboBox = new QComboBox(this);
    m_themeComboBox->setToolTip("Light or Dark, the one of them that Windows sets for apps, or a theme of the themes "
                                "folder beside verdandi.exe. A Windows contrast theme replaces them all.");
    // fillThemeList() blocks the signal, so this follows the user's choice.
    connect(m_themeComboBox, &QComboBox::currentIndexChanged, this, [this] {
        keepShownColors();
        m_chosenTheme = m_themeComboBox->currentData().toString().toStdString();
        showTheme(themeToShow());
    });
    auto* themeLayout = new QFormLayout();
    themeLayout->addRow("&Theme:", m_themeComboBox);

    auto* appearancePage = new QWidget(this);
    auto* appearancePageLayout = new QVBoxLayout(appearancePage);
    appearancePageLayout->setSpacing(kSectionSpacing);
    appearancePageLayout->addLayout(themeLayout);
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
    grid->setColumnStretch(2, 1);
    int row = 0;
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
        // showTheme() gives each its default colour and its dialog's title.
        auto* button = new ColorButton(list);
        button->setToolTip(entry.toolTip);
        grid->addWidget(button, row, 1);
        connect(button, &ColorButton::colorChanged, this, &SettingsDialog::showSampleColors);
        auto* sample = new ColorSample(entry.role, list);
        sample->setToolTip(entry.toolTip);
        grid->addWidget(sample, row, 2);
        m_colorButtons[static_cast<std::size_t>(entry.role)] = button;
        m_colorSamples[static_cast<std::size_t>(entry.role)] = sample;
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
    m_importThemeButton->setToolTip("Add a theme file to the themes folder, and choose its theme");
    m_importThemeButton->setEnabled(false);
    connect(m_importThemeButton, &QPushButton::clicked, this, &SettingsDialog::chooseThemeToImport);
    m_exportThemeButton = new QPushButton("E&xport...", this);
    m_exportThemeButton->setToolTip(
        "Write the theme shown here, with the colours shown, to a theme file, to share it or keep a copy");
    m_exportThemeButton->setEnabled(false);
    connect(m_exportThemeButton, &QPushButton::clicked, this, &SettingsDialog::chooseThemeFileToExport);
    m_resetColorsButton = new QPushButton("Reset C&olours", this);
    m_resetColorsButton->setToolTip("Every colour of the theme shown back to the theme's own");
    connect(m_resetColorsButton, &QPushButton::clicked, this, [this] {
        for (ColorButton* button : m_colorButtons) {
            button->setColor(std::nullopt);
        }
        showSampleColors();
    });
    auto* buttonRow = new QHBoxLayout();
    buttonRow->addWidget(m_importThemeButton);
    buttonRow->addWidget(m_exportThemeButton);
    buttonRow->addStretch();
    buttonRow->addWidget(m_resetColorsButton);

    // showTheme() names the theme in the title.
    m_colorsGroup = new QGroupBox("Colours", this);
    auto* groupLayout = new QVBoxLayout(m_colorsGroup);
    groupLayout->addWidget(scrollArea);
    groupLayout->addLayout(buttonRow);
    return m_colorsGroup;
}

void SettingsDialog::setSettings(const domain::Settings& settings)
{
    m_refreshIntervalSpinBox->setValue(toSeconds(domain::nearestRefreshInterval(settings.refreshInterval)));

    selectValue(m_logLevelComboBox, settings.logLevel);
    selectValue(m_byteUnitsComboBox, settings.displayUnits.bytes);
    selectValue(m_networkRateUnitsComboBox, settings.displayUnits.networkRates);

    m_cpuPanelCheckBox->setChecked(settings.dashboardPanels.cpu);
    m_memoryPanelCheckBox->setChecked(settings.dashboardPanels.memory);
    m_diskPanelCheckBox->setChecked(settings.dashboardPanels.disk);
    m_networkPanelCheckBox->setChecked(settings.dashboardPanels.network);
    m_uptimePanelCheckBox->setChecked(settings.dashboardPanels.uptime);

    m_userColors = settings.themeColors;
    m_chosenTheme = settings.theme;
    fillThemeList();
    showTheme(themeToShow());
}

void SettingsDialog::setThemeSources(ThemeSources sources)
{
    keepShownColors();
    m_themeSources = std::move(sources);
    m_themes = m_themeSources.themes ? m_themeSources.themes() : std::vector<domain::NamedTheme>{};
    m_importThemeButton->setEnabled(m_themeSources.themes && m_themeSources.readTheme && m_themeSources.addTheme);
    m_exportThemeButton->setEnabled(static_cast<bool>(m_themeSources.exportTheme));
    fillThemeList();
    showTheme(themeToShow());
}

void SettingsDialog::importTheme(const std::filesystem::path& path)
{
    if (!m_importThemeButton->isEnabled()) {
        return;
    }
    const ThemeImport imported = m_themeSources.readTheme(path);
    if (!imported.theme) {
        QStringList details{nativePath(path)};
        for (const std::string& problem : imported.problems) {
            details.append(QString::fromStdString(problem));
        }
        openWarning(this, "Import Theme", "No theme was imported.", details.join('\n'));
        return;
    }

    // A theme is named after its file, as in the themes folder (DC-153).
    const QString name = QString::fromStdWString(path.stem().wstring());
    if (domain::isBuiltInThemeName(name.toStdString())) {
        openWarning(this, "Import Theme", "No theme was imported.",
                    QString("%1\n%2 is the name of a built-in theme. Rename the file to import it.")
                        .arg(nativePath(path), name));
        return;
    }
    domain::NamedTheme theme{.name = name.toStdString(), .theme = *imported.theme};
    // Windows' file names ignore case, so a theme whose name differs only in
    // case is the same file, which keeps the name it has.
    const auto existing = std::ranges::find_if(m_themes, [&name](const domain::NamedTheme& candidate) {
        return QString::fromStdString(candidate.name).compare(name, Qt::CaseInsensitive) == 0;
    });
    if (existing == m_themes.end()) {
        addImportedTheme(theme, path, imported.problems);
        return;
    }
    theme.name = existing->name;

    auto* box = new QMessageBox(
        QMessageBox::Question, "Import Theme",
        QString("There is a theme called %1 already. Replace it?").arg(QString::fromStdString(theme.name)),
        QMessageBox::Yes | QMessageBox::No, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    // The name comes from a file, so it is not rich text.
    box->setTextFormat(Qt::PlainText);
    box->setInformativeText("The theme takes the file's colours, in place of its own and of those you chose in it.");
    box->setDefaultButton(QMessageBox::No);
    connect(box, &QMessageBox::buttonClicked, this,
            [this, box, theme, path, problems = imported.problems](const QAbstractButton* button) {
                if (button == box->button(QMessageBox::Yes)) {
                    addImportedTheme(theme, path, problems);
                }
            });
    box->open();
}

void SettingsDialog::addImportedTheme(const domain::NamedTheme& theme, const std::filesystem::path& path,
                                      const std::vector<std::string>& problems)
{
    if (const std::optional<std::string> error = m_themeSources.addTheme(theme)) {
        openWarning(this, "Import Theme", "The theme could not be added to the themes folder.",
                    QString("%1\n%2").arg(nativePath(path), QString::fromStdString(*error)));
        return;
    }

    keepShownColors();
    // The theme shows the file's colours.
    m_userColors.erase(theme.name);
    m_themes = m_themeSources.themes();
    m_chosenTheme = theme.name;
    fillThemeList();
    showTheme(themeToShow());

    if (!problems.empty()) {
        QStringList details{nativePath(path)};
        for (const std::string& problem : problems) {
            details.append(QString::fromStdString(problem));
        }
        openWarning(this, "Import Theme", "The theme was imported, but some values in the file were not used.",
                    details.join('\n'));
    }
}

void SettingsDialog::exportTheme(const std::filesystem::path& path)
{
    if (!m_themeSources.exportTheme) {
        return;
    }
    domain::Theme theme = themeNamed(m_shownTheme);
    theme.colors = theme.colors.withChanges(shownColors());
    if (const std::optional<std::string> error = m_themeSources.exportTheme(path, theme)) {
        openWarning(this, "Export Theme", "The theme could not be exported.",
                    QString("%1\n%2").arg(nativePath(path), QString::fromStdString(*error)));
    }
}

const std::string& SettingsDialog::shownThemeName() const
{
    return m_shownTheme;
}

void SettingsDialog::chooseThemeToImport()
{
    const QString path =
        QFileDialog::getOpenFileName(this, "Import Theme", themeFolder(), QString::fromLatin1(kThemeFileFilter));
    if (!path.isEmpty()) {
        importTheme(std::filesystem::path{path.toStdWString()});
    }
}

void SettingsDialog::chooseThemeFileToExport()
{
    const QString path =
        QFileDialog::getSaveFileName(this, "Export Theme", themeFolder() + "/" + defaultThemeFileName(m_shownTheme),
                                     QString::fromLatin1(kThemeFileFilter));
    if (!path.isEmpty()) {
        exportTheme(std::filesystem::path{path.toStdWString()});
    }
}

void SettingsDialog::fillThemeList()
{
    const QSignalBlocker blocker(m_themeComboBox);
    m_themeComboBox->clear();
    for (const std::string_view name : {domain::kFollowWindowsTheme, domain::kLightTheme, domain::kDarkTheme}) {
        m_themeComboBox->addItem(toQString(name), toQString(name));
    }
    if (!m_themes.empty()) {
        m_themeComboBox->insertSeparator(m_themeComboBox->count());
    }
    for (const domain::NamedTheme& theme : m_themes) {
        const QString name = QString::fromStdString(theme.name);
        m_themeComboBox->addItem(name, name);
    }
    // A theme that is not in the list stays chosen, for the list that
    // setThemeSources() fills.
    const int chosen = m_themeComboBox->findData(QString::fromStdString(m_chosenTheme));
    m_themeComboBox->setCurrentIndex(chosen >= 0 ? chosen : 0);
}

void SettingsDialog::showTheme(const std::string& name)
{
    m_shownTheme = name;
    const domain::Theme theme = themeNamed(name);
    const bool isDark = theme.scheme == domain::ColorScheme::Dark;
    const auto userColors = m_userColors.find(name);
    const domain::ThemeColors chosen = userColors != m_userColors.end() ? userColors->second : domain::ThemeColors{};
    const QString themeName = QString::fromStdString(name);
    for (const ColorRow& entry : kColorRows) {
        ColorButton* button = m_colorButtons[static_cast<std::size_t>(entry.role)];
        const std::optional<domain::RgbColor>& own = theme.colors[entry.role];
        button->setDefaultColor(own ? std::optional{theme::toQColor(*own)} : theme::defaultColor(entry.role, isDark));
        const std::optional<domain::RgbColor>& color = chosen[entry.role];
        button->setColor(color ? std::optional{theme::toQColor(*color)} : std::nullopt);
        button->setDialogTitle(QString("%1 in %2").arg(entry.label, themeName));
    }
    m_colorsGroup->setTitle(QString("Colours of %1").arg(themeName));
    showSampleColors();
}

void SettingsDialog::showSampleColors()
{
    const domain::Theme theme = themeNamed(m_shownTheme);
    const bool isDark = theme.scheme == domain::ColorScheme::Dark;
    SampleColors colors;
    for (std::size_t role = 0; role < domain::kThemeColorRoleCount; ++role) {
        const ColorButton* button = m_colorButtons[role];
        std::optional<QColor> color = button->color() ? button->color() : button->defaultColor();
        // A translucent role keeps its built-in opacity, as theme::color() gives it.
        if (const std::optional<QColor> builtIn =
                theme::defaultColor(static_cast<domain::ThemeColorRole>(role), isDark);
            color && builtIn) {
            color->setAlpha(builtIn->alpha());
        }
        colors[role] = color.value_or(QColor{});
    }

    // The palette's roles that neither the theme nor the user chose take
    // Windows' colours, and the cards the pages'.
    const QPalette windows =
        m_themeSources.windowsPalette ? m_themeSources.windowsPalette(theme.scheme) : QApplication::palette();
    const auto fill = [&colors](domain::ThemeColorRole role, const QColor& color) {
        QColor& shown = colors[static_cast<std::size_t>(role)];
        if (!shown.isValid()) {
            shown = color;
        }
    };
    fill(domain::ThemeColorRole::Window, windows.color(QPalette::Window));
    fill(domain::ThemeColorRole::Page, windows.color(QPalette::Base));
    fill(domain::ThemeColorRole::Text, windows.color(QPalette::WindowText));
    fill(domain::ThemeColorRole::Accent, windows.color(QPalette::Accent));
    // The styles draw a header as the button colour, or as near it as Windows' own parts are.
    fill(domain::ThemeColorRole::TableHeader, windows.color(QPalette::Button));
    fill(domain::ThemeColorRole::Card, colors[static_cast<std::size_t>(domain::ThemeColorRole::Page)]);

    for (ColorSample* sample : m_colorSamples) {
        sample->setColors(colors);
    }
}

void SettingsDialog::keepShownColors()
{
    const domain::ThemeColors colors = shownColors();
    if (colors.empty()) {
        m_userColors.erase(m_shownTheme);
    } else {
        m_userColors[m_shownTheme] = colors;
    }
}

std::string SettingsDialog::themeToShow() const
{
    std::string chosen = m_themeComboBox->currentData().toString().toStdString();
    if (chosen != domain::kFollowWindowsTheme) {
        return chosen;
    }
    domain::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark
                                     ? domain::ColorScheme::Dark
                                     : domain::ColorScheme::Light;
    if (m_themeSources.windowsColorScheme) {
        scheme = m_themeSources.windowsColorScheme();
    }
    return std::string{domain::builtInThemeName(scheme)};
}

domain::Theme SettingsDialog::themeNamed(std::string_view name) const
{
    if (name == domain::kDarkTheme) {
        return domain::Theme{.scheme = domain::ColorScheme::Dark};
    }
    const auto theme = std::ranges::find(m_themes, name, &domain::NamedTheme::name);
    return theme != m_themes.end() ? theme->theme : domain::Theme{};
}

domain::ThemeColors SettingsDialog::shownColors() const
{
    domain::ThemeColors shown;
    for (std::size_t role = 0; role < domain::kThemeColorRoleCount; ++role) {
        if (const std::optional<QColor>& chosen = m_colorButtons[role]->color()) {
            shown.colors[role] = theme::toRgbColor(*chosen);
        }
    }
    return shown;
}

domain::Settings SettingsDialog::settings() const
{
    std::map<std::string, domain::ThemeColors, std::less<>> themeColors = m_userColors;
    if (const domain::ThemeColors shown = shownColors(); shown.empty()) {
        themeColors.erase(m_shownTheme);
    } else {
        themeColors[m_shownTheme] = shown;
    }
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
        .theme = m_themeComboBox->currentData().toString().toStdString(),
        .themeColors = std::move(themeColors),
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

QComboBox* SettingsDialog::themeComboBox() const
{
    return m_themeComboBox;
}

ColorButton* SettingsDialog::colorButton(domain::ThemeColorRole role) const
{
    return m_colorButtons[static_cast<std::size_t>(role)];
}

ColorSample* SettingsDialog::colorSample(domain::ThemeColorRole role) const
{
    return m_colorSamples[static_cast<std::size_t>(role)];
}

QGroupBox* SettingsDialog::colorsGroup() const
{
    return m_colorsGroup;
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
