#pragma once
#include <string>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <d2d1.h>
#include "../widgets/MemoWidget.h"

namespace Settings {

struct ThemePalette {
    D2D1_COLOR_F windowBg;
    D2D1_COLOR_F sidebarBg;
    D2D1_COLOR_F cardBg;
    D2D1_COLOR_F border;
    D2D1_COLOR_F dockBg;
    D2D1_COLOR_F dockBorder;
    D2D1_COLOR_F drawerBg;
    D2D1_COLOR_F drawerBorder;
    D2D1_COLOR_F widgetBg;
    D2D1_COLOR_F widgetBorder;
    D2D1_COLOR_F accent;
};

inline ThemePalette GetThemePalette(int themePreset, int accentColorIdx, float dockOpacity, float widgetOpacity) {
    ThemePalette p;
    // Accent color: 0: 经典蓝, 1: 翡翠绿, 2: 紫罗兰, 3: 珊瑚红
    switch (accentColorIdx) {
        case 1: p.accent = D2D1::ColorF(0.19f, 0.82f, 0.35f, 1.0f); break; // Emerald Green
        case 2: p.accent = D2D1::ColorF(0.69f, 0.32f, 0.87f, 1.0f); break; // Violet Purple
        case 3: p.accent = D2D1::ColorF(1.00f, 0.23f, 0.19f, 1.0f); break; // Coral Red
        default: p.accent = D2D1::ColorF(0.00f, 0.48f, 1.00f, 1.0f); break; // Classic Blue
    }

    switch (themePreset) {
        case 1: // 太空灰 (Space Gray - sleek metallic graphite)
            p.windowBg = D2D1::ColorF(0.15f, 0.16f, 0.18f, 0.98f);
            p.sidebarBg = D2D1::ColorF(0.12f, 0.13f, 0.15f, 0.98f);
            p.cardBg = D2D1::ColorF(0.20f, 0.21f, 0.24f, 0.92f);
            p.border = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.16f);
            p.dockBg = D2D1::ColorF(0.18f, 0.19f, 0.22f, dockOpacity);
            p.dockBorder = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.22f);
            p.drawerBg = D2D1::ColorF(0.16f, 0.17f, 0.20f, 0.94f);
            p.drawerBorder = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.20f);
            p.widgetBg = D2D1::ColorF(0.16f, 0.17f, 0.20f, widgetOpacity);
            p.widgetBorder = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.20f);
            break;

        case 2: // 冰晶玻璃 (Ice Glass - airy crystal acrylic tint)
            p.windowBg = D2D1::ColorF(0.14f, 0.18f, 0.24f, 0.96f);
            p.sidebarBg = D2D1::ColorF(0.10f, 0.14f, 0.20f, 0.96f);
            p.cardBg = D2D1::ColorF(0.20f, 0.26f, 0.35f, 0.75f);
            p.border = D2D1::ColorF(0.70f, 0.88f, 1.00f, 0.28f);
            p.dockBg = D2D1::ColorF(0.20f, 0.28f, 0.38f, dockOpacity * 0.85f);
            p.dockBorder = D2D1::ColorF(0.70f, 0.88f, 1.00f, 0.35f);
            p.drawerBg = D2D1::ColorF(0.12f, 0.18f, 0.26f, 0.90f);
            p.drawerBorder = D2D1::ColorF(0.65f, 0.85f, 1.00f, 0.32f);
            p.widgetBg = D2D1::ColorF(0.14f, 0.22f, 0.32f, widgetOpacity * 0.85f);
            p.widgetBorder = D2D1::ColorF(0.65f, 0.85f, 1.00f, 0.32f);
            break;

        case 3: // 极光紫 (Aurora Purple - deep mystical cosmic violet)
            p.windowBg = D2D1::ColorF(0.13f, 0.09f, 0.20f, 0.98f);
            p.sidebarBg = D2D1::ColorF(0.09f, 0.06f, 0.15f, 0.98f);
            p.cardBg = D2D1::ColorF(0.19f, 0.13f, 0.28f, 0.88f);
            p.border = D2D1::ColorF(0.75f, 0.55f, 0.95f, 0.24f);
            p.dockBg = D2D1::ColorF(0.16f, 0.10f, 0.24f, dockOpacity);
            p.dockBorder = D2D1::ColorF(0.80f, 0.55f, 1.00f, 0.30f);
            p.drawerBg = D2D1::ColorF(0.14f, 0.09f, 0.22f, 0.94f);
            p.drawerBorder = D2D1::ColorF(0.80f, 0.55f, 1.00f, 0.28f);
            p.widgetBg = D2D1::ColorF(0.15f, 0.10f, 0.24f, widgetOpacity);
            p.widgetBorder = D2D1::ColorF(0.75f, 0.55f, 0.95f, 0.28f);
            break;

        case 0: // 深邃黑 (Deep Black - Apple dark mode obsidian)
        default:
            p.windowBg = D2D1::ColorF(0.09f, 0.10f, 0.12f, 0.98f);
            p.sidebarBg = D2D1::ColorF(0.06f, 0.07f, 0.08f, 0.98f);
            p.cardBg = D2D1::ColorF(0.13f, 0.14f, 0.17f, 0.90f);
            p.border = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.12f);
            p.dockBg = D2D1::ColorF(0.10f, 0.11f, 0.14f, dockOpacity);
            p.dockBorder = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.16f);
            p.drawerBg = D2D1::ColorF(0.10f, 0.11f, 0.14f, 0.94f);
            p.drawerBorder = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.16f);
            p.widgetBg = D2D1::ColorF(0.08f, 0.09f, 0.12f, widgetOpacity);
            p.widgetBorder = D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.18f);
            break;
    }
    return p;
}

