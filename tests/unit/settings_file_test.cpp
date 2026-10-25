#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "domain/settings.h"
#include "domain/theme.h"
#include "persistence/settings_file.h"
#include "support/settings_samples.h"

using namespace verdandi::domain;
using namespace verdandi::persistence;
using std::chrono::milliseconds;
using verdandi::tests::nonDefaultSettings;

namespace
{

using Keys = std::vector<std::string>;

// The keys of the problems found, sorted.
template <typename ReadResult> Keys problemKeys(const ReadResult& result)
{
    Keys keys;
    for (const auto& problem : result.problems) {
        keys.push_back(problem.key);
    }
    std::ranges::sort(keys);
    return keys;
}

// A theme of the dark scheme that chooses only its card colour, #202020.
Theme themeWithCard()
{
    Theme theme{.scheme = ColorScheme::Dark};
    theme.colors[ThemeColorRole::Card] = RgbColor{.red = 0x20, .green = 0x20, .blue = 0x20};
    return theme;
}

// A settings file that names one value of an enumeration, and the settings it holds.
struct NamedValueCase
{
    const char* name{};
    const char* text{};
    Settings expected{};
};

// Every value of every enumeration in the settings file, and one log level:
// the file gives the levels the names that Settings.LogLevelNames_* checks.
std::vector<NamedValueCase> namedValueCases()
{
    return {
        {"binary bytes", R"({"displayUnits": {"bytes": "binary"}})",
         Settings{.displayUnits = {.bytes = ByteUnits::Binary}}},
        {"decimal bytes", R"({"displayUnits": {"bytes": "decimal"}})",
         Settings{.displayUnits = {.bytes = ByteUnits::Decimal}}},
        {"network rates in bytes", R"({"displayUnits": {"networkRates": "bytesPerSecond"}})",
         Settings{.displayUnits = {.networkRates = NetworkRateUnits::BytesPerSecond}}},
        {"network rates in bits", R"({"displayUnits": {"networkRates": "bitsPerSecond"}})",
         Settings{.displayUnits = {.networkRates = NetworkRateUnits::BitsPerSecond}}},
        {"the Follow Windows theme", R"({"theme": "Follow Windows"})", Settings{.theme = "Follow Windows"}},
        {"the Light theme", R"({"theme": "Light"})", Settings{.theme = "Light"}},
        {"the Dark theme", R"({"theme": "Dark"})", Settings{.theme = "Dark"}},
        {"a theme of the themes folder", R"({"theme": "Verdandi"})", Settings{.theme = "Verdandi"}},
        {"a log level", R"({"logLevel": "critical"})", Settings{.logLevel = LogLevel::Critical}},
    };
}

} // namespace

// ---------------------------------------------------------------------------
// serializeSettings
// ---------------------------------------------------------------------------

TEST(SettingsFile, SerializeSettings_Defaults_WritesTheFileFormat)
{
    // The key names, the value names and the defaults that a settings file holds.
    const std::string expected = R"({
    "version": 1,
    "refreshIntervalMs": 1000,
    "displayUnits": {
        "bytes": "binary",
        "networkRates": "bytesPerSecond"
    },
    "dashboardPanels": {
        "cpu": true,
        "memory": true,
        "disk": true,
        "network": true,
        "uptime": true
    },
    "theme": "Follow Windows",
    "themeColors": {},
    "logLevel": "info"
}
)";

    EXPECT_EQ(serializeSettings(Settings{}), expected);
}

TEST(SettingsFile, SerializeSettings_ChosenColours_UnderTheirThemesInLowercaseInRoleOrder)
{
    Settings settings;
    settings.themeColors["Light"][ThemeColorRole::Page] = RgbColor{.red = 0xf0, .green = 0xf0, .blue = 0xf0};
    settings.themeColors["Verdandi"][ThemeColorRole::AccessDenied] = RgbColor{.red = 0xab, .green = 0xcd, .blue = 0xef};
    settings.themeColors["Verdandi"][ThemeColorRole::Card] = RgbColor{.red = 0x20, .green = 0x20, .blue = 0x20};
    // A theme whose colours the user left alone is left out.
    settings.themeColors["Dark"] = ThemeColors{};
    const std::string expected = R"(    "themeColors": {
        "Light": {
            "page": "#f0f0f0"
        },
        "Verdandi": {
            "card": "#202020",
            "accessDenied": "#abcdef"
        }
    },
)";

    const std::string text = serializeSettings(settings);

    EXPECT_NE(text.find(expected), std::string::npos) << text;
}

