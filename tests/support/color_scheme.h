#pragma once

#include <QCoreApplication>
#include <QGuiApplication>
#include <QStyleHints>

namespace verdandi::tests
{

/** Sets the application's colour scheme while it exists, and returns to the system's afterwards. */
class ScopedColorScheme
{
public:
    explicit ScopedColorScheme(Qt::ColorScheme scheme)
    {
        QGuiApplication::styleHints()->setColorScheme(scheme);
        QCoreApplication::processEvents();
    }

    ~ScopedColorScheme()
    {
        QGuiApplication::styleHints()->unsetColorScheme();
        QCoreApplication::processEvents();
    }

    ScopedColorScheme(const ScopedColorScheme&) = delete;
    ScopedColorScheme& operator=(const ScopedColorScheme&) = delete;
    ScopedColorScheme(ScopedColorScheme&&) = delete;
    ScopedColorScheme& operator=(ScopedColorScheme&&) = delete;
};

} // namespace verdandi::tests
