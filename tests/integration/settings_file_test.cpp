#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "domain/settings.h"
#include "domain/theme.h"
#include "persistence/settings_file.h"
#include "support/settings_samples.h"
#include "support/temporary_directory.h"

using namespace verdandi::persistence;
using verdandi::domain::ColorScheme;
using verdandi::domain::NamedTheme;
using verdandi::domain::Settings;
using verdandi::domain::Theme;
using verdandi::tests::nonDefaultSettings;
using verdandi::tests::TemporaryDirectory;

namespace
{

void writeFile(const std::filesystem::path& path, std::string_view text)
{
    std::ofstream file(path, std::ios::binary);
    file << text;
    EXPECT_TRUE(file.good()) << "cannot write the test's file";
}

std::ptrdiff_t entryCount(const std::filesystem::path& directory)
{
    std::error_code error;
    const std::filesystem::directory_iterator entries{directory, error};
    EXPECT_FALSE(error) << error.message();
    return std::distance(entries, std::filesystem::directory_iterator{});
}

// A theme of the dark scheme that chooses its card colour.
Theme oceanTheme()
{
    Theme theme{.scheme = ColorScheme::Dark};
    theme.colors[verdandi::domain::ThemeColorRole::Card] =
        verdandi::domain::RgbColor{.red = 0x20, .green = 0x20, .blue = 0x20};
    return theme;
}

} // namespace

TEST(SettingsFileIntegration, SaveThenLoad_NewDirectory_ReadsTheSettingsBack)
{
    const TemporaryDirectory directory;
    const auto path = directory.path() / "Verdandi" / "settings.json";

    const auto saveError = saveSettingsFile(path, nonDefaultSettings());
    const auto loaded = loadSettingsFile(path);

    EXPECT_FALSE(saveError.has_value()) << *saveError;
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->settings, nonDefaultSettings());
    EXPECT_TRUE(loaded->problems.empty());
}

TEST(SettingsFileIntegration, LoadSettingsFile_NoFile_ReturnsNullopt)
{
    const TemporaryDirectory directory;

    EXPECT_FALSE(loadSettingsFile(directory.path() / "settings.json").has_value());
}

TEST(SettingsFileIntegration, SaveSettingsFile_ExistingFile_ReplacedWithoutATemporaryFileLeft)
{
    const TemporaryDirectory directory;
    const auto path = directory.path() / "settings.json";
    writeFile(path, "not the settings");

    const auto saveError = saveSettingsFile(path, nonDefaultSettings());
    const auto loaded = loadSettingsFile(path);

    EXPECT_FALSE(saveError.has_value()) << *saveError;
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->settings, nonDefaultSettings());
    EXPECT_EQ(entryCount(directory.path()), 1);
}

TEST(SettingsFileIntegration, SaveSettingsFile_TemporaryFileCannotBeCreated_LeavesThePreviousFileWhole)
{
    const TemporaryDirectory directory;
    const auto path = directory.path() / "settings.json";
    ASSERT_FALSE(saveSettingsFile(path, nonDefaultSettings()).has_value());
    // A directory in place of the temporary file, which settings_file.cpp names after the file.
    std::error_code error;
    std::filesystem::create_directory(directory.path() / "settings.json.tmp", error);
    ASSERT_FALSE(error) << error.message();

    const auto saveError = saveSettingsFile(path, Settings{});
    const auto loaded = loadSettingsFile(path);

    EXPECT_TRUE(saveError.has_value());
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->settings, nonDefaultSettings());
}

TEST(SettingsFileIntegration, LoadSettingsFile_Directory_GivesTheDefaultsAndOneProblem)
{
    const TemporaryDirectory directory;

    const auto loaded = loadSettingsFile(directory.path());

    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->settings, Settings{});
    ASSERT_EQ(loaded->problems.size(), 1u);
    EXPECT_TRUE(loaded->problems.front().key.empty());
}

TEST(SettingsFileIntegration, SaveSettingsFile_FileInPlaceOfTheDirectory_ReturnsAnError)
{
    const TemporaryDirectory directory;
    writeFile(directory.path() / "Verdandi", "a file, not a directory");

    const auto saveError = saveSettingsFile(directory.path() / "Verdandi" / "settings.json", Settings{});

    EXPECT_TRUE(saveError.has_value());
}

