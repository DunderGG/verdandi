#pragma once

#include <cstddef>

namespace verdandi::domain
{

/**
 * A colour that the monitor draws itself, where the palette has no role for
 * it, named by what it colours (DC-150). Each has a shade for a light surface
 * and one for a dark surface.
 */
enum class ThemeColorRole
{
    /** The charts' lines, in the order that a chart's series take them: CPU, memory and Receive take the first. */
    ChartLine1,
    ChartLine2,
    ChartLine3,
    ChartLine4,
    ChartLine5,
    /** The charts' grid and frame. */
    ChartGrid,
    /** Text that says a state is good, such as Healthy or an adapter that is Up. */
    Good,
    /** Text that says a state needs attention. */
    Warning,
    /** Text that says a state is critical. */
    Critical,
    /** The row of a process that started in the last few seconds. */
    ProcessStarted,
    /** The row of a process that has exited, until it is removed. */
    ProcessExited,
    /** The padlock before a process whose details Windows denied. */
    AccessDenied,
    /** The shield before a process that the monitor never opens. */
    AccessNeverOpened,
    /** The barred circle before a process that has no details to read. */
    AccessUnavailable,
};

/**
 * The number of ThemeColorRole values, for arrays indexed by a role's value.
 * AccessUnavailable must stay the last value: a value added after it takes
 * its place here.
 */
inline constexpr std::size_t kThemeColorRoleCount = static_cast<std::size_t>(ThemeColorRole::AccessUnavailable) + 1;

} // namespace verdandi::domain
