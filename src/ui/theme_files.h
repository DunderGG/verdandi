#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "domain/theme.h"

namespace verdandi::ui
{

/**
 * What reading a theme file gave: its colours, or std::nullopt when it holds
 * none, and a sentence for each value that was not used as it stood.
 */
struct ThemeImport
{
    std::optional<domain::Theme> theme{};
    std::vector<std::string> problems{};
};

/**
 * The functions that read and write theme files, which main.cpp sets, since
 * ui may not depend on persistence (DC-152). importTheme reads the file at a
 * path. exportTheme writes colours to the file at a path, and returns
 * std::nullopt when it did, and otherwise why it did not. While one is empty,
 * the settings dialog's button for it is disabled.
 */
struct ThemeFiles
{
    std::function<ThemeImport(const std::filesystem::path&)> importTheme{};
    std::function<std::optional<std::string>(const std::filesystem::path&, const domain::Theme&)> exportTheme{};
};

} // namespace verdandi::ui