struct PinnedAppConfig {
    std::wstring name;
    std::wstring path;
    std::wstring action; // "launchpad", "launch", "recyclebin", "settings"
    std::wstring icon;
};

struct AppConfig {
    AppConfig() {
        InitWidgetDefaults();
    }

    void InitWidgetDefaults() {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        if (screenW <= 0) screenW = 1920;
        float defaultX = static_cast<float>(screenW) - 220.0f - 25.0f;
        float curY = 25.0f;

        widgetClockX = defaultX;
        widgetClockY = curY;
        widgetClockW = 220.0f;
        widgetClockH = 90.0f;
        curY += widgetClockH + 10.0f;

        widgetDiskX = defaultX;
        widgetDiskY = curY;
        widgetDiskW = 220.0f;
        widgetDiskH = 156.0f; // Standard default accommodates 4 disks cleanly
        curY += widgetDiskH + 10.0f;

        widgetSysMonX = defaultX;
        widgetSysMonY = curY;
        widgetSysMonW = 220.0f;
        widgetSysMonH = 115.0f;
        curY += widgetSysMonH + 10.0f;

        widgetCalendarX = defaultX;
        widgetCalendarY = curY;
        widgetCalendarW = 220.0f;
        widgetCalendarH = 165.0f;
        curY += widgetCalendarH + 10.0f;

        widgetMemoX = defaultX;
        widgetMemoY = curY;
        widgetMemoW = 220.0f;
        widgetMemoH = 135.0f;

        if (memoItems.empty()) {
            memoItems = {
                { L"保持专注，优雅从容", false },
                { L"享受纯净美观的桌面空间", false },
                { L"Macify 开源桌面套件", true }
            };
        }
    }

    // 1. 常规与系统 (General & System)
    bool autoStart = false;
    bool hideWindowsTaskbar = false;
    bool gameModeSuspend = true;
    bool showTrayIcon = true;
    bool notifyOnAppDrop = true;

    // 2. 底部 Dock 栏精细控制 (Dock Bar)
    float dockBaseSize = 58.0f;
    float dockMaxSize = 92.0f;
    float dockSpread = 155.0f;
    float dockBottomMargin = 36.0f;
    float dockBarVisualHeight = 78.0f;
    float dockBgOpacity = 0.80f;
    bool dockShowRunningDot = true;
    bool dockShowTooltip = true;
    bool dockAutoHide = true;
    int dockSlideSpeed = 1; // 0: 极速, 1: 标准平滑, 2: 缓慢优雅
    std::vector<PinnedAppConfig> pinnedApps;

    // 3. 应用启动台 (Launchpad Drawer)
    int drawerColumns = 7;
    float drawerIconSize = 58.0f;
    bool drawerAutoCloseOnBlur = true;
    bool drawerSearchAutoFocus = true;
    int drawerBlurMode = 0; // 0: 深邃磨砂, 1: 透明亚克力, 2: 云母 Mica
    std::vector<std::pair<std::wstring, std::wstring>> customApps; // { name, path }
    std::vector<std::wstring> hiddenApps; // App names or targetPaths permanently hidden from Launchpad

    bool IsAppHidden(const std::wstring& name, const std::wstring& path) const {
        for (const auto& h : hiddenApps) {
            if (!h.empty()) {
                if (!name.empty() && h == name) return true;
                if (!path.empty() && h == path) return true;
            }
        }
        return false;
    }

    void HideApp(const std::wstring& name, const std::wstring& path) {
        if (!name.empty() && !IsAppHidden(name, L"")) {
            hiddenApps.push_back(name);
        }
        if (!path.empty() && !IsAppHidden(L"", path)) {
            hiddenApps.push_back(path);
        }
    }

    void RestoreHiddenApps() {
        hiddenApps.clear();
    }

    // 4. 桌面小组件 (Desktop Widgets)
    bool widgetClockEnabled = true;
    bool widgetClock24Hour = true;
    bool widgetClockShowSeconds = true;
    bool widgetClockShowDate = true;
    float widgetClockX = 40.0f;
    float widgetClockY = 40.0f;
    float widgetClockW = 220.0f;
    float widgetClockH = 90.0f;

    bool widgetDiskEnabled = true;
    int widgetDiskRefreshMin = 15; // 10 - 30 minutes
    float widgetDiskX = 40.0f;
    float widgetDiskY = 145.0f;
    float widgetDiskW = 220.0f;
    float widgetDiskH = 115.0f;

    bool widgetSysMonEnabled = true;
    float widgetSysMonX = 40.0f;
    float widgetSysMonY = 275.0f;
    float widgetSysMonW = 220.0f;
    float widgetSysMonH = 115.0f;

    bool widgetCalendarEnabled = true;
    float widgetCalendarX = 40.0f;
    float widgetCalendarY = 405.0f;
    float widgetCalendarW = 220.0f;
    float widgetCalendarH = 165.0f;

    bool widgetMemoEnabled = false;
    float widgetMemoX = 40.0f;
    float widgetMemoY = 585.0f;
    float widgetMemoW = 220.0f;
    float widgetMemoH = 135.0f;
    std::vector<Widgets::MemoItem> memoItems;

    bool widgetLockPosition = false;
    float widgetOpacity = 0.85f;

    void UpdateWidgetBounds(const std::wstring& id, float x, float y, float w, float h) {
        if (id == L"widget_clock") {
            widgetClockX = x; widgetClockY = y; widgetClockW = w; widgetClockH = h;
        } else if (id == L"widget_disk") {
            widgetDiskX = x; widgetDiskY = y; widgetDiskW = w; widgetDiskH = h;
        } else if (id == L"widget_sysmon") {
            widgetSysMonX = x; widgetSysMonY = y; widgetSysMonW = w; widgetSysMonH = h;
        } else if (id == L"widget_calendar") {
            widgetCalendarX = x; widgetCalendarY = y; widgetCalendarW = w; widgetCalendarH = h;
        } else if (id == L"widget_memo") {
            widgetMemoX = x; widgetMemoY = y; widgetMemoW = w; widgetMemoH = h;
        }
    }

