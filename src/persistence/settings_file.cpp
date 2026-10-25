#include "persistence/settings_file.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <limits>
#include <optional>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

namespace verdandi::persistence
{

namespace
{

using nlohmann::json;
using nlohmann::ordered_json;

constexpr std::string_view kVersionKey = "version";
constexpr std::string_view kRefreshIntervalKey = "refreshIntervalMs";
constexpr std::string_view kDisplayUnitsKey = "displayUnits";
constexpr std::string_view kBytesKey = "bytes";
constexpr std::string_view kNetworkRatesKey = "networkRates";
constexpr std::string_view kDashboardPanelsKey = "dashboardPanels";
constexpr std::string_view kColorSchemeKey = "colorScheme";
constexpr std::string_view kThemeKey = "theme";
constexpr std::string_view kLightColorsKey = "light";
constexpr std::string_view kDarkColorsKey = "dark";
constexpr std::string_view kLogLevelKey = "logLevel";

// Appended to the settings file's name for the file that a save writes first.
constexpr std::string_view kTemporaryFileSuffix = ".tmp";

// The name that the settings file gives a value of an enumeration.
template <typename Enum> struct NamedValue
{
    std::string_view name{};
    Enum value{};
};

constexpr std::array kByteUnitsNames{
    NamedValue<domain::ByteUnits>{"binary", domain::ByteUnits::Binary},
    NamedValue<domain::ByteUnits>{"decimal", domain::ByteUnits::Decimal},
};

constexpr std::array kNetworkRateUnitsNames{
    NamedValue<domain::NetworkRateUnits>{"bytesPerSecond", domain::NetworkRateUnits::BytesPerSecond},
    NamedValue<domain::NetworkRateUnits>{"bitsPerSecond", domain::NetworkRateUnits::BitsPerSecond},
};

constexpr std::array kColorSchemeNames{
    NamedValue<domain::ColorScheme>{"system", domain::ColorScheme::System},
    NamedValue<domain::ColorScheme>{"light", domain::ColorScheme::Light},
    NamedValue<domain::ColorScheme>{"dark", domain::ColorScheme::Dark},
};

// A theme colour's key in the colours of a scheme, which name the role.
constexpr std::array kThemeColorRoleNames{
    NamedValue<domain::ThemeColorRole>{"window", domain::ThemeColorRole::Window},
    NamedValue<domain::ThemeColorRole>{"page", domain::ThemeColorRole::Page},
    NamedValue<domain::ThemeColorRole>{"card", domain::ThemeColorRole::Card},
    NamedValue<domain::ThemeColorRole>{"text", domain::ThemeColorRole::Text},
    NamedValue<domain::ThemeColorRole>{"accent", domain::ThemeColorRole::Accent},
    NamedValue<domain::ThemeColorRole>{"chartLine1", domain::ThemeColorRole::ChartLine1},
    NamedValue<domain::ThemeColorRole>{"chartLine2", domain::ThemeColorRole::ChartLine2},
    NamedValue<domain::ThemeColorRole>{"chartLine3", domain::ThemeColorRole::ChartLine3},
    NamedValue<domain::ThemeColorRole>{"chartLine4", domain::ThemeColorRole::ChartLine4},
    NamedValue<domain::ThemeColorRole>{"chartLine5", domain::ThemeColorRole::ChartLine5},
    NamedValue<domain::ThemeColorRole>{"chartGrid", domain::ThemeColorRole::ChartGrid},
    NamedValue<domain::ThemeColorRole>{"good", domain::ThemeColorRole::Good},
    NamedValue<domain::ThemeColorRole>{"warning", domain::ThemeColorRole::Warning},
    NamedValue<domain::ThemeColorRole>{"critical", domain::ThemeColorRole::Critical},
    NamedValue<domain::ThemeColorRole>{"processStarted", domain::ThemeColorRole::ProcessStarted},
    NamedValue<domain::ThemeColorRole>{"processExited", domain::ThemeColorRole::ProcessExited},
    NamedValue<domain::ThemeColorRole>{"accessDenied", domain::ThemeColorRole::AccessDenied},
    NamedValue<domain::ThemeColorRole>{"accessNeverOpened", domain::ThemeColorRole::AccessNeverOpened},
    NamedValue<domain::ThemeColorRole>{"accessUnavailable", domain::ThemeColorRole::AccessUnavailable},
};
static_assert(kThemeColorRoleNames.size() == domain::kThemeColorRoleCount, "every theme colour role has a name");

// A colour as the settings file writes it: "#" and two lowercase hexadecimal digits per channel.
constexpr std::size_t kColorTextLength = 7;
constexpr int kHexadecimal = 16;

// The levels' names, which the --log-level option gives them too.
constexpr auto kLogLevelNames = [] {
    std::array<NamedValue<domain::LogLevel>, domain::kLogLevelNames.size()> names{};
    std::ranges::transform(domain::kLogLevelNames, names.begin(), [](const domain::LogLevelName& entry) {
        return NamedValue<domain::LogLevel>{entry.name, entry.level};
    });
    return names;
}();

// A dashboard panel's key in the settings file, and its member of DashboardPanels.
struct PanelKey
{
    std::string_view key{};
    bool domain::DashboardPanels::* isShown{};
};

constexpr std::array kPanelKeys{
    PanelKey{"cpu", &domain::DashboardPanels::cpu},       PanelKey{"memory", &domain::DashboardPanels::memory},
    PanelKey{"disk", &domain::DashboardPanels::disk},     PanelKey{"network", &domain::DashboardPanels::network},
    PanelKey{"uptime", &domain::DashboardPanels::uptime},
};

template <typename Enum, std::size_t Count>
std::string_view nameOf(Enum value, const std::array<NamedValue<Enum>, Count>& names)
{
    const auto entry = std::ranges::find(names, value, &NamedValue<Enum>::value);
    assert(entry != names.end() && "every value of the enumeration has a name");
    return entry != names.end() ? entry->name : std::string_view{};
}

template <typename Enum, std::size_t Count> std::string joinedNames(const std::array<NamedValue<Enum>, Count>& names)
{
    std::string joined;
    for (const auto& entry : names) {
        if (!joined.empty()) {
            joined += ", ";
        }
        joined += entry.name;
    }
    return joined;
}

// Describes a value in a problem: a string in quotes, a number or a literal as
// written, and an array or an object by its type.
std::string describe(const json& value)
{
    if (value.is_array() || value.is_object()) {
        return std::format("an {}", value.type_name());
    }
    return value.dump(-1, ' ', false, json::error_handler_t::replace);
}

void report(std::vector<SettingsProblem>& problems, std::string_view key, std::string description)
{
    problems.push_back(SettingsProblem{.key = std::string{key}, .description = std::move(description)});
}

void reportUnknownKey(std::vector<SettingsProblem>& problems, std::string_view key)
{
    report(problems, key, std::format("{} is not a setting; ignoring it", key));
}

// Sets target to the value that value names, or reports value and leaves target as it is.
template <typename Enum, std::size_t Count>
void readName(const json& value, std::string_view key, const std::array<NamedValue<Enum>, Count>& names, Enum& target,
              std::vector<SettingsProblem>& problems)
{
    if (value.is_string()) {
        const std::string_view name = value.get_ref<const std::string&>();
        const auto entry = std::ranges::find(names, name, &NamedValue<Enum>::name);
        if (entry != names.end()) {
            target = entry->value;
            return;
        }
    }
    report(problems, key,
           std::format("{} is {}, not one of {}; using {}", key, describe(value), joinedNames(names),
                       nameOf(target, names)));
}

// Calls readMember(path, key, member) for each member of value, where path is
// the member's key under key, or reports that value is not an object, which
// leaves every setting it holds at its default.
template <typename ReadMember>
void readMembers(const json& value, std::string_view key, std::vector<SettingsProblem>& problems, ReadMember readMember)
{
    if (!value.is_object()) {
        report(problems, key, std::format("{} is {}, not an object; using its defaults", key, describe(value)));
        return;
    }
    for (const auto& [memberKey, member] : value.items()) {
        readMember(std::format("{}.{}", key, memberKey), memberKey, member);
    }
}

void readVersion(const json& value, std::vector<SettingsProblem>& problems)
{
    if (value.is_number_integer() && value == kSettingsVersion) {
        return;
    }
    report(problems, kVersionKey,
           std::format("{} is {}, but this monitor writes {}; reading the settings it knows", kVersionKey,
                       describe(value), kSettingsVersion));
}

void readRefreshInterval(const json& value, std::chrono::milliseconds& interval, std::vector<SettingsProblem>& problems)
{
    if (!value.is_number_integer()) {
        report(problems, kRefreshIntervalKey,
               std::format("{} is {}, not a whole number of milliseconds; using {}", kRefreshIntervalKey,
                           describe(value), interval.count()));
        return;
    }

    // nlohmann/json keeps a whole number that is not negative as unsigned. One
    // beyond the signed range is above the maximum either way.
    constexpr auto kLargest = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    const std::int64_t requested = value.is_number_unsigned()
                                       ? static_cast<std::int64_t>(std::min(value.get<std::uint64_t>(), kLargest))
                                       : value.get<std::int64_t>();
    const std::int64_t minimum = domain::kMinRefreshInterval.count();
    const std::int64_t maximum = domain::kMaxRefreshInterval.count();
    const std::chrono::milliseconds used = domain::nearestRefreshInterval(std::chrono::milliseconds{requested});
    if (requested < minimum || requested > maximum) {
        report(problems, kRefreshIntervalKey,
               std::format("{} is {}, outside {} to {}; using {}", kRefreshIntervalKey, describe(value), minimum,
                           maximum, used.count()));
    } else if (used.count() != requested) {
        report(problems, kRefreshIntervalKey,
               std::format("{} is {}, not a multiple of {}; using {}", kRefreshIntervalKey, describe(value),
                           domain::kRefreshIntervalStep.count(), used.count()));
    }
    interval = used;
}

void readDisplayUnits(const json& value, domain::DisplayUnits& units, std::vector<SettingsProblem>& problems)
{
    readMembers(value, kDisplayUnitsKey, problems,
                [&units, &problems](const std::string& path, const std::string& key, const json& member) {
                    if (key == kBytesKey) {
                        readName(member, path, kByteUnitsNames, units.bytes, problems);
                    } else if (key == kNetworkRatesKey) {
                        readName(member, path, kNetworkRateUnitsNames, units.networkRates, problems);
                    } else {
                        reportUnknownKey(problems, path);
                    }
                });
}

void readDashboardPanels(const json& value, domain::DashboardPanels& panels, std::vector<SettingsProblem>& problems)
{
    readMembers(value, kDashboardPanelsKey, problems,
                [&panels, &problems](const std::string& path, const std::string& key, const json& member) {
                    const auto panel = std::ranges::find(kPanelKeys, std::string_view{key}, &PanelKey::key);
                    if (panel == kPanelKeys.end()) {
                        reportUnknownKey(problems, path);
                    } else if (!member.is_boolean()) {
                        report(problems, path,
                               std::format("{} is {}, not true or false; using {}", path, describe(member),
                                           panels.*(panel->isShown)));
                    } else {
                        panels.*(panel->isShown) = member.get<bool>();
                    }
                });
}

std::string colorText(domain::RgbColor color)
{
    return std::format("#{:02x}{:02x}{:02x}", color.red, color.green, color.blue);
}

// Reads a colour written as "#rrggbb", in either case, or returns std::nullopt.
std::optional<domain::RgbColor> parseColor(std::string_view text)
{
    if (text.size() != kColorTextLength || text.front() != '#') {
        return std::nullopt;
    }
    std::array<std::uint8_t, 3> channels{};
    for (std::size_t channel = 0; channel < channels.size(); ++channel) {
        const char* first = text.data() + 1 + 2 * channel;
        const char* last = first + 2;
        const auto [end, error] = std::from_chars(first, last, channels[channel], kHexadecimal);
        if (error != std::errc{} || end != last) {
            return std::nullopt;
        }
    }
    return domain::RgbColor{.red = channels[0], .green = channels[1], .blue = channels[2]};
}

ordered_json themeColorsDocument(const domain::ThemeColors& colors)
{
    ordered_json document = ordered_json::object();
    for (const auto& [name, role] : kThemeColorRoleNames) {
        if (const auto& color = colors[role]) {
            document[name] = colorText(*color);
        }
    }
    return document;
}

ordered_json themeDocument(const domain::Theme& theme)
{
    ordered_json document = ordered_json::object();
    document[kLightColorsKey] = themeColorsDocument(theme.light);
    document[kDarkColorsKey] = themeColorsDocument(theme.dark);
    return document;
}

// Reads the colours chosen for one scheme, at key, such as "theme.dark".
void readThemeColors(const json& value, std::string_view key, domain::ThemeColors& colors,
                     std::vector<SettingsProblem>& problems)
{
    readMembers(value, key, problems,
                [&colors, &problems](const std::string& path, const std::string& roleKey, const json& member) {
                    const auto entry = std::ranges::find(kThemeColorRoleNames, std::string_view{roleKey},
                                                         &NamedValue<domain::ThemeColorRole>::name);
                    if (entry == kThemeColorRoleNames.end()) {
                        reportUnknownKey(problems, path);
                        return;
                    }
                    const std::optional<domain::RgbColor> color =
                        member.is_string() ? parseColor(member.get_ref<const std::string&>()) : std::nullopt;
                    if (!color) {
                        report(problems, path,
                               std::format("{} is {}, not a colour written as #rrggbb; using the default colour", path,
                                           describe(member)));
                        return;
                    }
                    colors[entry->value] = *color;
                });
}

void readTheme(const json& value, domain::Theme& theme, std::vector<SettingsProblem>& problems)
{
    readMembers(value, kThemeKey, problems,
                [&theme, &problems](const std::string& path, const std::string& key, const json& member) {
                    if (key == kLightColorsKey) {
                        readThemeColors(member, path, theme.light, problems);
                    } else if (key == kDarkColorsKey) {
                        readThemeColors(member, path, theme.dark, problems);
                    } else {
                        reportUnknownKey(problems, path);
                    }
                });
}

// A SAX handler that accepts every value and keeps the parser's message for
// the first syntax error, which says where it is. json::parse() without
// exceptions says only that the text is not valid, so parseSettings() runs
// this on the text that json::parse() rejected.
class SyntaxErrorRecorder final : public json::json_sax_t
{
public:
    bool null() override
    {
        return true;
    }

