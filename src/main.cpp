#include <windows.h>
#include <shellapi.h>
#include "core/Backdrop.h"
#include "core/DragOverlay.h"
#include "core/Version.h"
#include "dock/DockWindow.h"
#include "drawer/DrawerWindow.h"
#include "widgets/WidgetHostWindow.h"
#include "widgets/ClockWidget.h"
#include "widgets/DiskWidget.h"
#include "widgets/SysMonitorWidget.h"
#include "widgets/CalendarWidget.h"
#include "widgets/MemoWidget.h"
#include "settings/AppSettings.h"
#include "settings/SettingsWindow.h"

constexpr UINT WM_TRAYICON = WM_USER + 101;
constexpr UINT ID_TRAY_EXIT = 2001;
constexpr UINT ID_TRAY_SETTINGS = 2002;
constexpr UINT TIMER_GAME_DETECTOR = 3001;
constexpr UINT GAME_CHECK_INTERVAL_MS = 1500;

NOTIFYICONDATAW g_nid = {};
HWND g_hMsgHwnd = nullptr;

struct AppContext {
    Dock::DockWindow* dock = nullptr;
    Widgets::WidgetHostWindow* widgetHost = nullptr;
    Drawer::DrawerWindow* drawer = nullptr;
    Settings::SettingsWindow* settingsWindow = nullptr;
    bool isSuspended = false;
};
static AppContext g_app;