    int widgetSizePreset = 1; // 0: 200px (紧凑), 1: 220px (标准), 2: 260px (宽屏), 3: 300px (超大)

    void ApplyWidgetSizePreset(int preset) {
        widgetSizePreset = std::clamp(preset, 0, 3);
        float targetW = 220.0f;
        switch (widgetSizePreset) {
            case 0: targetW = 200.0f; break;
            case 1: targetW = 220.0f; break;
            case 2: targetW = 260.0f; break;
            case 3: targetW = 300.0f; break;
        }

        float ratio = targetW / 220.0f;
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        if (screenW <= 0) screenW = 1920;
        float newX = static_cast<float>(screenW) - targetW - 25.0f;

        // Proportional heights for all widgets based on their 220px base dimensions
        float clockBaseH = 90.0f;
        float diskBaseH = 156.0f;
        float sysMonBaseH = 115.0f;
        float calBaseH = 165.0f;
        float memoBaseH = 135.0f;

        widgetClockW = targetW; widgetClockH = std::round(clockBaseH * ratio);
        widgetDiskW = targetW; widgetDiskH = std::round(diskBaseH * ratio);
        widgetSysMonW = targetW; widgetSysMonH = std::round(sysMonBaseH * ratio);
        widgetCalendarW = targetW; widgetCalendarH = std::round(calBaseH * ratio);
        widgetMemoW = targetW; widgetMemoH = std::round(memoBaseH * ratio);

        // Reposition sequentially from top-right with 10px spacing
        float curY = 25.0f;
        float gap = 10.0f;

        widgetClockX = newX; widgetClockY = curY;
        if (widgetClockEnabled) curY += widgetClockH + gap;

        widgetDiskX = newX; widgetDiskY = curY;
        if (widgetDiskEnabled) curY += widgetDiskH + gap;

        widgetSysMonX = newX; widgetSysMonY = curY;
        if (widgetSysMonEnabled) curY += widgetSysMonH + gap;

        widgetCalendarX = newX; widgetCalendarY = curY;
        if (widgetCalendarEnabled) curY += widgetCalendarH + gap;

        widgetMemoX = newX; widgetMemoY = curY;
    }

    void ResetWidgetLayout() {
        widgetSizePreset = 1;
        InitWidgetDefaults();
    }

    // 5. 个性化主题与外观 (Themes & Appearance)
    int themePreset = 0; // 0: 深邃夜空黑, 1: 太空灰, 2: 纯净磨砂水晶, 3: 极光幻紫
    int accentColor = 0; // 0: Mac经典蓝 (#0A84FF), 1: 翡翠绿 (#30D158), 2: 霓虹紫 (#BF5AF2), 3: 珊瑚红 (#FF453A)
    float cornerRadius = 24.0f;