TEST(SettingsFile, SerializeThenParse_EveryValue_ReadsBackWithoutProblems)
{
    std::vector<NamedValueCase> cases = namedValueCases();
    cases.push_back({"every setting changed", "", nonDefaultSettings()});

    for (const auto& [name, text, expected] : cases) {
        SCOPED_TRACE(name);

        const SettingsReadResult result = parseSettings(serializeSettings(expected));

        EXPECT_EQ(result.settings, expected);
        EXPECT_TRUE(result.problems.empty());
    }
}

// ---------------------------------------------------------------------------
// parseSettings
// ---------------------------------------------------------------------------

TEST(SettingsFile, ParseSettings_ValueNames_ReadAsTheirValues)
{
    for (const auto& [name, text, expected] : namedValueCases()) {
        SCOPED_TRACE(name);

        const SettingsReadResult result = parseSettings(text);

        EXPECT_EQ(result.settings, expected);
        EXPECT_TRUE(result.problems.empty());
    }
}

TEST(SettingsFile, ParseSettings_ThemeColorRoleNames_ReadAsTheirRoles)
{
    struct Case
    {
        const char* name{};
        ThemeColorRole role{};
    };
    const std::array cases{
        Case{"window", ThemeColorRole::Window},
        Case{"page", ThemeColorRole::Page},
        Case{"card", ThemeColorRole::Card},
        Case{"text", ThemeColorRole::Text},
        Case{"accent", ThemeColorRole::Accent},
        Case{"chartLine1", ThemeColorRole::ChartLine1},
        Case{"chartLine2", ThemeColorRole::ChartLine2},
        Case{"chartLine3", ThemeColorRole::ChartLine3},
        Case{"chartLine4", ThemeColorRole::ChartLine4},
        Case{"chartLine5", ThemeColorRole::ChartLine5},
        Case{"chartGrid", ThemeColorRole::ChartGrid},
        Case{"good", ThemeColorRole::Good},
        Case{"warning", ThemeColorRole::Warning},
        Case{"critical", ThemeColorRole::Critical},
        Case{"processStarted", ThemeColorRole::ProcessStarted},
        Case{"processExited", ThemeColorRole::ProcessExited},
        Case{"accessDenied", ThemeColorRole::AccessDenied},
        Case{"accessNeverOpened", ThemeColorRole::AccessNeverOpened},
        Case{"accessUnavailable", ThemeColorRole::AccessUnavailable},
    };
    static_assert(cases.size() == kThemeColorRoleCount);

    for (const auto& [name, role] : cases) {
        SCOPED_TRACE(name);
        Settings expected;
        expected.themeColors["Dark"][role] = RgbColor{.red = 0x01, .green = 0x02, .blue = 0x03};

        const SettingsReadResult result =
            parseSettings(std::format(R"({{"themeColors": {{"Dark": {{"{}": "#010203"}}}}}})", name));
        const ThemeReadResult theme =
            parseTheme(std::format(R"({{"scheme": "dark", "colors": {{"{}": "#010203"}}}})", name));

        EXPECT_EQ(result.settings, expected);
        EXPECT_TRUE(result.problems.empty());
        EXPECT_EQ(theme.theme, (Theme{.scheme = ColorScheme::Dark, .colors = expected.themeColors["Dark"]}));
        EXPECT_TRUE(theme.problems.empty());
    }
}

TEST(SettingsFile, ParseSettings_ThemeColours_EitherCaseInAnyTheme)
{
    Settings expected;
    expected.themeColors["Light"][ThemeColorRole::ChartLine1] = RgbColor{.red = 0xab, .green = 0xcd, .blue = 0xef};
    expected.themeColors["Verdandi"][ThemeColorRole::Text] = RgbColor{.red = 0xab, .green = 0xcd, .blue = 0xef};

    // A theme whose colours the user left alone has no entry.
    const SettingsReadResult result = parseSettings(
        R"({"themeColors": {"Light": {"chartLine1": "#abcdef"}, "Verdandi": {"text": "#ABcDeF"}, "Dark": {}}})");

    EXPECT_EQ(result.settings, expected);
    EXPECT_TRUE(result.problems.empty());
}

TEST(SettingsFile, ParseSettings_MissingSettings_KeepTheirDefaults)
{
    // A panel that a later version adds is missing from an older file, and is shown.
    const SettingsReadResult empty = parseSettings("{}");
    const SettingsReadResult partial = parseSettings(R"({"dashboardPanels": {"uptime": false}})");

    EXPECT_EQ(empty.settings, Settings{});
    EXPECT_TRUE(empty.problems.empty());
    EXPECT_EQ(partial.settings, Settings{.dashboardPanels = {.uptime = false}});
    EXPECT_TRUE(partial.problems.empty());
}

TEST(SettingsFile, ParseSettings_RefreshInterval_TakesTheNearestThatCanBeChosen)
{
    struct Case
    {
        const char* name;
        const char* text;
        milliseconds expected;
        Keys problemKeys;
    };
    const std::vector<Case> cases{
        {"minimum", R"({"refreshIntervalMs": 500})", milliseconds{500}, {}},
        {"between", R"({"refreshIntervalMs": 2500})", milliseconds{2500}, {}},
        {"maximum", R"({"refreshIntervalMs": 10000})", milliseconds{10'000}, {}},
        {"below the minimum", R"({"refreshIntervalMs": 499})", milliseconds{500}, {"refreshIntervalMs"}},
        {"zero", R"({"refreshIntervalMs": 0})", milliseconds{500}, {"refreshIntervalMs"}},
        {"negative", R"({"refreshIntervalMs": -1000})", milliseconds{500}, {"refreshIntervalMs"}},
        {"above the maximum", R"({"refreshIntervalMs": 10001})", milliseconds{10'000}, {"refreshIntervalMs"}},
        {"between two steps", R"({"refreshIntervalMs": 1234})", milliseconds{1000}, {"refreshIntervalMs"}},
        {"beyond 64-bit signed",
         R"({"refreshIntervalMs": 18446744073709551615})",
         milliseconds{10'000},
         {"refreshIntervalMs"}},
    };

    for (const auto& [name, text, expected, keys] : cases) {
        SCOPED_TRACE(name);

        const SettingsReadResult result = parseSettings(text);

        EXPECT_EQ(result.settings.refreshInterval, expected);
        EXPECT_EQ(problemKeys(result), keys);
    }
    const SettingsReadResult betweenSteps = parseSettings(R"({"refreshIntervalMs": 1234})");
    ASSERT_EQ(betweenSteps.problems.size(), 1u);
    EXPECT_EQ(betweenSteps.problems.front().description,
              "refreshIntervalMs is 1234, not a multiple of 500; using 1000");
}

TEST(SettingsFile, ParseSettings_InvalidValue_KeepsItsDefaultAndIsReported)
{
    struct Case
    {
        const char* name;
        const char* text;
        const char* problemKey;
    };
    const std::vector<Case> cases{
        {"interval as text", R"({"refreshIntervalMs": "2000"})", "refreshIntervalMs"},
        {"interval with a fraction", R"({"refreshIntervalMs": 2000.5})", "refreshIntervalMs"},
        {"interval null", R"({"refreshIntervalMs": null})", "refreshIntervalMs"},
        {"display units not an object", R"({"displayUnits": "decimal"})", "displayUnits"},
        {"unknown byte units", R"({"displayUnits": {"bytes": "metric"}})", "displayUnits.bytes"},
        {"byte units in another case", R"({"displayUnits": {"bytes": "Decimal"}})", "displayUnits.bytes"},
        {"network rate units as a number", R"({"displayUnits": {"networkRates": 8}})", "displayUnits.networkRates"},
        {"panels as a list", R"({"dashboardPanels": ["cpu"]})", "dashboardPanels"},
        {"panel as a number", R"({"dashboardPanels": {"cpu": 0}})", "dashboardPanels.cpu"},
        {"panel as text", R"({"dashboardPanels": {"cpu": "false"}})", "dashboardPanels.cpu"},
        {"theme as an object", R"({"theme": {"name": "Dark"}})", "theme"},
        {"theme as a boolean", R"({"theme": true})", "theme"},
        {"theme without a name", R"({"theme": ""})", "theme"},
        {"themes' colours not an object", R"({"themeColors": "Dark"})", "themeColors"},
        {"theme's colours not an object", R"({"themeColors": {"Dark": ["#000000"]}})", "themeColors.Dark"},
        {"colour as a number", R"({"themeColors": {"Dark": {"card": 2105376}}})", "themeColors.Dark.card"},
        {"colour by name", R"({"themeColors": {"Dark": {"card": "red"}}})", "themeColors.Dark.card"},
        {"colour without #", R"({"themeColors": {"Dark": {"card": "202020"}}})", "themeColors.Dark.card"},
        {"colour of three digits", R"({"themeColors": {"Dark": {"card": "#222"}}})", "themeColors.Dark.card"},
        {"colour with opacity", R"({"themeColors": {"Dark": {"card": "#20202080"}}})", "themeColors.Dark.card"},
        {"colour with a non-hexadecimal digit", R"({"themeColors": {"Dark": {"card": "#20202g"}}})",
         "themeColors.Dark.card"},
        {"colour with a sign", R"({"themeColors": {"Dark": {"card": "#+20202"}}})", "themeColors.Dark.card"},
        {"colour null", R"({"themeColors": {"Light": {"page": null}}})", "themeColors.Light.page"},
        {"unknown log level", R"({"logLevel": "verbose"})", "logLevel"},
        {"log level in another case", R"({"logLevel": "Debug"})", "logLevel"},
        {"log level as a number", R"({"logLevel": 1})", "logLevel"},
    };

    for (const auto& [name, text, problemKey] : cases) {
        SCOPED_TRACE(name);

        const SettingsReadResult result = parseSettings(text);

        EXPECT_EQ(result.settings, Settings{});
        EXPECT_EQ(problemKeys(result), Keys{problemKey});
    }
}

TEST(SettingsFile, ParseSettings_InvalidValues_OtherSettingsStillRead)
{
    const SettingsReadResult result = parseSettings(R"({
        "refreshIntervalMs": "fast",
        "displayUnits": {"bytes": "decimal", "networkRates": "baud"},
        "dashboardPanels": {"cpu": false, "disk": "no"},
        "logLevel": "warn"
    })");

    EXPECT_EQ(result.settings, (Settings{
                                   .displayUnits = {.bytes = ByteUnits::Decimal},
                                   .dashboardPanels = {.cpu = false},
                                   .logLevel = LogLevel::Warn,
                               }));
    EXPECT_EQ(problemKeys(result), (Keys{"dashboardPanels.disk", "displayUnits.networkRates", "refreshIntervalMs"}));
}

TEST(SettingsFile, ParseSettings_UnknownKeys_IgnoredAndReported)
{
    const SettingsReadResult result = parseSettings(R"({
        "refreshIntervalMs": 2000,
        "fontSize": 12,
        "displayUnits": {"temperature": "celsius"},
        "dashboardPanels": {"gpu": true},
        "themeColors": {"Light": {"background": "#000000"}}
    })");

    EXPECT_EQ(result.settings, Settings{.refreshInterval = milliseconds{2000}});
    EXPECT_EQ(problemKeys(result),
              (Keys{"dashboardPanels.gpu", "displayUnits.temperature", "fontSize", "themeColors.Light.background"}));
}

TEST(SettingsFile, ParseSettings_VersionOtherThanOne_ReportedAndTheSettingsRead)
{
    struct Case
    {
        const char* name;
        const char* text;
        Keys problemKeys;
    };
    const std::vector<Case> cases{
        {"version 1", R"({"version": 1, "logLevel": "debug"})", {}},
        {"no version", R"({"logLevel": "debug"})", {}},
        {"a newer version", R"({"version": 2, "logLevel": "debug"})", {"version"}},
        {"version 0", R"({"version": 0, "logLevel": "debug"})", {"version"}},
        {"version as text", R"({"version": "1", "logLevel": "debug"})", {"version"}},
    };

    for (const auto& [name, text, keys] : cases) {
        SCOPED_TRACE(name);

        const SettingsReadResult result = parseSettings(text);

        EXPECT_EQ(result.settings, Settings{.logLevel = LogLevel::Debug});
        EXPECT_EQ(problemKeys(result), keys);
    }
}

TEST(SettingsFile, ParseSettings_NotAJsonObject_GivesTheDefaultsAndOneProblem)
{
    struct Case
    {
        const char* name;
        const char* text;
    };
    const std::vector<Case> cases{
        {"empty", ""},
        {"cut short", R"({"logLevel": "debug")"},
        {"trailing comma", R"({"logLevel": "debug",})"},
        {"comment", "{\"logLevel\": \"debug\" // the default is info\n}"},
        {"two objects", R"({"logLevel": "debug"} {})"},
        {"array", R"([{"logLevel": "debug"}])"},
        {"string", R"("debug")"},
        {"null", "null"},
    };

    for (const auto& [name, text] : cases) {
        SCOPED_TRACE(name);

        const SettingsReadResult result = parseSettings(text);

        EXPECT_EQ(result.settings, Settings{});
        EXPECT_EQ(problemKeys(result), Keys{""});
    }
}

TEST(SettingsFile, ParseSettings_SyntaxErrorAfterValuesOfEveryKind_ProblemSaysWhere)
{
    // The values before the error are each of a kind the parser reports
    // separately, which must not stop it before it reaches the error.
    const SettingsReadResult result = parseSettings(
        "{\n    \"a\": null, \"b\": true, \"c\": -1, \"d\": 1, \"e\": 1.5, \"f\": \"x\", \"g\": [{}],\n}\n");

    ASSERT_EQ(result.problems.size(), 1u);
    EXPECT_NE(result.problems.front().description.find("line 3, column 1"), std::string::npos)
        << result.problems.front().description;
}

TEST(SettingsFile, ParseSettings_Utf8ByteOrderMark_Skipped)
{
    // Notepad's "UTF-8 with BOM" and Windows PowerShell 5.1's "-Encoding utf8" begin a file with one.
    const SettingsReadResult result = parseSettings("\xEF\xBB\xBF{\"logLevel\": \"debug\"}");

    EXPECT_EQ(result.settings, Settings{.logLevel = LogLevel::Debug});
    EXPECT_TRUE(result.problems.empty());
}

// ---------------------------------------------------------------------------
// serializeTheme and parseTheme
// ---------------------------------------------------------------------------

TEST(SettingsFile, SerializeTheme_TheVersionTheSchemeAndTheColours)
{
    Theme theme = themeWithCard();
    theme.colors[ThemeColorRole::Accent] = RgbColor{.red = 0xf4, .green = 0xb7, .blue = 0x40};
    const std::string expected = R"({
    "version": 1,
    "scheme": "dark",
    "colors": {
        "card": "#202020",
        "accent": "#f4b740"
    }
}
)";

    EXPECT_EQ(serializeTheme(theme), expected);
}

TEST(SettingsFile, SerializeThenParseTheme_EachScheme_ReadsBackWithoutProblems)
{
    for (const ColorScheme scheme : {ColorScheme::Light, ColorScheme::Dark}) {
        SCOPED_TRACE(scheme == ColorScheme::Dark ? "dark" : "light");
        Theme theme = themeWithCard();
        theme.scheme = scheme;

        const ThemeReadResult result = parseTheme(serializeTheme(theme));

        EXPECT_EQ(result.theme, theme);
        EXPECT_TRUE(result.problems.empty());
    }
}

TEST(SettingsFile, ParseTheme_ValuesNotUsed_TheOthersReadAndEachReported)
{
    const ThemeReadResult result = parseTheme(R"({
        "version": 2,
        "scheme": "dark",
        "colors": {"card": "#202020", "page": "black", "background": "#000000"},
        "author": "me"
    })");

    EXPECT_EQ(result.theme, themeWithCard());
    EXPECT_EQ(problemKeys(result), (Keys{"author", "colors.background", "colors.page", "version"}));
}

TEST(SettingsFile, ParseTheme_NoThemeToRead_NoThemeAndOneProblem)
{
    struct Case
    {
        const char* name{};
        const char* text{};
    };
    const std::array cases{
        Case{"empty", ""},
        Case{"not valid JSON", R"({"scheme": {)"},
        Case{"an array", R"([{"scheme": "dark"}])"},
        Case{"no scheme", R"({"version": 1, "colors": {"card": "#202020"}})"},
        Case{"unknown scheme", R"({"scheme": "dusk", "colors": {}})"},
        Case{"scheme in another case", R"({"scheme": "Dark", "colors": {}})"},
        Case{"a settings file", R"({"version": 1, "theme": "Dark"})"},
    };

    for (const auto& [name, text] : cases) {
        SCOPED_TRACE(name);

        const ThemeReadResult result = parseTheme(text);

        EXPECT_FALSE(result.theme.has_value());
        // The settings file's theme is not part of a theme file.
        EXPECT_EQ(result.problems.size(), std::string_view{name} == "a settings file" ? 2u : 1u);
    }
}