    bool boolean(bool) override
    {
        return true;
    }

    bool number_integer(number_integer_t) override
    {
        return true;
    }

    bool number_unsigned(number_unsigned_t) override
    {
        return true;
    }

    bool number_float(number_float_t, const string_t&) override
    {
        return true;
    }

    bool string(string_t&) override
    {
        return true;
    }

    bool binary(binary_t&) override
    {
        return true;
    }

    bool start_object(std::size_t) override
    {
        return true;
    }

    bool key(string_t&) override
    {
        return true;
    }

    bool end_object() override
    {
        return true;
    }

    bool start_array(std::size_t) override
    {
        return true;
    }

    bool end_array() override
    {
        return true;
    }

    bool parse_error(std::size_t, const std::string&, const json::exception& error) override
    {
        // Without its ID, such as "[json.exception.parse_error.101] ", the
        // message begins "parse error at line 3, column 1: ".
        const std::string_view message = error.what();
        const auto idEnd = message.starts_with('[') ? message.find("] ") : std::string_view::npos;
        m_message = idEnd == std::string_view::npos ? message : message.substr(idEnd + 2);
        return false;
    }

    [[nodiscard]] const std::string& message() const
    {
        return m_message;
    }

private:
    std::string m_message;
};

std::string syntaxError(std::string_view text)
{
    SyntaxErrorRecorder recorder;
    static_cast<void>(json::sax_parse(text, &recorder));
    return recorder.message();
}

SettingsReadResult defaultsWithProblem(std::string description)
{
    SettingsReadResult result;
    report(result.problems, {}, std::move(description));
    return result;
}

// Returns the text of the file at path, or std::nullopt if it cannot be opened.
std::optional<std::string> readText(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

// Writes text to the file at path, creating its directory, through a
// temporary file beside it that then replaces the file. Returns std::nullopt
// when the file was written, and otherwise why it was not.
std::optional<std::string> writeThroughTemporaryFile(const std::filesystem::path& path, const std::string& text)
{
    std::error_code error;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            return std::format("Cannot create the file's directory: {}", error.message());
        }
    }

