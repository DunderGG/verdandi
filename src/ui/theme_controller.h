#pragma once

#include <string_view>

#include <QObject>

namespace verdandi::ui
{

/**
 * Follows the colour scheme that the application draws in (DC-147). Qt reads
 * the one that Windows sets for applications when the QApplication starts,
 * and again whenever Windows changes it, and its windows11 style and the
 * windows' title bars follow it. The scheme is Qt::ColorScheme::Light or
 * Dark, or Unknown while a Windows contrast theme is on, whose own colours
 * then replace both.
 *
 * One controller serves the whole application: main.cpp creates it before
 * the main window, so that it outlives every window.
 *
 * Not thread-safe; use only on the UI thread.
 */
class ThemeController : public QObject
{
    Q_OBJECT

public:
    /** Requires a QApplication. */
    explicit ThemeController(QObject* parent = nullptr);
    ~ThemeController() override = default;

    /** Returns the scheme that the application draws in now. */
    [[nodiscard]] Qt::ColorScheme colorScheme() const;

signals:
    /** Emitted with the new scheme when the scheme that the application draws in changes. */
    void colorSchemeChanged(Qt::ColorScheme scheme);
};

/**
 * Returns how the log names scheme: "the light colour scheme", "the dark
 * colour scheme", or "a contrast theme" for Qt::ColorScheme::Unknown.
 */
[[nodiscard]] std::string_view colorSchemeName(Qt::ColorScheme scheme);

} // namespace verdandi::ui