TEST(SettingsFileIntegration, SaveSettingsFile_DirectoryInPlaceOfTheFile_ReturnsAnErrorAndRemovesTheTemporaryFile)
{
    const TemporaryDirectory directory;
    const auto path = directory.path() / "settings.json";
    std::error_code error;
    std::filesystem::create_directory(path, error);
    ASSERT_FALSE(error) << error.message();

    const auto saveError = saveSettingsFile(path, Settings{});

    EXPECT_TRUE(saveError.has_value());
    EXPECT_EQ(entryCount(directory.path()), 1);
}

TEST(SettingsFileIntegration, SaveThemeFileThenLoad_NewDirectory_ReadsTheThemeBack)
{
    const TemporaryDirectory directory;
    const auto path = directory.path() / "themes" / "Ocean.json";

    const auto saveError = saveThemeFile(path, oceanTheme());
    const ThemeReadResult loaded = loadThemeFile(path);

    EXPECT_FALSE(saveError.has_value()) << *saveError;
    EXPECT_EQ(loaded.theme, oceanTheme());
    EXPECT_TRUE(loaded.problems.empty());
    // Written through a temporary file, which is gone.
    EXPECT_EQ(entryCount(path.parent_path()), 1);
}

TEST(SettingsFileIntegration, LoadThemeFile_NoFile_NoThemeAndAProblem)
{
    const TemporaryDirectory directory;

    const ThemeReadResult loaded = loadThemeFile(directory.path() / "missing.json");

    EXPECT_FALSE(loaded.theme.has_value());
    EXPECT_EQ(loaded.problems.size(), 1u);
}

TEST(SettingsFileIntegration, LoadThemeFolder_EachThemeFile_NamedAfterItInOrderOfName)
{
    const TemporaryDirectory directory;
    ASSERT_FALSE(saveThemeFile(directory.path() / "paper.JSON", Theme{}).has_value());
    ASSERT_FALSE(saveThemeFile(directory.path() / "Ocean.json", oceanTheme()).has_value());
    // Neither a theme file nor a theme: a built-in theme's name, a file that
    // names no scheme, a file of another kind and a directory.
    ASSERT_FALSE(saveThemeFile(directory.path() / "light.json", Theme{}).has_value());
    writeFile(directory.path() / "Broken.json", R"({"colors": {}})");
    writeFile(directory.path() / "notes.txt", "{}");
    std::error_code error;
    std::filesystem::create_directory(directory.path() / "folder.json", error);
    ASSERT_FALSE(error) << error.message();

    const ThemeFolderReadResult loaded = loadThemeFolder(directory.path());

    EXPECT_EQ(loaded.themes,
              (std::vector<NamedTheme>{{.name = "Ocean", .theme = oceanTheme()}, {.name = "paper", .theme = Theme{}}}));
    std::vector<std::string> problemKeys;
    for (const SettingsProblem& problem : loaded.problems) {
        problemKeys.push_back(problem.key);
        EXPECT_TRUE(problem.description.starts_with(problem.key)) << problem.description;
    }
    std::ranges::sort(problemKeys);
    EXPECT_EQ(problemKeys, (std::vector<std::string>{"Broken.json", "light.json"}));
}

TEST(SettingsFileIntegration, LoadThemeFolder_NoFolder_NoThemesAndNoProblem)
{
    const TemporaryDirectory directory;

    const ThemeFolderReadResult loaded = loadThemeFolder(directory.path() / "themes");

    EXPECT_TRUE(loaded.themes.empty());
    EXPECT_TRUE(loaded.problems.empty());
}

TEST(SettingsFileIntegration, LoadThemeFolder_TheThemesBesideTheApplication_VerdandiReadWithoutProblems)
{
    // The build copies the repository's themes beside verdandi.exe (DC-153).
    const auto folder = std::filesystem::path{VERDANDI_APPLICATION_PATH}.parent_path() / "themes";

    const ThemeFolderReadResult loaded = loadThemeFolder(folder);

    EXPECT_TRUE(loaded.problems.empty()) << loaded.problems.front().description;
    const auto verdandi = std::ranges::find(loaded.themes, std::string{"Verdandi"}, &NamedTheme::name);
    ASSERT_NE(verdandi, loaded.themes.end());
    EXPECT_EQ(verdandi->theme.scheme, ColorScheme::Dark);
}