// Reliable fullscreen game & D3D exclusive fullscreen detector
bool IsFullscreenGameRunning() {
    // 1. Direct3D exclusive fullscreen check (DirectX 9/11/12, Vulkan, OpenGL exclusive)
    QUERY_USER_NOTIFICATION_STATE state;
    if (SUCCEEDED(SHQueryUserNotificationState(&state))) {
        if (state == QUNS_RUNNING_D3D_FULL_SCREEN) {
            return true;
        }
    }

    // 2. Foreground Window check (covers Borderless Fullscreen games & full-screen exclusive)
    HWND fg = GetForegroundWindow();
    if (!fg) return false;

    // Filter desktop (Progman / WorkerW) and taskbar
    wchar_t className[256] = {};
    GetClassNameW(fg, className, 256);
    if (wcscmp(className, L"Progman") == 0 ||
        wcscmp(className, L"WorkerW") == 0 ||
        wcscmp(className, L"Shell_TrayWnd") == 0 ||
        wcscmp(className, L"Shell_SecondaryTrayWnd") == 0) {
        return false;
    }

    // Filter our own windows
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    if (pid == GetCurrentProcessId()) {
        return false;
    }

    // Check if the foreground window covers the entire monitor
    RECT rc;
    if (GetWindowRect(fg, &rc)) {
        HMONITOR hMon = MonitorFromWindow(fg, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = { sizeof(mi) };
        if (GetMonitorInfoW(hMon, &mi)) {
            if (rc.left <= mi.rcMonitor.left && rc.top <= mi.rcMonitor.top &&
                rc.right >= mi.rcMonitor.right && rc.bottom >= mi.rcMonitor.bottom) {
                LONG style = GetWindowLongW(fg, GWL_STYLE);
                // Borderless popup, or window without titlebar caption
                if ((style & WS_VISIBLE) && ((style & WS_POPUP) || !(style & WS_CAPTION))) {
                    return true;
                }
            }
        }
    }

    return false;
}

LRESULT CALLBACK MessageWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TIMER && wParam == TIMER_GAME_DETECTOR) {
        bool gameActive = IsFullscreenGameRunning();
        if (gameActive && !g_app.isSuspended) {
            // Fullscreen game entered! Suspend all rendering, animation & timers
            g_app.isSuspended = true;
            if (g_app.drawer && g_app.drawer->IsVisible()) {
                g_app.drawer->Show(false);
            }
            if (g_app.dock) {
                g_app.dock->Suspend();
            }
            if (g_app.widgetHost) {
                g_app.widgetHost->Suspend();
            }
            // Release memory working set back to the OS
            SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
        } else if (!gameActive && g_app.isSuspended) {
            // Game closed or switched back to desktop! Resume all widgets and dock smoothly
            g_app.isSuspended = false;
            if (g_app.dock) {
                g_app.dock->Resume();
            }
            if (g_app.widgetHost) {
                g_app.widgetHost->Resume();
            }
            // Trim working set after resuming
            SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
        }
        return 0;
    }

    if (msg == WM_TRAYICON) {
        if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_CONTEXTMENU) {
            POINT pt;
            GetCursorPos(&pt);

            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_SETTINGS, L"⚙️  系统偏好设置...");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"退出程序");

            SetForegroundWindow(hwnd);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(hMenu);

            if (cmd == ID_TRAY_SETTINGS) {
                if (g_app.settingsWindow) {
                    g_app.settingsWindow->Show(true);
                }
            } else if (cmd == ID_TRAY_EXIT) {
                PostQuitMessage(0);
            }
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    // 0. Initialize native Windows dark mode for app & menus
    Core::InitializeDarkMode();

    // 1. Initialize COM for Direct2D & WIC imaging
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return 1;

    // 2. Create message-only window for system tray handling and game detection
    const wchar_t TRAY_CLASS[] = L"TrayMessageReceiverClass";
    WNDCLASSEXW twc = {};
    twc.cbSize = sizeof(twc);
    twc.lpfnWndProc = MessageWndProc;
    twc.hInstance = hInstance;
    twc.lpszClassName = TRAY_CLASS;
    RegisterClassExW(&twc);

    g_hMsgHwnd = CreateWindowExW(0, TRAY_CLASS, L"TrayMsg", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);
    if (g_hMsgHwnd) {
        BOOL dark = TRUE;
        DwmSetWindowAttribute(g_hMsgHwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    }

    // 3. Register System Tray Icon
    HICON hAppIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    if (!hAppIcon) {
        hAppIcon = (HICON)LoadImageW(nullptr, L"app.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
    }
    if (!hAppIcon) {
        hAppIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }

    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = g_hMsgHwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = hAppIcon;
    std::wstring tipText = L"Macify v" + std::wstring(APP_VERSION_WSTRING) + L" (智能省电/游戏休眠)";
    wcscpy_s(g_nid.szTip, tipText.c_str());
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    // 4. Load centralized configurations
    wchar_t exePathBuf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH);
    std::wstring exeDir = exePathBuf;
    size_t lastSlash = exeDir.find_last_of(L"\\/");
    std::wstring configPath = (lastSlash != std::wstring::npos) ? (exeDir.substr(0, lastSlash) + L"\\config.json") : L"config.json";

    auto& cfg = Settings::SettingsManager::Instance().Config();
    cfg.LoadFromFile(configPath);

    // 5. Create and initialize Settings Window
    Settings::SettingsWindow settingsWindow;
    settingsWindow.Create();
    settingsWindow.SyncConfig(cfg);
    g_app.settingsWindow = &settingsWindow;

    // 6. Create and initialize Launchpad Drawer Window
    Drawer::DrawerWindow drawer;
    drawer.Create();

    // 7. Create and initialize Dock Window
    Dock::DockWindow dock;
    if (dock.Create()) {
        dock.SetOnToggleLaunchpad([&drawer]() {
            drawer.Toggle();
        });
        dock.SetIsLaunchpadOpenCallback([&drawer]() {
            return drawer.IsVisible();
        });
        dock.SetOnOpenSettings([&settingsWindow]() {
            settingsWindow.Show(true);
        });

        // Load persisted pinned apps from config.json (or load defaults)
        dock.LoadConfig();
        dock.Show(true);

        // 7. Initialize DragOverlay for global cross-window drag preview
        static Core::DragOverlay dragOverlay;
        dragOverlay.Create(hInstance);

        // Wire drag and drop from Launchpad to Dock
        drawer.SetOnDragStart([](const std::wstring& name, const std::wstring& targetPath, const std::wstring& iconPath, int screenX, int screenY) {
            std::wstring pathToLoad = !iconPath.empty() ? iconPath : targetPath;
            dragOverlay.StartDrag(name, pathToLoad, screenX, screenY);
        });

        drawer.SetOnDragMove([&dock](int screenX, int screenY) {
            dragOverlay.UpdateDrag(screenX, screenY);
            dock.OnExternalDragMove(screenX, screenY);
        });

        drawer.SetOnDragEnd([&dock]() {
            dragOverlay.EndDrag();
            dock.OnExternalDragLeave();
        });

        drawer.SetOnOpenSettings([&settingsWindow]() {
            settingsWindow.Show(true);
        });

        drawer.SetOnPinToDock([&dock](const std::wstring& name, const std::wstring& path) {
            dock.PinApp(name, path);
        });

        drawer.SetOnDropApp([&dock, &drawer](const std::wstring& name, const std::wstring& path, int screenX, int screenY) {
            dragOverlay.EndDrag();
            RECT dockRect;
            GetWindowRect(dock.GetHwnd(), &dockRect);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            bool isNearBottom = (screenY >= dockRect.top - 60 && screenY <= screenHeight);
            bool isHorzOver = (screenX >= dockRect.left - 50 && screenX <= dockRect.right + 50);

            if (isNearBottom && isHorzOver) {
                int insertIdx = dock.GetInsertIndex();
                dock.PinAppAtIndex(name, path, insertIdx);
                drawer.Show(false);
            } else {
                dock.OnExternalDragLeave();
            }
        });
    }

    // 8. Create and initialize Desktop Widgets Host
    Widgets::WidgetHostWindow widgetHost;
    if (widgetHost.Create()) {
        widgetHost.AddWidget(std::make_unique<Widgets::ClockWidget>(cfg.widgetClockX, cfg.widgetClockY, cfg.widgetClockW, cfg.widgetClockH));
        widgetHost.AddWidget(std::make_unique<Widgets::DiskWidget>(cfg.widgetDiskX, cfg.widgetDiskY, cfg.widgetDiskW, cfg.widgetDiskH));
        widgetHost.AddWidget(std::make_unique<Widgets::SysMonitorWidget>(cfg.widgetSysMonX, cfg.widgetSysMonY, cfg.widgetSysMonW, cfg.widgetSysMonH));
        widgetHost.AddWidget(std::make_unique<Widgets::CalendarWidget>(cfg.widgetCalendarX, cfg.widgetCalendarY, cfg.widgetCalendarW, cfg.widgetCalendarH));
        widgetHost.AddWidget(std::make_unique<Widgets::MemoWidget>(cfg.widgetMemoX, cfg.widgetMemoY, cfg.widgetMemoW, cfg.widgetMemoH));

        widgetHost.SetOnBoundsChanged([&cfg, configPath](const std::wstring& id, float x, float y, float w, float h) {
            cfg.UpdateWidgetBounds(id, x, y, w, h);
            cfg.SaveToFile(configPath);
        });

        widgetHost.Show(true);
    }

    // 9. Wire live synchronization callbacks for Settings Window
    Settings::SettingsCallbacks scb;
    scb.onDockChanged = [&dock](const Settings::AppConfig& c) {
        dock.ApplySettings(c.dockBaseSize, c.dockMaxSize, c.dockSpread, c.dockBottomMargin,
                           c.dockBarVisualHeight, c.dockBgOpacity, c.dockShowRunningDot,
                           c.dockShowTooltip, c.dockAutoHide);
    };
    scb.onDrawerChanged = [&drawer](const Settings::AppConfig& c) {
        drawer.ApplySettings(c.drawerColumns, c.drawerIconSize, c.drawerAutoCloseOnBlur, c.drawerSearchAutoFocus);
    };
    scb.onWidgetsChanged = [&widgetHost](const Settings::AppConfig& c) {
        widgetHost.ApplySettings(c.widgetClockEnabled, c.widgetClock24Hour, c.widgetClockShowSeconds, c.widgetClockShowDate,
                                 c.widgetDiskEnabled, static_cast<float>(c.widgetDiskRefreshMin * 60.0f),
                                 c.widgetSysMonEnabled, c.widgetCalendarEnabled, c.widgetMemoEnabled,
                                 c.widgetLockPosition);
        widgetHost.SyncWidgetBounds(c.widgetClockX, c.widgetClockY, c.widgetClockW, c.widgetClockH,
                                    c.widgetDiskX, c.widgetDiskY, c.widgetDiskW, c.widgetDiskH,
                                    c.widgetSysMonX, c.widgetSysMonY, c.widgetSysMonW, c.widgetSysMonH,
                                    c.widgetCalendarX, c.widgetCalendarY, c.widgetCalendarW, c.widgetCalendarH,
                                    c.widgetMemoX, c.widgetMemoY, c.widgetMemoW, c.widgetMemoH);
    };
    scb.onAppearanceChanged = [&dock, &drawer, &widgetHost](const Settings::AppConfig& c) {
        auto p = Settings::GetThemePalette(c.themePreset, c.accentColor, c.dockBgOpacity, c.widgetOpacity);
        dock.ApplyTheme(p.dockBg, p.dockBorder, c.cornerRadius);
        drawer.ApplyTheme(p.drawerBg, p.drawerBorder, c.cornerRadius);
        widgetHost.ApplyTheme(p.widgetBg, p.widgetBorder, p.accent);
    };
    scb.onSystemChanged = [](const Settings::AppConfig& c) {
        Settings::AppConfig::ApplyTaskbarVisibility(c.hideWindowsTaskbar);
        Settings::AppConfig::SetRegistryAutoStart(c.autoStart);
    };
    scb.onReloadApps = [&drawer]() {
        drawer.ReloadApps();
    };
    scb.onAddAppsFromExplorer = [&drawer]() {
        drawer.AddCustomAppsFromDialog();
    };
    scb.onRestartApp = []() {
        wchar_t szExe[MAX_PATH];
        GetModuleFileNameW(nullptr, szExe, MAX_PATH);
        ShellExecuteW(nullptr, L"open", szExe, nullptr, nullptr, SW_SHOWNORMAL);
        PostQuitMessage(0);
    };
    settingsWindow.SetCallbacks(scb);

    // Initial settings application
    dock.ApplySettings(cfg.dockBaseSize, cfg.dockMaxSize, cfg.dockSpread, cfg.dockBottomMargin,
                       cfg.dockBarVisualHeight, cfg.dockBgOpacity, cfg.dockShowRunningDot,
                       cfg.dockShowTooltip, cfg.dockAutoHide);
    drawer.ApplySettings(cfg.drawerColumns, cfg.drawerIconSize, cfg.drawerAutoCloseOnBlur, cfg.drawerSearchAutoFocus);
    widgetHost.ApplySettings(cfg.widgetClockEnabled, cfg.widgetClock24Hour, cfg.widgetClockShowSeconds, cfg.widgetClockShowDate,
                             cfg.widgetDiskEnabled, static_cast<float>(cfg.widgetDiskRefreshMin * 60.0f),
                             cfg.widgetSysMonEnabled, cfg.widgetCalendarEnabled, cfg.widgetMemoEnabled,
                             cfg.widgetLockPosition);
    
    // Apply initial theme and appearance across all modules
    auto p = Settings::GetThemePalette(cfg.themePreset, cfg.accentColor, cfg.dockBgOpacity, cfg.widgetOpacity);
    dock.ApplyTheme(p.dockBg, p.dockBorder, cfg.cornerRadius);
    drawer.ApplyTheme(p.drawerBg, p.drawerBorder, cfg.cornerRadius);
    widgetHost.ApplyTheme(p.widgetBg, p.widgetBorder, p.accent);

    if (cfg.hideWindowsTaskbar) {
        Settings::AppConfig::ApplyTaskbarVisibility(true);
    }

    // Store references in g_app context for Game Mode auto-suspend/resume
    g_app.dock = &dock;
    g_app.widgetHost = &widgetHost;
    g_app.drawer = &drawer;

    // Start 1.5s Game Mode detector timer
    SetTimer(g_hMsgHwnd, TIMER_GAME_DETECTOR, GAME_CHECK_INTERVAL_MS, nullptr);

    // Trim working set after initialization to keep resident memory ultra-lean
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);

    // 10. Windows Message Loop
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 11. Cleanup & restore
    if (cfg.hideWindowsTaskbar) {
        Settings::AppConfig::ApplyTaskbarVisibility(false);
    }
    KillTimer(g_hMsgHwnd, TIMER_GAME_DETECTOR);
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    CoUninitialize();

    return 0;
}
