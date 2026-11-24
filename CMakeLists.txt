cmake_minimum_required(VERSION 3.20)
project(Macify VERSION 0.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows Subsystem: GUI (no console pop-up in release)
if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
    add_compile_options(/W4 /utf-8 /EHsc)
else()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
    add_compile_options(-Wall -Wextra -std=c++20)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/core/D2DContext.cpp
    src/dock/DockWindow.cpp
    src/drawer/AppScanner.cpp
    src/drawer/DrawerWindow.cpp
    src/widgets/ClockWidget.cpp
    src/widgets/DiskWidget.cpp
    src/widgets/WidgetHostWindow.cpp
    src/settings/SettingsWindow.cpp
)

# Header files
set(HEADERS
    src/core/D2DContext.h
    src/core/Backdrop.h
    src/dock/DockItem.h
    src/dock/DockWindow.h
    src/drawer/AppItem.h
    src/drawer/AppScanner.h
    src/drawer/DrawerWindow.h
    src/widgets/IWidget.h
    src/widgets/ClockWidget.h
    src/widgets/DiskWidget.h
    src/widgets/WidgetHostWindow.h
    src/settings/AppSettings.h
    src/settings/SettingsWindow.h
)

add_executable(MacDesktopBeauty WIN32 ${SOURCES} ${HEADERS})

# Link Win32 and Direct2D libraries
target_link_libraries(MacDesktopBeauty PRIVATE
    user32
    gdi32
    shell32
    ole32
    uuid
    advapi32
    comdlg32
    d2d1
    dwrite
    windowscodecs
    dwmapi
)

# Copy config.json to build output directory
add_custom_command(TARGET MacDesktopBeauty POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
    "$<TARGET_FILE_DIR:MacDesktopBeauty>/config.json"
)
