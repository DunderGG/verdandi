#pragma once

#include <chrono>
#include <ostream>
#include <string>

#include "domain/settings.h"
#include "domain/theme.h"
#include "persistence/settings_file.h"

namespace verdandi::domain
{

/** Prints settings in a failed expectation as a settings file's JSON. GoogleTest finds it by argument lookup. */
inline void PrintTo(const Settings& settings, std::ostream* stream)
{
    *stream << persistence::serializeSettings(settings);
}

} // namespace verdandi::domain

namespace verdandi::tests
{

/**
 * Settings in which every value differs from its default: the Dark theme,
 * and a colour chosen in it and in the Light theme.
 */
[[nodiscard]] inline domain::Settings nonDefaultSettings()
{
    domain::Settings settings{
        .refreshInterval = std::chrono::milliseconds{2500},
        .displayUnits = {.bytes = domain::ByteUnits::Decimal, .networkRates = domain::NetworkRateUnits::BitsPerSecond},
        .dashboardPanels = {.cpu = false, .memory = false, .disk = false, .network = false, .uptime = false},
        .theme = std::string{domain::kDarkTheme},
        .logLevel = domain::LogLevel::Debug,
    };
    settings.themeColors[std::string{domain::kLightTheme}][domain::ThemeColorRole::Page] =
        domain::RgbColor{.red = 0xf0, .green = 0xf0, .blue = 0xf0};
    settings.themeColors[std::string{domain::kDarkTheme}][domain::ThemeColorRole::Card] =
        domain::RgbColor{.red = 0x20, .green = 0x20, .blue = 0x20};
    return settings;
}

} // namespace verdandi::tests
