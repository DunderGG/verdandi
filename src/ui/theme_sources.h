#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <QPalette>

#include "domain/theme.h"

namespace verdandi::ui
{

/**
 * What reading a theme file gave: its theme, or std::nullopt when it holds
 * none, and a sentence for each value that was not used as it stood.
 */
struct ThemeImport
{
    std::optional<domain::Theme> theme{};
    std::vector<std::string> problems{};
};

/**
 * Where the settings dialog finds the themes of the themes folder and the
 * scheme that Windows sets, and the functions that it reads and writes theme
 * files with, which main.cpp sets, since ui may not depend on persistence
 * (DC-152, DC-153).
 * themes             — returns the themes of the themes folder, ordered by
 *                      name.
 * windowsColorScheme — returns the scheme that Windows sets for apps, whose
 *                      theme Follow Windows draws with.
 * windowsPalette     — returns Windows' own palette for a scheme, whose
 *                      colours the colour samples take for the palette's
 *                      roles that neither a theme nor the user chose.
 * readTheme          — reads the theme file at a path.
 * addTheme           — writes a theme to the themes folder, in place of the
 *                      one of the same name, and returns std::nullopt when it
 *                      did, and otherwise why it did not.
 * exportTheme        — writes a theme to the file at a path, and returns
 *                      std::nullopt when it did, and otherwise why it did not.
 * While themes is empty, the dialog offers the built-in themes only; while
 * windowsColorScheme is, Follow Windows takes the scheme that the application
 * draws in; and while windowsPalette is, the samples take the application's
 * palette. Import... is disabled while themes, readTheme or addTheme is
 * empty, and Export... while exportTheme is.
 */
struct ThemeSources
{
    std::function<std::vector<domain::NamedTheme>()> themes{};
    std::function<domain::ColorScheme()> windowsColorScheme{};
    std::function<QPalette(domain::ColorScheme)> windowsPalette{};
    std::function<ThemeImport(const std::filesystem::path&)> readTheme{};
    std::function<std::optional<std::string>(const domain::NamedTheme&)> addTheme{};
    std::function<std::optional<std::string>(const std::filesystem::path&, const domain::Theme&)> exportTheme{};
};

} // namespace verdandi::ui