    std::filesystem::path temporaryPath = path;
    temporaryPath += kTemporaryFileSuffix;
    std::ofstream file(temporaryPath, std::ios::binary | std::ios::trunc);
    if (!file) {
        return std::string{"Cannot create the temporary file beside it"};
    }
    file << text;
    file.close();
    if (file.fail()) {
        std::filesystem::remove(temporaryPath, error);
        return std::string{"Cannot write the temporary file beside it"};
    }

    // MSVC's std::filesystem::rename() calls MoveFileExW with
    // MOVEFILE_REPLACE_EXISTING, which replaces the file in one step.
    std::filesystem::rename(temporaryPath, path, error);
    if (error) {
        std::string reason = error.message();
        std::filesystem::remove(temporaryPath, error);
        return std::format("Cannot replace the file: {}", reason);
    }
    return std::nullopt;
}

} // namespace

std::string serializeSettings(const domain::Settings& settings)
{
    ordered_json displayUnits = ordered_json::object();
    displayUnits[kBytesKey] = nameOf(settings.displayUnits.bytes, kByteUnitsNames);
    displayUnits[kNetworkRatesKey] = nameOf(settings.displayUnits.networkRates, kNetworkRateUnitsNames);

    ordered_json dashboardPanels = ordered_json::object();
    for (const auto& [key, isShown] : kPanelKeys) {
        dashboardPanels[key] = settings.dashboardPanels.*isShown;
    }

    ordered_json document = ordered_json::object();
    document[kVersionKey] = kSettingsVersion;
    document[kRefreshIntervalKey] = settings.refreshInterval.count();
    document[kDisplayUnitsKey] = std::move(displayUnits);
    document[kDashboardPanelsKey] = std::move(dashboardPanels);
    document[kColorSchemeKey] = nameOf(settings.colorScheme, kColorSchemeNames);
    document[kThemeKey] = themeDocument(settings.theme);
    document[kLogLevelKey] = nameOf(settings.logLevel, kLogLevelNames);
    return document.dump(4) + "\n";
}

