#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "domain/settings.h"
#include "domain/theme.h"

namespace verdandi::persistence
{

/** The format version that serializeSettings() writes as the file's "version". */
inline constexpr int kSettingsVersion = 1;

/**
 * A value in a settings file that was not used as it stood, and what was used
 * instead. key is the setting's path in the file, such as "refreshIntervalMs"
 * or "dashboardPanels.cpu", and is empty when the problem is with the whole
 * file. description is a sentence for the log, which names the key.
 */
struct SettingsProblem
{
    std::string key{};
    std::string description{};
};

/** Settings read from a settings file, and the problems found in it. */
struct SettingsReadResult
{
    domain::Settings settings{};
    std::vector<SettingsProblem> problems{};
};

/** Returns the JSON text of a settings file that holds settings, ending with a newline. */
[[nodiscard]] std::string serializeSettings(const domain::Settings& settings);

/**
 * Reads settings from the JSON text of a settings file, one setting at a time
 * (DC-123). A setting that is missing keeps its default. A whole number of
 * milliseconds that is not a refresh interval a user can choose takes the
 * nearest that is (domain::nearestRefreshInterval()), and any other value that
 * is not valid keeps its default. A key that is not a setting is ignored. Text that
 * is not a JSON object gives the defaults. Every value not used as it stood,
 * and every key ignored, is reported as a problem.
 */
[[nodiscard]] SettingsReadResult parseSettings(std::string_view text);

/**
 * Reads the settings file at path. Returns std::nullopt when there is no file
 * there, as before the first save, so the caller uses the defaults. A file
 * that cannot be opened gives the defaults and one problem.
 */
[[nodiscard]] std::optional<SettingsReadResult> loadSettingsFile(const std::filesystem::path& path);

/**
 * Writes settings to the file at path, creating its directory. The text goes
 * to a temporary file beside it first, which then replaces the file, so a save
 * that fails leaves the previous file whole. Returns std::nullopt when the
 * file was written, and otherwise why it was not.
 */
[[nodiscard]] std::optional<std::string> saveSettingsFile(const std::filesystem::path& path,
                                                          const domain::Settings& settings);

/**
 * Colours read from a theme file, and the problems found in it. theme is
 * std::nullopt when the file holds no theme to read, and then problems says
 * why.
 */
struct ThemeReadResult
{
    std::optional<domain::Theme> theme{};
    std::vector<SettingsProblem> problems{};
};

/**
 * Returns the JSON text of a theme file that holds theme, ending with a
 * newline (DC-152). A theme file is a settings file that holds only the
 * version and the theme, so parseSettings() reads it as settings too.
 */
[[nodiscard]] std::string serializeTheme(const domain::Theme& theme);

/**
 * Reads the colours of a theme file, or of the theme of any settings file,
 * from its JSON text, one colour at a time, as parseSettings() reads them: a
 * colour that is not valid, or a role that is not known, is reported and
 * left out. The file's other settings are neither read nor reported. Text
 * that is not a JSON object, or holds no theme object, gives no theme and one
 * problem.
 */
[[nodiscard]] ThemeReadResult parseTheme(std::string_view text);

/** Reads the theme file at path. A file that cannot be opened gives no theme and one problem. */
[[nodiscard]] ThemeReadResult loadThemeFile(const std::filesystem::path& path);

/**
 * Writes theme to a theme file at path, creating its directory, through a
 * temporary file beside it as saveSettingsFile() does. Returns std::nullopt
 * when the file was written, and otherwise why it was not.
 */
[[nodiscard]] std::optional<std::string> saveThemeFile(const std::filesystem::path& path, const domain::Theme& theme);

} // namespace verdandi::persistence
