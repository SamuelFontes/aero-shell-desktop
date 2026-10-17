cmake_minimum_required(VERSION 3.24)
project(aeroshell-desktop VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(GNUInstallDirs)
include(CTest)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/aeroshell" CACHE PATH "AeroShell private runtime prefix" FORCE)
endif()
if(NOT CMAKE_INSTALL_PREFIX STREQUAL "/opt/aeroshell")
    message(FATAL_ERROR "AeroShell must use the private /opt/aeroshell prefix; system integration paths are declared separately.")
endif()
foreach(directory_var IN ITEMS CMAKE_INSTALL_BINDIR CMAKE_INSTALL_LIBEXECDIR)
    if(IS_ABSOLUTE "${${directory_var}}" OR "${${directory_var}}" MATCHES "(^|/)\\.\\.(/|$)")
        message(FATAL_ERROR "${directory_var} must be a safe relative path below /opt/aeroshell.")
    endif()
endforeach()

find_package(Qt6 6.8 REQUIRED COMPONENTS Core DBus Gui Widgets Qml Quick QuickControls2)
find_package(KF6KIO REQUIRED)
find_package(KF6Service REQUIRED)
find_package(KF6Svg REQUIRED)
find_package(KF6WindowSystem REQUIRED)
find_package(KWayland REQUIRED)
find_package(PolkitQt6-1 REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)
add_subdirectory(src/effects/aeroglassblur)

qt_add_executable(aeroshell-shell
    src/shell/main.cpp
    src/shell/shellactions.h
    src/shell/taskbarmodel.cpp
    src/shell/taskbarmodel.h
    src/shell/audiocontrol.cpp
    src/shell/audiocontrol.h
    src/shell/batterystatus.cpp
    src/shell/batterystatus.h
    src/shell/statusnotifier.cpp
    src/shell/statusnotifier.h
    src/shell/keyboardlayoutstatus.cpp
    src/shell/keyboardlayoutstatus.h
    src/shell/networkstatus.cpp
    src/shell/networkstatus.h
    src/shell/notifications.cpp
    src/shell/notifications.h
    src/shell/polkitagent.cpp
    src/shell/polkitagent.h
    src/shell/poweractions.cpp
    src/shell/poweractions.h
    src/shell/screenlockcontroller.cpp
    src/shell/screenlockcontroller.h
    src/shell/applicationmodel.cpp
    src/shell/applicationmodel.h
    src/shell/shellbridge.cpp
    src/shell/shellbridge.h
    src/shell/themeiconprovider.cpp
    src/shell/themeiconprovider.h
    src/shell/shellsettings.cpp
    src/shell/shellsettings.h
    src/shell/desktopitemmodel.cpp
    src/shell/desktopitemmodel.h
    src/shell/dbusmenumodel.cpp
    src/shell/dbusmenumodel.h
    src/shell/screenevents.cpp
    src/shell/screenevents.h
    src/shell/shellsounds.cpp
    src/shell/shellsounds.h
)

qt_add_qml_module(aeroshell-shell
    URI AeroShell.Shell
    VERSION 1.0
    QML_FILES
        src/shell/qml/ShellRoot.qml
        src/shell/qml/DesktopWindow.qml
        src/shell/qml/PanelWindow.qml
        src/shell/qml/LauncherWindow.qml
        src/shell/qml/SettingsWindow.qml
        src/shell/qml/PolkitAuthWindow.qml
        src/shell/qml/RunWindow.qml
        src/shell/qml/SecurityWindow.qml
)

target_link_libraries(aeroshell-shell PRIVATE
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    KF6::KIOGui
    KF6::Service
    KF6::WindowSystem
    Plasma::KWaylandClient
    PolkitQt6-1::Agent
    PolkitQt6-1::Core
)

qt_add_executable(aeroshell-explorer src/explorer/main.cpp src/explorer/explorer.cpp src/explorer/explorer.h src/explorer/taskmanager.cpp src/explorer/taskmanager.h src/explorer/recyclebin.cpp src/explorer/recyclebin.h)
target_link_libraries(aeroshell-explorer PRIVATE Qt6::Widgets KF6::KIOWidgets)
install(TARGETS aeroshell-explorer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES session/aeroshell-mimeapps.list DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES session/org.aeroshell.explorer.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

install(TARGETS aeroshell-shell RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY assets/ DESTINATION ${CMAKE_INSTALL_DATADIR}/aeroshell PATTERN "plasma" EXCLUDE)
install(DIRECTORY assets/plasma/ DESTINATION ${CMAKE_INSTALL_DATADIR}/plasma)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/aeroshell/LICENSES RENAME MIT.txt)
install(PROGRAMS session/aeroshell-shell-session DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
install(PROGRAMS session/aeroshell-check-compatibility DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
install(PROGRAMS session/aeroshell-session-bus DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
install(FILES session/org.aeroshell.shell.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(PROGRAMS session/aeroshell-session-wayland DESTINATION /usr/bin)
install(FILES session/aeroshell.desktop DESTINATION /usr/share/wayland-sessions)
install(FILES packaging/aeroshell-portals.conf DESTINATION /usr/share/xdg-desktop-portal)

if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_executable(test-aero-glass-plugin tests/test-aero-glass-plugin.cpp)
    target_link_libraries(test-aero-glass-plugin PRIVATE Qt6::Core)
    add_test(NAME aero-glass-plugin COMMAND test-aero-glass-plugin $<TARGET_FILE:aeroglassblur>)
    add_executable(test-taskbar tests/test-taskbar.cpp src/shell/taskbarmodel.cpp src/shell/taskbarmodel.h src/shell/applicationmodel.cpp src/shell/applicationmodel.h)
    target_link_libraries(test-taskbar PRIVATE Qt6::Gui KF6::KIOGui KF6::Service Plasma::KWaylandClient)
    add_test(NAME taskbar COMMAND test-taskbar)
    add_executable(test-keyboard-status tests/test-keyboard-status.cpp src/shell/keyboardlayoutstatus.cpp src/shell/keyboardlayoutstatus.h)
    target_link_libraries(test-keyboard-status PRIVATE Qt6::Core Qt6::DBus)
    add_test(NAME keyboard-status COMMAND dbus-run-session -- $<TARGET_FILE:test-keyboard-status>)
    set_tests_properties(keyboard-status PROPERTIES TIMEOUT 10)
    add_executable(test-explorer tests/test-explorer.cpp src/explorer/explorer.cpp src/explorer/explorer.h)
    target_link_libraries(test-explorer PRIVATE Qt6::Widgets KF6::KIOWidgets)
    add_test(NAME explorer COMMAND dbus-run-session -- $<TARGET_FILE:test-explorer>)
    set_tests_properties(explorer PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(test-audio-control
        tests/test-audio-control.cpp
        src/shell/audiocontrol.cpp
        src/shell/audiocontrol.h
    )
    target_link_libraries(test-audio-control PRIVATE Qt6::Core)
    add_test(NAME audio-control COMMAND test-audio-control)

    add_executable(test-battery-status
        tests/test-battery-status.cpp
        src/shell/batterystatus.cpp
        src/shell/batterystatus.h
    )
    target_link_libraries(test-battery-status PRIVATE Qt6::Core)
    add_test(NAME battery-status COMMAND test-battery-status)

    add_executable(test-network-status
        tests/test-network-status.cpp
        src/shell/networkstatus.cpp
        src/shell/networkstatus.h
    )
    target_link_libraries(test-network-status PRIVATE Qt6::Core)
    add_test(NAME network-status COMMAND test-network-status)

    add_executable(test-statusnotifier-icon
        tests/test-statusnotifier-icon.cpp
        src/shell/statusnotifiericon.h
    )
    target_link_libraries(test-statusnotifier-icon PRIVATE Qt6::Gui)
    add_test(NAME statusnotifier-icon COMMAND test-statusnotifier-icon)

    add_executable(test-theme-assets
        tests/test-theme-assets.cpp
        src/shell/themeiconprovider.cpp
        src/shell/themeiconprovider.h
    )
    target_link_libraries(test-theme-assets PRIVATE Qt6::Gui Qt6::Quick KF6::Svg)
    target_compile_definitions(test-theme-assets PRIVATE AEROSHELL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME theme-assets COMMAND test-theme-assets)
    set_tests_properties(theme-assets PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(test-notifications
        tests/test-notifications.cpp
        src/shell/notifications.cpp
        src/shell/notifications.h
    )
    target_link_libraries(test-notifications PRIVATE Qt6::Core Qt6::DBus Qt6::Gui)
    add_test(NAME notifications COMMAND test-notifications)

    add_executable(test-shell-settings
        tests/test-shell-settings.cpp
        src/shell/shellsettings.cpp
        src/shell/shellsettings.h
    )
    target_link_libraries(test-shell-settings PRIVATE Qt6::Core Qt6::Gui)
    target_compile_definitions(test-shell-settings PRIVATE AEROSHELL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME shell-settings COMMAND test-shell-settings)
    set_tests_properties(shell-settings PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_test(NAME kwin-compatibility COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/test-compatibility.sh)
    add_executable(test-qml-startup tests/test-qml-startup.cpp
        src/shell/themeiconprovider.cpp src/shell/themeiconprovider.h
        src/shell/notifications.cpp src/shell/notifications.h)
    target_link_libraries(test-qml-startup PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::DBus Plasma::KWaylandClient KF6::Service)
    target_compile_definitions(test-qml-startup PRIVATE AEROSHELL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME qml-startup COMMAND test-qml-startup)
    set_tests_properties(qml-startup PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
    add_test(NAME session-launch COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/test-session-launch.sh)
    add_test(NAME default-appearance
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test-default-appearance.py)
endif()