SettingsReadResult parseSettings(std::string_view text)
{
    const json document = json::parse(text, nullptr, false);
    if (document.is_discarded()) {
        return defaultsWithProblem(std::format("Not valid JSON ({}); using the default settings", syntaxError(text)));
    }
    if (!document.is_object()) {
        return defaultsWithProblem(
            std::format("Not a JSON object but {}; using the default settings", describe(document)));
    }

    SettingsReadResult result;
    domain::Settings& settings = result.settings;
    for (const auto& [key, value] : document.items()) {
        if (key == kVersionKey) {
            readVersion(value, result.problems);
        } else if (key == kRefreshIntervalKey) {
            readRefreshInterval(value, settings.refreshInterval, result.problems);
        } else if (key == kDisplayUnitsKey) {
            readDisplayUnits(value, settings.displayUnits, result.problems);
        } else if (key == kDashboardPanelsKey) {
            readDashboardPanels(value, settings.dashboardPanels, result.problems);
        } else if (key == kColorSchemeKey) {
            readName(value, kColorSchemeKey, kColorSchemeNames, settings.colorScheme, result.problems);
        } else if (key == kThemeKey) {
            readTheme(value, settings.theme, result.problems);
        } else if (key == kLogLevelKey) {
            readName(value, kLogLevelKey, kLogLevelNames, settings.logLevel, result.problems);
        } else {
            reportUnknownKey(result.problems, key);
        }
    }
    return result;
}

