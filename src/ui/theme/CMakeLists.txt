find_package(Qt6 CONFIG REQUIRED COMPONENTS Gui)

# The colours that the widgets draw themselves, where the palette has no role
# for them (DC-150), and the colours that the user chose (DC-151), shared by
# the chart widgets and the rest of the UI. Same dependency rules as src/ui/:
# Qt and domain types only, no monitoring, platform, or Windows headers.
add_library(verdandi_ui_theme STATIC
    theme_colors.cpp
    theme_colors.h
)

set_target_properties(verdandi_ui_theme PROPERTIES
    AUTOMOC ON
)

target_include_directories(verdandi_ui_theme
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)

target_link_libraries(verdandi_ui_theme
    PUBLIC
        Qt6::Gui
        verdandi_domain
)