    // 获取系统注册表开机自启状态
    static bool CheckRegistryAutoStart() {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            wchar_t path[MAX_PATH] = {};
            DWORD size = sizeof(path);
            DWORD type = REG_SZ;
            LONG res = RegQueryValueExW(hKey, L"MacDesktopBeauty", nullptr, &type, (LPBYTE)path, &size);
            RegCloseKey(hKey);
            return (res == ERROR_SUCCESS);
        }
        return false;
    }

    // 设置系统注册表开机自启
    static void SetRegistryAutoStart(bool enable) {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            if (enable) {
                wchar_t exePath[MAX_PATH] = {};
                GetModuleFileNameW(nullptr, exePath, MAX_PATH);
                std::wstring cmd = L"\"" + std::wstring(exePath) + L"\"";
                RegSetValueExW(hKey, L"MacDesktopBeauty", 0, REG_SZ, (const BYTE*)cmd.c_str(), (DWORD)((cmd.length() + 1) * sizeof(wchar_t)));
            } else {
                RegDeleteValueW(hKey, L"MacDesktopBeauty");
            }
            RegCloseKey(hKey);
        }
    }

    // 隐藏/恢复 Windows 原生任务栏
    static void ApplyTaskbarVisibility(bool hide) {
        HWND hTaskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
        if (hTaskbar) {
            ShowWindow(hTaskbar, hide ? SW_HIDE : SW_SHOW);
        }
        HWND hSecondary = FindWindowW(L"Shell_SecondaryTrayWnd", nullptr);
        if (hSecondary) {
            ShowWindow(hSecondary, hide ? SW_HIDE : SW_SHOW);
        }
    }

    void ResetToDefaults() {
        autoStart = false;
        hideWindowsTaskbar = false;
        gameModeSuspend = true;
        showTrayIcon = true;
        notifyOnAppDrop = true;

        dockBaseSize = 58.0f;
        dockMaxSize = 92.0f;
        dockSpread = 155.0f;
        dockBottomMargin = 36.0f;
        dockBarVisualHeight = 78.0f;
        dockBgOpacity = 0.80f;
        dockShowRunningDot = true;
        dockShowTooltip = true;
        dockAutoHide = true;
        dockSlideSpeed = 1;
        pinnedApps = {
            { L"启动台", L"", L"launchpad", L"launchpad" },
            { L"资源管理器", L"C:\\Windows\\explorer.exe", L"launch", L"" },
            { L"回收站", L"shell:RecycleBinFolder", L"recyclebin", L"" },
            { L"设置", L"", L"settings", L"settings" },
            { L"关机", L"", L"power", L"power" }
        };

        drawerColumns = 7;
        drawerIconSize = 58.0f;
        drawerAutoCloseOnBlur = true;
        drawerSearchAutoFocus = true;
        drawerBlurMode = 0;
        customApps.clear();
        hiddenApps.clear();

        InitWidgetDefaults();

        widgetClockEnabled = true;
        widgetClock24Hour = true;
        widgetClockShowSeconds = true;
        widgetClockShowDate = true;

        widgetDiskEnabled = true;
        widgetDiskRefreshMin = 15;

        widgetSysMonEnabled = true;
        widgetCalendarEnabled = true;
        widgetMemoEnabled = false;
        memoItems = {
            { L"保持专注，优雅从容", false },
            { L"享受纯净美观的桌面空间", false },
            { L"Macify 开源桌面套件", true }
        };

        widgetLockPosition = false;
        widgetOpacity = 0.85f;
        widgetSizePreset = 1;

        themePreset = 0;
        accentColor = 0;
        cornerRadius = 24.0f;
    }

    void LoadFromFile(const std::wstring& filePath) {
        std::ifstream file(filePath.c_str());
        if (!file.is_open()) return;

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        auto parseBool = [&content](const std::string& key, bool defaultVal) -> bool {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == std::string::npos) return defaultVal;
            size_t colon = content.find(':', pos);
            if (colon == std::string::npos) return defaultVal;
            size_t comma = content.find_first_of(",}\n", colon);
            std::string val = content.substr(colon + 1, comma - colon - 1);
            if (val.find("true") != std::string::npos) return true;
            if (val.find("false") != std::string::npos) return false;
            return defaultVal;
        };

        auto parseFloat = [&content](const std::string& key, float defaultVal) -> float {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == std::string::npos) return defaultVal;
            size_t colon = content.find(':', pos);
            if (colon == std::string::npos) return defaultVal;
            size_t comma = content.find_first_of(",}\n", colon);
            std::string val = content.substr(colon + 1, comma - colon - 1);
            try {
                return std::stof(val);
            } catch (...) {
                return defaultVal;
            }
        };

        auto parseInt = [&content](const std::string& key, int defaultVal) -> int {
            size_t pos = content.find("\"" + key + "\"");
            if (pos == std::string::npos) return defaultVal;
            size_t colon = content.find(':', pos);
            if (colon == std::string::npos) return defaultVal;
            size_t comma = content.find_first_of(",}\n", colon);
            std::string val = content.substr(colon + 1, comma - colon - 1);
            try {
                return std::stoi(val);
            } catch (...) {
                return defaultVal;
            }
        };

        // 1. General
        autoStart = CheckRegistryAutoStart();
        hideWindowsTaskbar = parseBool("hideWindowsTaskbar", hideWindowsTaskbar);
        gameModeSuspend = parseBool("gameModeSuspend", gameModeSuspend);
        showTrayIcon = parseBool("showTrayIcon", showTrayIcon);
        notifyOnAppDrop = parseBool("notifyOnAppDrop", notifyOnAppDrop);

        // 2. Dock
        dockBaseSize = parseFloat("baseIconSize", dockBaseSize);
        dockMaxSize = parseFloat("maxIconSize", dockMaxSize);
        dockSpread = parseFloat("spread", dockSpread);
        dockBottomMargin = parseFloat("bottomMargin", dockBottomMargin);
        dockBarVisualHeight = parseFloat("barVisualHeight", dockBarVisualHeight);
        dockBgOpacity = parseFloat("bgOpacity", dockBgOpacity);
        dockShowRunningDot = parseBool("showRunningDot", dockShowRunningDot);
        dockShowTooltip = parseBool("showTooltip", dockShowTooltip);
        dockAutoHide = parseBool("autoHide", dockAutoHide);
        dockSlideSpeed = parseInt("slideSpeed", dockSlideSpeed);

        size_t pinnedPos = content.find("\"pinnedApps\"");
        if (pinnedPos != std::string::npos) {
            size_t openBracket = content.find('[', pinnedPos);
            size_t closeBracket = content.find(']', openBracket);
            if (openBracket != std::string::npos && closeBracket != std::string::npos) {
                pinnedApps.clear();
                std::string arrStr = content.substr(openBracket, closeBracket - openBracket + 1);
                size_t p = 0;
                while ((p = arrStr.find('{', p)) != std::string::npos) {
                    size_t endObj = arrStr.find('}', p);
                    if (endObj == std::string::npos) break;
                    std::string obj = arrStr.substr(p, endObj - p + 1);

                    auto extractVal = [&obj](const std::string& key) -> std::string {
                        size_t kpos = obj.find("\"" + key + "\"");
                        if (kpos == std::string::npos) return "";
                        size_t col = obj.find(':', kpos);
                        if (col == std::string::npos) return "";
                        size_t q1 = obj.find('"', col);
                        if (q1 == std::string::npos) return "";
                        size_t q2 = obj.find('"', q1 + 1);
                        if (q2 == std::string::npos) return "";
                        return obj.substr(q1 + 1, q2 - q1 - 1);
                    };

                    auto toWString = [](const std::string& s) -> std::wstring {
                        std::string unescaped;
                        for (size_t i = 0; i < s.length(); ++i) {
                            if (s[i] == '\\' && i + 1 < s.length() && s[i+1] == '\\') {
                                unescaped += '\\';
                                ++i;
                            } else {
                                unescaped += s[i];
                            }
                        }
                        int wlen = MultiByteToWideChar(CP_UTF8, 0, unescaped.c_str(), -1, nullptr, 0);
                        if (wlen > 0) {
                            std::wstring res(wlen, 0);
                            MultiByteToWideChar(CP_UTF8, 0, unescaped.c_str(), -1, &res[0], wlen);
                            if (!res.empty() && res.back() == L'\0') res.pop_back();
                            return res;
                        }
                        return L"";
                    };

                    PinnedAppConfig item;
                    item.name = toWString(extractVal("name"));
                    item.path = toWString(extractVal("path"));
                    item.action = toWString(extractVal("action"));
                    item.icon = toWString(extractVal("icon"));
                    if (!item.name.empty() || !item.path.empty() || !item.action.empty()) {
                        pinnedApps.push_back(item);
                    }
                    p = endObj + 1;
                }
            }
        }
        if (pinnedApps.empty()) {
            pinnedApps = {
                { L"启动台", L"", L"launchpad", L"launchpad" },
                { L"资源管理器", L"C:\\Windows\\explorer.exe", L"launch", L"" },
                { L"回收站", L"shell:RecycleBinFolder", L"recyclebin", L"" },
                { L"设置", L"", L"settings", L"settings" },
                { L"关机", L"", L"power", L"power" }
            };
        }

        // 3. Launchpad
        drawerColumns = parseInt("drawerColumns", drawerColumns);
        drawerIconSize = parseFloat("drawerIconSize", drawerIconSize);
        drawerAutoCloseOnBlur = parseBool("drawerAutoCloseOnBlur", drawerAutoCloseOnBlur);
        drawerSearchAutoFocus = parseBool("drawerSearchAutoFocus", drawerSearchAutoFocus);
        drawerBlurMode = parseInt("drawerBlurMode", drawerBlurMode);

        customApps.clear();
        size_t customPos = content.find("\"customApps\"");
        if (customPos != std::string::npos) {
            size_t openBracket = content.find('[', customPos);
            size_t closeBracket = content.find(']', openBracket);
            if (openBracket != std::string::npos && closeBracket != std::string::npos) {
                std::string arrStr = content.substr(openBracket, closeBracket - openBracket + 1);
                size_t p = 0;
                while ((p = arrStr.find('{', p)) != std::string::npos) {
                    size_t endObj = arrStr.find('}', p);
                    if (endObj == std::string::npos) break;
                    std::string obj = arrStr.substr(p, endObj - p + 1);
                    
                    auto extractVal = [&obj](const std::string& key) -> std::string {
                        size_t kpos = obj.find("\"" + key + "\"");
                        if (kpos == std::string::npos) return "";
                        size_t col = obj.find(':', kpos);
                        if (col == std::string::npos) return "";
                        size_t q1 = obj.find('"', col);
                        if (q1 == std::string::npos) return "";
                        size_t q2 = obj.find('"', q1 + 1);
                        if (q2 == std::string::npos) return "";
                        return obj.substr(q1 + 1, q2 - q1 - 1);
                    };

                    std::string nameUtf8 = extractVal("name");
                    std::string pathUtf8 = extractVal("path");
                    if (!pathUtf8.empty()) {
                        std::string unescapedPath;
                        for (size_t i = 0; i < pathUtf8.length(); ++i) {
                            if (pathUtf8[i] == '\\' && i + 1 < pathUtf8.length() && pathUtf8[i+1] == '\\') {
                                unescapedPath += '\\';
                                ++i;
                            } else {
                                unescapedPath += pathUtf8[i];
                            }
                        }

                        int wlen = MultiByteToWideChar(CP_UTF8, 0, unescapedPath.c_str(), -1, nullptr, 0);
                        std::wstring wpath(wlen, 0);
                        MultiByteToWideChar(CP_UTF8, 0, unescapedPath.c_str(), -1, &wpath[0], wlen);
                        if (!wpath.empty() && wpath.back() == L'\0') wpath.pop_back();

                        int wnamelen = MultiByteToWideChar(CP_UTF8, 0, nameUtf8.c_str(), -1, nullptr, 0);
                        std::wstring wname(wnamelen, 0);
                        MultiByteToWideChar(CP_UTF8, 0, nameUtf8.c_str(), -1, &wname[0], wnamelen);
                        if (!wname.empty() && wname.back() == L'\0') wname.pop_back();

                        customApps.push_back({ wname, wpath });
                    }
                    p = endObj + 1;
                }
            }
        }

        hiddenApps.clear();
        size_t hiddenPos = content.find("\"hiddenApps\"");
        if (hiddenPos != std::string::npos) {
            size_t openBracket = content.find('[', hiddenPos);
            size_t closeBracket = content.find(']', openBracket);
            if (openBracket != std::string::npos && closeBracket != std::string::npos) {
                std::string arrStr = content.substr(openBracket, closeBracket - openBracket + 1);
                size_t p = 0;
                while ((p = arrStr.find('"', p)) != std::string::npos) {
                    size_t q = arrStr.find('"', p + 1);
                    if (q == std::string::npos) break;
                    std::string u8Str = arrStr.substr(p + 1, q - p - 1);
                    if (!u8Str.empty()) {
                        std::string unescaped;
                        for (size_t i = 0; i < u8Str.length(); ++i) {
                            if (u8Str[i] == '\\' && i + 1 < u8Str.length() && u8Str[i+1] == '\\') {
                                unescaped += '\\';
                                ++i;
                            } else {
                                unescaped += u8Str[i];
                            }
                        }
                        int wlen = MultiByteToWideChar(CP_UTF8, 0, unescaped.c_str(), -1, nullptr, 0);
                        if (wlen > 0) {
                            std::wstring wstr(wlen, 0);
                            MultiByteToWideChar(CP_UTF8, 0, unescaped.c_str(), -1, &wstr[0], wlen);
                            if (!wstr.empty() && wstr.back() == L'\0') wstr.pop_back();
                            hiddenApps.push_back(wstr);
                        }
                    }
                    p = q + 1;
                }
            }
        }

        // 4. Widgets
        widgetClockEnabled = parseBool("clockEnabled", widgetClockEnabled);
        widgetClock24Hour = parseBool("clock24Hour", widgetClock24Hour);
        widgetClockShowSeconds = parseBool("clockShowSeconds", widgetClockShowSeconds);
        widgetClockShowDate = parseBool("clockShowDate", widgetClockShowDate);
        widgetClockX = parseFloat("clockX", widgetClockX);
        widgetClockY = parseFloat("clockY", widgetClockY);
        widgetClockW = parseFloat("clockW", widgetClockW);
        widgetClockH = parseFloat("clockH", widgetClockH);

        widgetDiskEnabled = parseBool("diskEnabled", widgetDiskEnabled);
        widgetDiskRefreshMin = parseInt("diskRefreshMin", widgetDiskRefreshMin);
        // Backward compatibility if old config had diskRefreshSec in seconds
        int oldSec = parseInt("diskRefreshSec", -1);
        if (oldSec > 0) {
            widgetDiskRefreshMin = (std::max)(10, (std::min)(30, oldSec / 60));
        }
        widgetDiskX = parseFloat("diskX", widgetDiskX);
        widgetDiskY = parseFloat("diskY", widgetDiskY);
        widgetDiskW = parseFloat("diskW", widgetDiskW);
        widgetDiskH = parseFloat("diskH", widgetDiskH);

        widgetSysMonEnabled = parseBool("sysMonEnabled", widgetSysMonEnabled);
        widgetSysMonX = parseFloat("sysMonX", widgetSysMonX);
        widgetSysMonY = parseFloat("sysMonY", widgetSysMonY);
        widgetSysMonW = parseFloat("sysMonW", widgetSysMonW);
        widgetSysMonH = parseFloat("sysMonH", widgetSysMonH);

        widgetCalendarEnabled = parseBool("calendarEnabled", widgetCalendarEnabled);
        widgetCalendarX = parseFloat("calendarX", widgetCalendarX);
        widgetCalendarY = parseFloat("calendarY", widgetCalendarY);
        widgetCalendarW = parseFloat("calendarW", widgetCalendarW);
        widgetCalendarH = parseFloat("calendarH", widgetCalendarH);

        widgetMemoEnabled = parseBool("memoEnabled", widgetMemoEnabled);
        widgetMemoX = parseFloat("memoX", widgetMemoX);
        widgetMemoY = parseFloat("memoY", widgetMemoY);
        widgetMemoW = parseFloat("memoW", widgetMemoW);
        widgetMemoH = parseFloat("memoH", widgetMemoH);

        widgetLockPosition = parseBool("widgetLockPosition", widgetLockPosition);
        widgetOpacity = parseFloat("widgetOpacity", widgetOpacity);
        widgetSizePreset = parseInt("widgetSizePreset", widgetSizePreset);

        size_t memosPos = content.find("\"memos\"");
        if (memosPos != std::string::npos) {
            size_t openBracket = content.find('[', memosPos);
            size_t closeBracket = content.find(']', openBracket);
            if (openBracket != std::string::npos && closeBracket != std::string::npos) {
                memoItems.clear();
                std::string arrStr = content.substr(openBracket, closeBracket - openBracket + 1);
                size_t p = 0;
                while ((p = arrStr.find('{', p)) != std::string::npos) {
                    size_t endObj = arrStr.find('}', p);
                    if (endObj == std::string::npos) break;
                    std::string obj = arrStr.substr(p, endObj - p + 1);

                    auto extractVal = [&obj](const std::string& key) -> std::string {
                        size_t kpos = obj.find("\"" + key + "\"");
                        if (kpos == std::string::npos) return "";
                        size_t col = obj.find(':', kpos);
                        if (col == std::string::npos) return "";
                        size_t q1 = obj.find('"', col);
                        if (q1 == std::string::npos) return "";
                        size_t q2 = obj.find('"', q1 + 1);
                        if (q2 == std::string::npos) return "";
                        return obj.substr(q1 + 1, q2 - q1 - 1);
                    };

                    auto extractBool = [&obj](const std::string& key, bool def) -> bool {
                        size_t kpos = obj.find("\"" + key + "\"");
                        if (kpos == std::string::npos) return def;
                        size_t col = obj.find(':', kpos);
                        if (col == std::string::npos) return def;
                        size_t t = obj.find("true", col);
                        size_t f = obj.find("false", col);
                        if (t != std::string::npos && (f == std::string::npos || t < f)) return true;
                        if (f != std::string::npos) return false;
                        return def;
                    };

                    auto toWString = [](const std::string& s) -> std::wstring {
                        std::string unescaped;
                        for (size_t i = 0; i < s.length(); ++i) {
                            if (s[i] == '\\' && i + 1 < s.length() && s[i+1] == '\\') {
                                unescaped += '\\';
                                ++i;
                            } else {
                                unescaped += s[i];
                            }
                        }
                        int wlen = MultiByteToWideChar(CP_UTF8, 0, unescaped.c_str(), -1, nullptr, 0);
                        if (wlen > 0) {
                            std::wstring res(wlen, 0);
                            MultiByteToWideChar(CP_UTF8, 0, unescaped.c_str(), -1, &res[0], wlen);
                            if (!res.empty() && res.back() == L'\0') res.pop_back();
                            return res;
                        }
                        return L"";
                    };

                    Widgets::MemoItem mi;
                    mi.text = toWString(extractVal("text"));
                    mi.done = extractBool("done", false);
                    if (!mi.text.empty()) {
                        memoItems.push_back(mi);
                    }
                    p = endObj + 1;
                }
            }
        }
        if (memoItems.empty()) {
            memoItems = {
                { L"保持专注，优雅从容", false },
                { L"享受纯净美观的桌面空间", false },
                { L"Macify 开源桌面套件", true }
            };
        }

        // 5. Appearance
        themePreset = parseInt("themePreset", themePreset);
        accentColor = parseInt("accentColor", accentColor);
        cornerRadius = parseFloat("cornerRadius", cornerRadius);
    }

    void SaveToFile(const std::wstring& filePath, const std::string& existingPinnedAppsJson = "") const {
        std::ostringstream ss;
        ss << "{\n";
        
        // 1. General
        ss << "  \"general\": {\n";
        ss << "    \"autoStart\": " << (autoStart ? "true" : "false") << ",\n";
        ss << "    \"hideWindowsTaskbar\": " << (hideWindowsTaskbar ? "true" : "false") << ",\n";
        ss << "    \"gameModeSuspend\": " << (gameModeSuspend ? "true" : "false") << ",\n";
        ss << "    \"showTrayIcon\": " << (showTrayIcon ? "true" : "false") << ",\n";
        ss << "    \"notifyOnAppDrop\": " << (notifyOnAppDrop ? "true" : "false") << "\n";
        ss << "  },\n";

        // 2. Dock
        ss << "  \"dock\": {\n";
        ss << "    \"baseIconSize\": " << static_cast<int>(dockBaseSize) << ",\n";
        ss << "    \"maxIconSize\": " << static_cast<int>(dockMaxSize) << ",\n";
        ss << "    \"spread\": " << dockSpread << ",\n";
        ss << "    \"bottomMargin\": " << dockBottomMargin << ",\n";
        ss << "    \"barVisualHeight\": " << dockBarVisualHeight << ",\n";
        ss << "    \"bgOpacity\": " << dockBgOpacity << ",\n";
        ss << "    \"showRunningDot\": " << (dockShowRunningDot ? "true" : "false") << ",\n";
        ss << "    \"showTooltip\": " << (dockShowTooltip ? "true" : "false") << ",\n";
        ss << "    \"autoHide\": " << (dockAutoHide ? "true" : "false") << ",\n";
        ss << "    \"slideSpeed\": " << dockSlideSpeed;
        
        if (!pinnedApps.empty()) {
            ss << ",\n    \"pinnedApps\": [\n";
            for (size_t i = 0; i < pinnedApps.size(); ++i) {
                auto toEscapedUtf8 = [](const std::wstring& ws) -> std::string {
                    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
                    std::string u8(len > 1 ? len - 1 : 0, '\0');
                    if (len > 1) {
                        WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &u8[0], len, nullptr, nullptr);
                    }
                    std::string escaped;
                    for (char c : u8) {
                        if (c == '\\') escaped += "\\\\";
                        else escaped += c;
                    }
                    return escaped;
                };

                ss << "      {\n";
                ss << "        \"name\": \"" << toEscapedUtf8(pinnedApps[i].name) << "\",\n";
                if (!pinnedApps[i].icon.empty()) {
                    ss << "        \"icon\": \"" << toEscapedUtf8(pinnedApps[i].icon) << "\",\n";
                }
                if (!pinnedApps[i].path.empty()) {
                    ss << "        \"path\": \"" << toEscapedUtf8(pinnedApps[i].path) << "\",\n";
                }
                ss << "        \"action\": \"" << toEscapedUtf8(pinnedApps[i].action) << "\"\n";
                ss << "      }" << (i + 1 < pinnedApps.size() ? ",\n" : "\n");
            }
            ss << "    ]\n";
        } else if (!existingPinnedAppsJson.empty()) {
            ss << ",\n    \"pinnedApps\": " << existingPinnedAppsJson << "\n";
        } else {
            ss << "\n";
        }
        ss << "  },\n";

        // 3. Launchpad
        ss << "  \"drawer\": {\n";
        ss << "    \"columns\": " << drawerColumns << ",\n";
        ss << "    \"iconSize\": " << drawerIconSize << ",\n";
        ss << "    \"autoCloseOnBlur\": " << (drawerAutoCloseOnBlur ? "true" : "false") << ",\n";
        ss << "    \"searchAutoFocus\": " << (drawerSearchAutoFocus ? "true" : "false") << ",\n";
        ss << "    \"blurMode\": " << drawerBlurMode;
        
        if (!customApps.empty()) {
            ss << ",\n    \"customApps\": [\n";
            for (size_t i = 0; i < customApps.size(); ++i) {
                std::string u8Name, u8Path;
                int nlen = WideCharToMultiByte(CP_UTF8, 0, customApps[i].first.c_str(), -1, nullptr, 0, nullptr, nullptr);
                if (nlen > 0) {
                    std::string tmp(nlen, 0);
                    WideCharToMultiByte(CP_UTF8, 0, customApps[i].first.c_str(), -1, &tmp[0], nlen, nullptr, nullptr);
                    if (!tmp.empty() && tmp.back() == '\0') tmp.pop_back();
                    u8Name = tmp;
                }
                int plen = WideCharToMultiByte(CP_UTF8, 0, customApps[i].second.c_str(), -1, nullptr, 0, nullptr, nullptr);
                if (plen > 0) {
                    std::string tmp(plen, 0);
                    WideCharToMultiByte(CP_UTF8, 0, customApps[i].second.c_str(), -1, &tmp[0], plen, nullptr, nullptr);
                    if (!tmp.empty() && tmp.back() == '\0') tmp.pop_back();
                    u8Path = tmp;
                }

                std::string escapedPath;
                for (char c : u8Path) {
                    if (c == '\\') escapedPath += "\\\\";
                    else escapedPath += c;
                }

                ss << "      {\n";
                ss << "        \"name\": \"" << u8Name << "\",\n";
                ss << "        \"path\": \"" << escapedPath << "\"\n";
                ss << "      }" << (i + 1 < customApps.size() ? "," : "") << "\n";
            }
            ss << "    ]\n";
        } else {
            ss << "\n";
        }

        if (!hiddenApps.empty()) {
            ss << ",\n    \"hiddenApps\": [\n";
            for (size_t i = 0; i < hiddenApps.size(); ++i) {
                int nlen = WideCharToMultiByte(CP_UTF8, 0, hiddenApps[i].c_str(), -1, nullptr, 0, nullptr, nullptr);
                std::string u8Str(nlen > 1 ? nlen - 1 : 0, '\0');
                if (nlen > 1) {
                    WideCharToMultiByte(CP_UTF8, 0, hiddenApps[i].c_str(), -1, &u8Str[0], nlen, nullptr, nullptr);
                }
                std::string escaped;
                for (char c : u8Str) {
                    if (c == '\\') escaped += "\\\\";
                    else escaped += c;
                }
                ss << "      \"" << escaped << "\"" << (i + 1 < hiddenApps.size() ? ",\n" : "\n");
            }
            ss << "    ]\n";
        }
        ss << "  },\n";

        // 4. Widgets
        ss << "  \"widgets\": {\n";
        ss << "    \"clockEnabled\": " << (widgetClockEnabled ? "true" : "false") << ",\n";
        ss << "    \"clock24Hour\": " << (widgetClock24Hour ? "true" : "false") << ",\n";
        ss << "    \"clockShowSeconds\": " << (widgetClockShowSeconds ? "true" : "false") << ",\n";
        ss << "    \"clockShowDate\": " << (widgetClockShowDate ? "true" : "false") << ",\n";
        ss << "    \"clockX\": " << widgetClockX << ",\n";
        ss << "    \"clockY\": " << widgetClockY << ",\n";
        ss << "    \"clockW\": " << widgetClockW << ",\n";
        ss << "    \"clockH\": " << widgetClockH << ",\n";
        ss << "    \"diskEnabled\": " << (widgetDiskEnabled ? "true" : "false") << ",\n";
        ss << "    \"diskRefreshMin\": " << widgetDiskRefreshMin << ",\n";
        ss << "    \"diskX\": " << widgetDiskX << ",\n";
        ss << "    \"diskY\": " << widgetDiskY << ",\n";
        ss << "    \"diskW\": " << widgetDiskW << ",\n";
        ss << "    \"diskH\": " << widgetDiskH << ",\n";
        ss << "    \"sysMonEnabled\": " << (widgetSysMonEnabled ? "true" : "false") << ",\n";
        ss << "    \"sysMonX\": " << widgetSysMonX << ",\n";
        ss << "    \"sysMonY\": " << widgetSysMonY << ",\n";
        ss << "    \"sysMonW\": " << widgetSysMonW << ",\n";
        ss << "    \"sysMonH\": " << widgetSysMonH << ",\n";
        ss << "    \"calendarEnabled\": " << (widgetCalendarEnabled ? "true" : "false") << ",\n";
        ss << "    \"calendarX\": " << widgetCalendarX << ",\n";
        ss << "    \"calendarY\": " << widgetCalendarY << ",\n";
        ss << "    \"calendarW\": " << widgetCalendarW << ",\n";
        ss << "    \"calendarH\": " << widgetCalendarH << ",\n";
        ss << "    \"memoEnabled\": " << (widgetMemoEnabled ? "true" : "false") << ",\n";
        ss << "    \"memoX\": " << widgetMemoX << ",\n";
        ss << "    \"memoY\": " << widgetMemoY << ",\n";
        ss << "    \"memoW\": " << widgetMemoW << ",\n";
        ss << "    \"memoH\": " << widgetMemoH << ",\n";
        ss << "    \"memos\": [\n";
        for (size_t i = 0; i < memoItems.size(); ++i) {
            int nlen = WideCharToMultiByte(CP_UTF8, 0, memoItems[i].text.c_str(), -1, nullptr, 0, nullptr, nullptr);
            std::string u8Text(nlen > 1 ? nlen - 1 : 0, '\0');
            if (nlen > 1) {
                WideCharToMultiByte(CP_UTF8, 0, memoItems[i].text.c_str(), -1, &u8Text[0], nlen, nullptr, nullptr);
            }
            std::string escaped;
            for (char c : u8Text) {
                if (c == '\\') escaped += "\\\\";
                else if (c == '"') escaped += "\\\"";
                else escaped += c;
            }
            ss << "      {\n";
            ss << "        \"text\": \"" << escaped << "\",\n";
            ss << "        \"done\": " << (memoItems[i].done ? "true" : "false") << "\n";
            ss << "      }" << (i + 1 < memoItems.size() ? "," : "") << "\n";
        }
        ss << "    ],\n";
        ss << "    \"lockPosition\": " << (widgetLockPosition ? "true" : "false") << ",\n";
        ss << "    \"widgetSizePreset\": " << widgetSizePreset << ",\n";
        ss << "    \"opacity\": " << widgetOpacity << "\n";
        ss << "  },\n";

        // 5. Appearance
        ss << "  \"appearance\": {\n";
        ss << "    \"themePreset\": " << themePreset << ",\n";
        ss << "    \"accentColor\": " << accentColor << ",\n";
        ss << "    \"cornerRadius\": " << cornerRadius << "\n";
        ss << "  }\n";
        ss << "}\n";

        std::string jsonStr = ss.str();
        std::ofstream file(filePath.c_str(), std::ios::trunc);
        if (file.is_open()) {
            file << jsonStr;
            file.close();
        }
    }
};

class SettingsManager {
public:
    static SettingsManager& Instance() {
        static SettingsManager inst;
        return inst;
    }

    AppConfig& Config() { return m_config; }
    const AppConfig& Config() const { return m_config; }

private:
    SettingsManager() {
        m_config.ResetToDefaults();
    }
    AppConfig m_config;
};

} // namespace Settings