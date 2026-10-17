# The original runtime is built privately; no system KDE package is replaced.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(AEROSHELL_NATIVE_STAGE "" CACHE PATH "Previously built original runtime staging root (contains opt/aeroshell)")
set(AEROSHELL_NATIVE_JOBS 4 CACHE STRING "Parallel jobs per upstream build")
set(AEROSHELL_VULKAN_INCLUDE "" CACHE PATH "Optional Vulkan headers for an isolated SDK")
if(NOT AEROSHELL_NATIVE_STAGE)
    set(AEROSHELL_NATIVE_STAGE "${CMAKE_BINARY_DIR}/native-runtime/native-stage")
    add_custom_target(native-runtime ALL
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/build-native-runtime.py
            --build-root ${CMAKE_BINARY_DIR}/native-runtime
            --jobs ${AEROSHELL_NATIVE_JOBS}
            --vulkan-include "${AEROSHELL_VULKAN_INCLUDE}"
        USES_TERMINAL VERBATIM)
elseif(NOT EXISTS "${AEROSHELL_NATIVE_STAGE}/opt/aeroshell/share/aeroshell/native-sources.json")
    message(FATAL_ERROR "AEROSHELL_NATIVE_STAGE must contain a completed pinned runtime build")
else()
    execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/validate-native-runtime.py
        ${AEROSHELL_NATIVE_STAGE} COMMAND_ERROR_IS_FATAL ANY)
endif()
install(DIRECTORY "${AEROSHELL_NATIVE_STAGE}/opt/aeroshell/" DESTINATION .)
install(DIRECTORY defaults/native/ DESTINATION share/aeroshell/native-defaults)
foreach(asset IN ITEMS icons cursors sounds wallpapers)
    install(DIRECTORY assets/${asset} DESTINATION share/aeroshell)
endforeach()
install(PROGRAMS session/aeroshell-native-session DESTINATION /usr/bin RENAME aeroshell-session-wayland)
install(PROGRAMS session/aeroshell-native-desktop session/aeroshell-session-bus DESTINATION libexec)
install(FILES session/aeroshell.desktop DESTINATION /usr/share/wayland-sessions)
install(FILES packaging/aeroshell-portals.conf DESTINATION /usr/share/xdg-desktop-portal)
install(FILES LICENSE packaging/native-sources.json DESTINATION share/aeroshell)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
    add_executable(test-native-style tests/test-native-style.cpp)
    target_link_libraries(test-native-style PRIVATE Qt6::Widgets)
    find_package(KWayland REQUIRED)
    add_executable(native-input tests/native-input.cpp)
    target_link_libraries(native-input PRIVATE Qt6::Widgets Plasma::KWaylandClient)
    add_test(NAME native-profile COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test-native-profile.py)
    add_test(NAME install-policy COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test-install-policy.py)
endif()