std::optional<SettingsReadResult> loadSettingsFile(const std::filesystem::path& path)
{
    const std::optional<std::string> text = readText(path);
    if (!text) {
        std::error_code error;
        if (!std::filesystem::exists(path, error) && !error) {
            return std::nullopt;
        }
        return defaultsWithProblem("The file cannot be opened; using the default settings");
    }
    return parseSettings(*text);
}

std::optional<std::string> saveSettingsFile(const std::filesystem::path& path, const domain::Settings& settings)
{
    return writeThroughTemporaryFile(path, serializeSettings(settings));
}

std::string serializeTheme(const domain::Theme& theme)
{
    ordered_json document = ordered_json::object();
    document[kVersionKey] = kSettingsVersion;
    document[kThemeKey] = themeDocument(theme);
    return document.dump(4) + "\n";
}

ThemeReadResult parseTheme(std::string_view text)
{
    ThemeReadResult result;
    const json document = json::parse(text, nullptr, false);
    if (document.is_discarded()) {
        report(result.problems, {}, std::format("Not valid JSON ({}); no colours read", syntaxError(text)));
        return result;
    }
    if (!document.is_object()) {
        report(result.problems, {}, std::format("Not a JSON object but {}; no colours read", describe(document)));
        return result;
    }
    const auto theme = document.find(kThemeKey);
    if (theme == document.end()) {
        report(result.problems, {}, "The file holds no theme; no colours read");
        return result;
    }
    if (!theme->is_object()) {
        report(result.problems, kThemeKey,
               std::format("{} is {}, not an object; no colours read", kThemeKey, describe(*theme)));
        return result;
    }

    // The other settings of a settings file are not the theme's, so they
    // are neither read nor reported.
    if (const auto version = document.find(kVersionKey); version != document.end()) {
        readVersion(*version, result.problems);
    }
    result.theme.emplace();
    readTheme(*theme, *result.theme, result.problems);
    return result;
}

ThemeReadResult loadThemeFile(const std::filesystem::path& path)
{
    const std::optional<std::string> text = readText(path);
    if (!text) {
        ThemeReadResult result;
        report(result.problems, {}, "The file cannot be opened; no colours read");
        return result;
    }
    return parseTheme(*text);
}

std::optional<std::string> saveThemeFile(const std::filesystem::path& path, const domain::Theme& theme)
{
    return writeThroughTemporaryFile(path, serializeTheme(theme));
}

} // namespace verdandi::persistence
