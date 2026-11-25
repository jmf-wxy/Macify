@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

echo ========================================================
echo   Macify v0.0.7 Builder (Native C++ Desktop Suite)
echo ========================================================

:: If g++ is not in current session PATH, check D:\tools\w64devkit\bin
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    if exist "D:\tools\w64devkit\bin\g++.exe" (
        set "PATH=D:\tools\w64devkit\bin;%PATH%"
    )
)

:: 1. Check for g++ (MinGW / GCC)
where g++ >nul 2>nul
if %errorlevel% equ 0 (
    echo [*] Found g++ compiler. Building lightweight executable...
    taskkill /F /IM MacDesktopBeauty.exe >nul 2>nul
    if not exist "bin" mkdir bin
    
    where windres >nul 2>nul
    if %errorlevel% equ 0 (
        if exist "app.rc" (
            echo [*] Compiling application icon resource...
            windres -i app.rc -o app.res.o
        )
    )
    
    set "RES_OBJ="
    if exist "app.res.o" set "RES_OBJ=app.res.o"
    
    g++ -std=c++20 -O2 -mwindows -municode ^
        src/main.cpp ^
        src/core/D2DContext.cpp ^
        src/core/InputBox.cpp ^
        src/dock/DockWindow.cpp ^
        src/drawer/AppScanner.cpp ^
        src/drawer/DrawerWindow.cpp ^
        src/widgets/ClockWidget.cpp ^
        src/widgets/DiskWidget.cpp ^
        src/widgets/SysMonitorWidget.cpp ^
        src/widgets/CalendarWidget.cpp ^
        src/widgets/MemoWidget.cpp ^
        src/widgets/WidgetHostWindow.cpp ^
        src/settings/SettingsWindow.cpp ^
        !RES_OBJ! ^
        -ld2d1 -ldwrite -lwindowscodecs -ldwmapi -lole32 -lshell32 -lgdi32 -luser32 -luuid -ladvapi32 -lcomdlg32 ^
        -o bin/MacDesktopBeauty.exe
        
    if %errorlevel% equ 0 (
        :: Preserve user configuration: only initialize if bin\config.json does not exist
        if not exist "bin\config.json" (
            echo [*] Initializing bin\config.json from template...
            copy /Y config.json bin\config.json >nul
        ) else (
            echo [*] Preserved existing user configuration in bin\config.json
        )
        if exist "app.ico" copy /Y app.ico bin\app.ico >nul
        echo ========================================================
        echo [SUCCESS] Build succeeded: bin\MacDesktopBeauty.exe
        echo Binary size: ~580 KB (Pure native, ultra-low memory)
        echo You can now launch bin\MacDesktopBeauty.exe directly!
        echo ========================================================
        exit /b 0
    ) else (
        echo [FAILED] Compilation returned error.
        exit /b 1
    )
)

:: 2. Check for CMake
where cmake >nul 2>nul
if %errorlevel% equ 0 (
    echo [*] Found CMake. Building with CMake...
    cmake -B build
    cmake --build build --config Release
    exit /b %errorlevel%
)

:: 3. Neither found
echo [!] Neither g++ nor cmake was found.
echo Please ensure MinGW is in D:\tools\w64devkit\bin or in your PATH.
pause
