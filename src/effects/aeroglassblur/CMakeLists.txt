# AeroThemePlasma's Wayland Aero Glass effect, hosted only by AeroShell's KWin.
find_package(KWin REQUIRED)
find_package(KDecoration3 REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS WaylandClient)
find_package(WaylandProtocols 1.48 REQUIRED)
find_package(KF6Config REQUIRED)

kconfig_add_kcfg_files(aeroglassblur_CONFIG blurconfig.kcfgc)
add_library(aeroglassblur MODULE)
set_property(TARGET aeroglassblur APPEND PROPERTY AUTOMOC_MACRO_NAMES KWIN_EFFECT_FACTORY_SUPPORTED_ENABLED)
set_target_properties(aeroglassblur PROPERTIES PREFIX "" AUTORCC ON LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/effects")
target_include_directories(aeroglassblur PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_sources(aeroglassblur PRIVATE blur.cpp blur.h main.cpp aeroblur.qrc ${aeroglassblur_CONFIG})
target_compile_definitions(aeroglassblur PRIVATE KWIN_BUILD_WAYLAND)
target_link_options(aeroglassblur PRIVATE "LINKER:--no-undefined")
qt6_generate_wayland_protocol_server_sources(aeroglassblur PRIVATE_CODE FILES
    "${WaylandProtocols_DATADIR}/staging/ext-background-effect/ext-background-effect-v1.xml")
target_link_libraries(aeroglassblur PRIVATE KF6::ConfigGui KDecoration3::KDecoration KF6::Svg KWin::kwin)
install(TARGETS aeroglassblur LIBRARY DESTINATION lib/qt6/plugins/kwin/effects/plugins)
