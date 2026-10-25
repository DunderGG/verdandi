#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <string>
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
Keys problemKeys(const SettingsReadResult& result)
{
    Keys keys;
    for (const auto& problem : result.problems) {
        keys.push_back(problem.key);
    }
    std::ranges::sort(keys);
    return keys;
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
        {"Windows' colour scheme", R"({"colorScheme": "system"})", Settings{.colorScheme = ColorScheme::System}},
        {"light colour scheme", R"({"colorScheme": "light"})", Settings{.colorScheme = ColorScheme::Light}},
        {"dark colour scheme", R"({"colorScheme": "dark"})", Settings{.colorScheme = ColorScheme::Dark}},
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
    "colorScheme": "system",
    "theme": {
        "light": {},
        "dark": {}
    },
    "logLevel": "info"
}
)";

    EXPECT_EQ(serializeSettings(Settings{}), expected);
}

TEST(SettingsFile, SerializeSettings_ChosenColours_WrittenInLowercaseUnderTheirRolesInRoleOrder)
{
    Settings settings;
    settings.theme.light[ThemeColorRole::Page] = RgbColor{.red = 0xf0, .green = 0xf0, .blue = 0xf0};
    settings.theme.dark[ThemeColorRole::AccessDenied] = RgbColor{.red = 0xab, .green = 0xcd, .blue = 0xef};
    settings.theme.dark[ThemeColorRole::Card] = RgbColor{.red = 0x20, .green = 0x20, .blue = 0x20};
    const std::string expected = R"(    "theme": {
        "light": {
            "page": "#f0f0f0"
        },
        "dark": {
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
        expected.theme.dark[role] = RgbColor{.red = 0x01, .green = 0x02, .blue = 0x03};

        const SettingsReadResult result =
            parseSettings(std::format(R"({{"theme": {{"dark": {{"{}": "#010203"}}}}}})", name));

        EXPECT_EQ(result.settings, expected);
        EXPECT_TRUE(result.problems.empty());
    }
}

TEST(SettingsFile, ParseSettings_ThemeColours_EitherCaseInEitherScheme)
{
    Settings expected;
    expected.theme.light[ThemeColorRole::ChartLine1] = RgbColor{.red = 0xab, .green = 0xcd, .blue = 0xef};
    expected.theme.dark[ThemeColorRole::Text] = RgbColor{.red = 0xab, .green = 0xcd, .blue = 0xef};

    const SettingsReadResult result =
        parseSettings(R"({"theme": {"light": {"chartLine1": "#abcdef"}, "dark": {"text": "#ABcDeF"}}})");

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
        {"unknown colour scheme", R"({"colorScheme": "auto"})", "colorScheme"},
        {"colour scheme in another case", R"({"colorScheme": "Dark"})", "colorScheme"},
        {"colour scheme as a boolean", R"({"colorScheme": true})", "colorScheme"},
        {"theme not an object", R"({"theme": "dark"})", "theme"},
        {"scheme's colours not an object", R"({"theme": {"dark": ["#000000"]}})", "theme.dark"},
        {"colour as a number", R"({"theme": {"dark": {"card": 2105376}}})", "theme.dark.card"},
        {"colour by name", R"({"theme": {"dark": {"card": "red"}}})", "theme.dark.card"},
        {"colour without #", R"({"theme": {"dark": {"card": "202020"}}})", "theme.dark.card"},
        {"colour of three digits", R"({"theme": {"dark": {"card": "#222"}}})", "theme.dark.card"},
        {"colour with opacity", R"({"theme": {"dark": {"card": "#20202080"}}})", "theme.dark.card"},
        {"colour with a non-hexadecimal digit", R"({"theme": {"dark": {"card": "#20202g"}}})", "theme.dark.card"},
        {"colour with a sign", R"({"theme": {"dark": {"card": "#+20202"}}})", "theme.dark.card"},
        {"colour null", R"({"theme": {"light": {"page": null}}})", "theme.light.page"},
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
        "theme": {"contrast": {}, "light": {"background": "#000000"}}
    })");

    EXPECT_EQ(result.settings, Settings{.refreshInterval = milliseconds{2000}});
    EXPECT_EQ(problemKeys(result), (Keys{"dashboardPanels.gpu", "displayUnits.temperature", "fontSize",
                                         "theme.contrast", "theme.light.background"}));
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
