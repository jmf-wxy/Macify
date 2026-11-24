#pragma once
#include <windows.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace Core {

// DWM System Backdrop types for Windows 11 build 22621+
enum DWM_BACKDROP_TYPE {
    BACKDROP_AUTO = 0,
    BACKDROP_NONE = 1,
    BACKDROP_MAINSURFACE_MICA = 2,
    BACKDROP_TRANSIENT_ACRYLIC = 3,
    BACKDROP_TABBED_MICA_ALT = 4
};

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

enum DWM_WINDOW_CORNER_PREFERENCE {
    DWMWCP_DEFAULT = 0,
    DWMWCP_DONOTROUND = 1,
    DWMWCP_ROUND = 2,
    DWMWCP_ROUNDSMALL = 3
};

enum PreferredAppMode {
    Default = 0,
    AllowDark = 1,
    ForceDark = 2,
    ForceLight = 3,
    Max = 4
};

inline void InitializeDarkMode() {
    HMODULE hUxTheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (hUxTheme) {
        using fnSetPreferredAppMode = PreferredAppMode (WINAPI*)(PreferredAppMode);
        using fnFlushMenuThemes = void (WINAPI*)();
        auto SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(135));
        auto FlushMenuThemes = (fnFlushMenuThemes)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(136));
        if (SetPreferredAppMode) {
            SetPreferredAppMode(ForceDark);
        }
        if (FlushMenuThemes) {
            FlushMenuThemes();
        }
    }
}

inline void RemoveWindowBorder(HWND hwnd) {
    // 0. Enable immersive dark mode on the window
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    // 1. Tell DWM not to draw any window border (DWMWA_COLOR_NONE = 0xFFFFFFFE)
    COLORREF noneColor = 0xFFFFFFFE;
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &noneColor, sizeof(noneColor));

    // 2. Do not let DWM round the outer transparent window
    DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    // 3. Extend frame into client area to allow full transparency
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

inline void EnableBackdrop(HWND hwnd, DWM_BACKDROP_TYPE type = BACKDROP_TRANSIENT_ACRYLIC) {
    // 1. Enable immersive dark mode by default for modern macOS dark look
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    // 2. Set rounded corners (Win11 standard)
    DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    // 3. Set modern system backdrop (Mica / Acrylic)
    int backdrop = type;
    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));

    // 4. Extend frame into client area to allow backdrop rendering across entire window
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

} // namespace Core
