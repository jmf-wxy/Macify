#include "DrawerWindow.h"
#include "AppScanner.h"
#include "../core/Backdrop.h"
#include "../settings/AppSettings.h"
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <algorithm>
#include <cwctype>
#include <cmath>

namespace Drawer {

constexpr UINT_PTR TIMER_CARET_ID = 3001;
constexpr UINT_PTR TIMER_LONG_PRESS_ID = 3002;

DrawerWindow::DrawerWindow()
    : m_hwnd(nullptr)
    , m_dragIndex(-1)
    , m_dragStart{ 0, 0 }
    , m_dragCurrent{ 0, 0 }
    , m_isDragging(false)
    , m_winWidth(780.0f)
    , m_winHeight(580.0f)
    , m_columns(5)
    , m_cellWidth(144.0f)
    , m_cellHeight(104.0f)
    , m_gridPaddingX(30.0f)
    , m_iconSize(48.0f)
    , m_autoCloseOnBlur(true)
    , m_autoFocusSearch(true)
    , m_cornerRadius(22.0f)
    , m_caretVisible(true)
    , m_selectedIndex(0)
    , m_hoveredIndex(-1)
    , m_scrollOffset(0.0f)
    , m_listTop(72.0f)
    , m_listBottom(564.0f)
    , m_pBrushBg(nullptr)
    , m_pBrushBorder(nullptr)
    , m_pBrushSearchBg(nullptr)
    , m_pBrushSearchBorder(nullptr)
    , m_pBrushSearchText(nullptr)
    , m_pBrushPlaceholder(nullptr)
    , m_pBrushCaret(nullptr)
    , m_pBrushHover(nullptr)
    , m_pBrushSelected(nullptr)
    , m_pBrushItemText(nullptr)
    , m_pBrushScrollThumb(nullptr)
    , m_pBrushAddBtn(nullptr)
    , m_pBrushAddBtnHover(nullptr)
    , m_pSearchFormat(nullptr)
    , m_pItemFormat(nullptr)
    , m_pEmptyFormat(nullptr)
    , m_pAddBtnFormat(nullptr)
    , m_btnAddRect{ 0.0f, 0.0f, 0.0f, 0.0f }
    , m_isAddBtnHovered(false)
    , m_isHoveringDeleteBtn(false)
    , m_hoveredDeleteIndex(-1)
    , m_pBrushDeleteBadge(nullptr)
    , m_pBrushDeleteBadgeHover(nullptr)
    , m_pBrushWhite(nullptr) {
}

DrawerWindow::~DrawerWindow() {
    if (m_hwnd) {
        KillTimer(m_hwnd, TIMER_CARET_ID);
        KillTimer(m_hwnd, TIMER_LONG_PRESS_ID);
    }

    if (m_pBrushWhite) m_pBrushWhite->Release();
    if (m_pBrushDeleteBadgeHover) m_pBrushDeleteBadgeHover->Release();
    if (m_pBrushDeleteBadge) m_pBrushDeleteBadge->Release();

    if (m_pAddBtnFormat) m_pAddBtnFormat->Release();
    if (m_pEmptyFormat) m_pEmptyFormat->Release();
    if (m_pItemFormat) m_pItemFormat->Release();
    if (m_pSearchFormat) m_pSearchFormat->Release();

    if (m_pBrushAddBtnHover) m_pBrushAddBtnHover->Release();
    if (m_pBrushAddBtn) m_pBrushAddBtn->Release();
    if (m_pBrushScrollThumb) m_pBrushScrollThumb->Release();
    if (m_pBrushItemText) m_pBrushItemText->Release();
    if (m_pBrushSelected) m_pBrushSelected->Release();
    if (m_pBrushHover) m_pBrushHover->Release();
    if (m_pBrushCaret) m_pBrushCaret->Release();
    if (m_pBrushPlaceholder) m_pBrushPlaceholder->Release();
    if (m_pBrushSearchText) m_pBrushSearchText->Release();
    if (m_pBrushSearchBorder) m_pBrushSearchBorder->Release();
    if (m_pBrushSearchBg) m_pBrushSearchBg->Release();
    if (m_pBrushBorder) m_pBrushBorder->Release();
    if (m_pBrushBg) m_pBrushBg->Release();
}

bool DrawerWindow::Create() {
    const wchar_t CLASS_NAME[] = L"MinimalMacDrawerWindow";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DrawerWindow::WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Make width and height over 68% of the computer screen (comfortable macOS Launchpad style)
    m_winWidth = std::max(1200.0f, std::round(screenWidth * 0.68f));
    m_winHeight = std::max(750.0f, std::round(screenHeight * 0.68f));

    m_columns = 7;
    m_gridPaddingX = 48.0f;
    m_cellWidth = (m_winWidth - m_gridPaddingX * 2.0f) / m_columns;
    m_cellHeight = 126.0f;
    m_iconSize = 58.0f;

    m_listTop = 92.0f;
    m_listBottom = m_winHeight - 24.0f;

    // Dead center on primary screen
    int winX = (screenWidth - static_cast<int>(m_winWidth)) / 2;
    int winY = (screenHeight - static_cast<int>(m_winHeight)) / 2;

    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"MacLaunchpadDrawer",
        WS_POPUP,
        winX,
        winY,
        static_cast<int>(m_winWidth),
        static_cast<int>(m_winHeight),
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );

    if (!m_hwnd) return false;

    Core::RemoveWindowBorder(m_hwnd);
    DragAcceptFiles(m_hwnd, TRUE);

    if (!m_d2d.Initialize(m_hwnd)) return false;

    auto rt = m_d2d.GetRenderTarget();
    if (rt) {
        rt->CreateSolidColorBrush(D2D1::ColorF(0.09f, 0.10f, 0.14f, 0.94f), &m_pBrushBg);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.16f), &m_pBrushBorder);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.08f), &m_pBrushSearchBg);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.22f), &m_pBrushSearchBorder);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.96f), &m_pBrushSearchText);
        rt->CreateSolidColorBrush(D2D1::ColorF(0.65f, 0.68f, 0.75f, 0.65f), &m_pBrushPlaceholder);
        rt->CreateSolidColorBrush(D2D1::ColorF(0.35f, 0.65f, 1.0f, 0.95f), &m_pBrushCaret);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.07f), &m_pBrushHover);
        rt->CreateSolidColorBrush(D2D1::ColorF(0.25f, 0.55f, 0.95f, 0.28f), &m_pBrushSelected);
        rt->CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.96f, 0.98f, 0.95f), &m_pBrushItemText);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.28f), &m_pBrushScrollThumb);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.09f), &m_pBrushAddBtn);
        rt->CreateSolidColorBrush(D2D1::ColorF(0.04f, 0.52f, 1.0f, 0.90f), &m_pBrushAddBtnHover);
        rt->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.22f, 0.22f, 0.88f), &m_pBrushDeleteBadge);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.25f, 0.25f, 1.0f), &m_pBrushDeleteBadgeHover);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &m_pBrushWhite);
    }

    auto dwrite = m_d2d.GetDWriteFactory();
    if (dwrite) {
        dwrite->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            13.5f,
            L"zh-CN",
            &m_pSearchFormat
        );
        if (m_pSearchFormat) {
            m_pSearchFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        dwrite->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.0f,
            L"zh-CN",
            &m_pItemFormat
        );
        if (m_pItemFormat) {
            m_pItemFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_pItemFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        }

        dwrite->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            14.0f,
            L"zh-CN",
            &m_pEmptyFormat
        );
        if (m_pEmptyFormat) {
            m_pEmptyFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_pEmptyFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        dwrite->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.5f,
            L"zh-CN",
            &m_pAddBtnFormat
        );
        if (m_pAddBtnFormat) {
            m_pAddBtnFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_pAddBtnFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    ReloadApps();
    return true;
}

void DrawerWindow::ReloadApps() {
    m_allApps = AppScanner::ScanDesktopApps();

    // Merge custom added applications from configuration
    auto& cfg = Settings::SettingsManager::Instance().Config();
    for (const auto& custom : cfg.customApps) {
        bool alreadyExists = false;
        for (const auto& app : m_allApps) {
            if (app->targetPath == custom.second || app->name == custom.first) {
                alreadyExists = true;
                break;
            }
        }
        if (!alreadyExists && !custom.second.empty()) {
            auto item = std::make_unique<AppItem>();
            item->name = custom.first;
            item->targetPath = custom.second;
            item->iconPath = custom.second;
            item->searchKey = custom.first;
            std::transform(item->searchKey.begin(), item->searchKey.end(), item->searchKey.begin(), ::towlower);
            if (m_d2d.GetWICFactory() && m_d2d.GetRenderTarget()) {
                item->EnsureIcon(m_d2d.GetWICFactory(), m_d2d.GetRenderTarget());
            }
            m_allApps.push_back(std::move(item));
        }
    }

    // Filter out permanently deleted / hidden apps so they never appear again
    auto hit = std::remove_if(m_allApps.begin(), m_allApps.end(), [&](const std::unique_ptr<AppItem>& app) {
        return cfg.IsAppHidden(app->name, app->targetPath);
    });
    if (hit != m_allApps.end()) {
        m_allApps.erase(hit, m_allApps.end());
    }

    std::sort(m_allApps.begin(), m_allApps.end(), [](const std::unique_ptr<AppItem>& a, const std::unique_ptr<AppItem>& b) {
        return a->name < b->name;
    });

    FilterApps();
}

bool DrawerWindow::AddCustomApp(const std::wstring& filePath, const std::wstring& customName) {
    if (filePath.empty()) return false;

    DWORD attr = GetFileAttributesW(filePath.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        return false;
    }

    std::wstring appName = customName;
    std::wstring iconPath = filePath;

    std::wstring lowerPath = filePath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);
    if (lowerPath.length() > 4 && lowerPath.substr(lowerPath.length() - 4) == L".lnk") {
        IShellLinkW* psl = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl))) {
            IPersistFile* ppf = nullptr;
            if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf))) {
                if (SUCCEEDED(ppf->Load(filePath.c_str(), STGM_READ))) {
                    wchar_t szTarget[MAX_PATH] = {};
                    WIN32_FIND_DATAW wfd = {};
                    if (SUCCEEDED(psl->GetPath(szTarget, MAX_PATH, &wfd, SLGP_UNCPRIORITY)) && wcslen(szTarget) > 0) {
                        iconPath = szTarget;
                    }
                }
                ppf->Release();
            }
            psl->Release();
        }
    }

    if (appName.empty()) {
        size_t lastSlash = filePath.find_last_of(L"\\/");
        std::wstring fileName = (lastSlash != std::wstring::npos) ? filePath.substr(lastSlash + 1) : filePath;
        size_t lastDot = fileName.find_last_of(L'.');
        appName = (lastDot != std::wstring::npos) ? fileName.substr(0, lastDot) : fileName;
        std::wstring lowerApp = appName;
        std::transform(lowerApp.begin(), lowerApp.end(), lowerApp.begin(), ::towlower);
        if (lowerApp.length() > 4 && lowerApp.substr(lowerApp.length() - 4) == L".exe") {
            appName = appName.substr(0, appName.length() - 4);
        }
    }

    // Check duplicate
    for (const auto& app : m_allApps) {
        if (app->targetPath == filePath || app->name == appName) {
            return false;
        }
    }

    auto item = std::make_unique<AppItem>();
    item->name = appName;
    item->targetPath = filePath;
    item->iconPath = iconPath;
    item->searchKey = appName;
    std::transform(item->searchKey.begin(), item->searchKey.end(), item->searchKey.begin(), ::towlower);
    if (m_d2d.GetWICFactory() && m_d2d.GetRenderTarget()) {
        item->EnsureIcon(m_d2d.GetWICFactory(), m_d2d.GetRenderTarget());
    }

    m_allApps.push_back(std::move(item));
    std::sort(m_allApps.begin(), m_allApps.end(), [](const std::unique_ptr<AppItem>& a, const std::unique_ptr<AppItem>& b) {
        return a->name < b->name;
    });

    FilterApps();

    // Persist to configuration
    auto& cfg = Settings::SettingsManager::Instance().Config();
    bool existsInConfig = false;
    for (const auto& p : cfg.customApps) {
        if (p.second == filePath) {
            existsInConfig = true;
            break;
        }
    }
    if (!existsInConfig) {
        cfg.customApps.push_back({ appName, filePath });
        wchar_t exePathBuf[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH);
        std::wstring exeDir = exePathBuf;
        size_t slash = exeDir.find_last_of(L"\\/");
        std::wstring configPath = (slash != std::wstring::npos) ? (exeDir.substr(0, slash) + L"\\config.json") : L"config.json";
        cfg.SaveToFile(configPath);
    }

    InvalidateRect(m_hwnd, nullptr, FALSE);
    return true;
}

void DrawerWindow::AddCustomAppsFromDialog() {
    wchar_t fileBuf[32768] = {};
    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"应用程序与快捷方式 (*.exe;*.lnk)\0*.exe;*.lnk\0所有文件 (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = sizeof(fileBuf) / sizeof(wchar_t);
    ofn.lpstrTitle = L"从文件资源管理器选择要添加到启动台的应用 (.exe / .lnk)";
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_NODEREFERENCELINKS;

    if (GetOpenFileNameW(&ofn)) {
        const wchar_t* p = fileBuf;
        std::wstring dirOrFile = p;
        p += dirOrFile.length() + 1;

        if (*p == L'\0') {
            AddCustomApp(dirOrFile);
        } else {
            std::wstring dir = dirOrFile;
            if (!dir.empty() && dir.back() != L'\\') dir += L'\\';
            while (*p != L'\0') {
                std::wstring fileName = p;
                AddCustomApp(dir + fileName);
                p += fileName.length() + 1;
            }
        }
    }
}

void DrawerWindow::RemoveCustomApp(const std::wstring& targetPath) {
    auto& cfg = Settings::SettingsManager::Instance().Config();
    auto it = std::remove_if(cfg.customApps.begin(), cfg.customApps.end(), [&](const std::pair<std::wstring, std::wstring>& p) {
        return p.second == targetPath;
    });
    if (it != cfg.customApps.end()) {
        cfg.customApps.erase(it, cfg.customApps.end());
        wchar_t exePathBuf[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH);
        std::wstring exeDir = exePathBuf;
        size_t slash = exeDir.find_last_of(L"\\/");
        std::wstring configPath = (slash != std::wstring::npos) ? (exeDir.substr(0, slash) + L"\\config.json") : L"config.json";
        cfg.SaveToFile(configPath);
    }

    auto ait = std::remove_if(m_allApps.begin(), m_allApps.end(), [&](const std::unique_ptr<AppItem>& app) {
        return app->targetPath == targetPath;
    });
    if (ait != m_allApps.end()) {
        m_allApps.erase(ait, m_allApps.end());
    }

    FilterApps();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DrawerWindow::DeleteApp(const std::wstring& name, const std::wstring& targetPath) {
    auto& cfg = Settings::SettingsManager::Instance().Config();
    cfg.HideApp(name, targetPath);

    // Also remove from customApps if present
    auto it = std::remove_if(cfg.customApps.begin(), cfg.customApps.end(), [&](const std::pair<std::wstring, std::wstring>& p) {
        return p.second == targetPath || p.first == name;
    });
    if (it != cfg.customApps.end()) {
        cfg.customApps.erase(it, cfg.customApps.end());
    }

    // Persist to config.json
    wchar_t exePathBuf[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH);
    std::wstring exeDir = exePathBuf;
    size_t slash = exeDir.find_last_of(L"\\/");
    std::wstring configPath = (slash != std::wstring::npos) ? (exeDir.substr(0, slash) + L"\\config.json") : L"config.json";
    cfg.SaveToFile(configPath);

    // Remove from m_allApps
    auto ait = std::remove_if(m_allApps.begin(), m_allApps.end(), [&](const std::unique_ptr<AppItem>& app) {
        return app->targetPath == targetPath || app->name == name;
    });
    if (ait != m_allApps.end()) {
        m_allApps.erase(ait, m_allApps.end());
    }

    m_hoveredIndex = -1;
    m_isHoveringDeleteBtn = false;
    m_hoveredDeleteIndex = -1;

    FilterApps();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DrawerWindow::OnDropFiles(HDROP hDrop) {
    if (!hDrop) return;
    UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
    for (UINT i = 0; i < count; ++i) {
        wchar_t szFile[MAX_PATH] = {};
        if (DragQueryFileW(hDrop, i, szFile, MAX_PATH) > 0) {
            AddCustomApp(szFile);
        }
    }
    DragFinish(hDrop);
}

void DrawerWindow::OnRButtonUp(int x, int y) {
    int clickedIdx = -1;
    if (y >= m_listTop && y <= m_listBottom && x >= m_gridPaddingX && x <= m_winWidth - m_gridPaddingX) {
        int col = static_cast<int>((x - m_gridPaddingX) / m_cellWidth);
        int row = static_cast<int>((y - m_listTop + m_scrollOffset) / m_cellHeight);
        int idx = row * m_columns + col;
        if (col >= 0 && col < m_columns && idx >= 0 && idx < static_cast<int>(m_filteredApps.size())) {
            clickedIdx = idx;
        }
    }

    POINT screenPt = { x, y };
    ClientToScreen(m_hwnd, &screenPt);

    HMENU hMenu = CreatePopupMenu();
    if (clickedIdx >= 0) {
        auto* app = m_filteredApps[clickedIdx];
        std::wstring openText = L"🚀 打开 " + app->name;
        AppendMenuW(hMenu, MF_STRING, 101, openText.c_str());
        AppendMenuW(hMenu, MF_STRING, 102, L"📌 固定到底部 Dock 栏");
        AppendMenuW(hMenu, MF_STRING, 103, L"📁 打开文件所在目录");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, 104, L"🗑️ 从启动台删除此图标 (不再在此显示)");
    } else {
        AppendMenuW(hMenu, MF_STRING, 201, L"➕ 从资源管理器添加应用 (.exe / .lnk)...");
        AppendMenuW(hMenu, MF_STRING, 202, L"🔄 重新全盘扫描桌面应用");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, 203, L"⚙️ 系统偏好设置...");
    }

    SetForegroundWindow(m_hwnd);
    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screenPt.x, screenPt.y, 0, m_hwnd, nullptr);
    DestroyMenu(hMenu);

    if (cmd == 101 && clickedIdx >= 0) {
        auto* app = m_filteredApps[clickedIdx];
        ShellExecuteW(nullptr, L"open", app->targetPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        Show(false);
    } else if (cmd == 102 && clickedIdx >= 0) {
        auto* app = m_filteredApps[clickedIdx];
        if (m_onPinToDock) {
            m_onPinToDock(app->name, app->targetPath);
        }
    } else if (cmd == 103 && clickedIdx >= 0) {
        auto* app = m_filteredApps[clickedIdx];
        std::wstring selectArg = L"/select,\"" + app->targetPath + L"\"";
        ShellExecuteW(nullptr, L"open", L"explorer.exe", selectArg.c_str(), nullptr, SW_SHOWNORMAL);
    } else if (cmd == 104 && clickedIdx >= 0) {
        auto* app = m_filteredApps[clickedIdx];
        std::wstring confirmMsg = L"确定要从启动台中删除应用【" + app->name + L"】吗？\n\n删除后该图标将永久屏蔽且不再在此显示（您可随时在设置中一键恢复）。";
        if (MessageBoxW(m_hwnd, confirmMsg.c_str(), L"从启动台删除应用图标", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            DeleteApp(app->name, app->targetPath);
        }
    } else if (cmd == 201) {
        AddCustomAppsFromDialog();
    } else if (cmd == 202) {
        ReloadApps();
    } else if (cmd == 203) {
        if (m_onOpenSettings) {
            m_onOpenSettings();
        }
    }
}

void DrawerWindow::FilterApps() {
    m_filteredApps.clear();
    std::wstring query = m_searchText;
    std::transform(query.begin(), query.end(), query.begin(), ::towlower);

    for (const auto& item : m_allApps) {
        if (query.empty() || item->searchKey.find(query) != std::wstring::npos) {
            m_filteredApps.push_back(item.get());
        }
    }

    m_selectedIndex = 0;
    m_scrollOffset = 0.0f;
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DrawerWindow::ApplySettings(int columns, float iconSize, bool autoCloseOnBlur, bool autoFocusSearch) {
    if (columns >= 4 && columns <= 10) {
        m_columns = columns;
        m_cellWidth = (m_winWidth - m_gridPaddingX * 2.0f) / m_columns;
    }
    m_iconSize = iconSize;
    m_autoCloseOnBlur = autoCloseOnBlur;
    m_autoFocusSearch = autoFocusSearch;
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DrawerWindow::ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, float cornerRadius) {
    m_cornerRadius = cornerRadius;
    if (m_pBrushBg) m_pBrushBg->SetColor(bg);
    if (m_pBrushBorder) m_pBrushBorder->SetColor(border);
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DrawerWindow::Toggle() {
    if (IsVisible()) {
        Show(false);
    } else {
        Show(true);
    }
}

void DrawerWindow::Show(bool show) {
    if (!m_hwnd) return;

    if (show) {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int winX = (screenWidth - static_cast<int>(m_winWidth)) / 2;
        int winY = (screenHeight - static_cast<int>(m_winHeight)) / 2;
        SetWindowPos(m_hwnd, HWND_TOPMOST, winX, winY, static_cast<int>(m_winWidth), static_cast<int>(m_winHeight), SWP_SHOWWINDOW);

        m_searchText.clear();
        ReloadApps(); // Instant refresh of current desktop applications
        SetForegroundWindow(m_hwnd);
        SetFocus(m_hwnd);
        SetTimer(m_hwnd, TIMER_CARET_ID, 500, nullptr);
        InvalidateRect(m_hwnd, nullptr, FALSE);
    } else {
        KillTimer(m_hwnd, TIMER_CARET_ID);
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

bool DrawerWindow::IsVisible() const {
    return m_hwnd && IsWindowVisible(m_hwnd);
}

void DrawerWindow::LaunchSelected() {
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_filteredApps.size())) {
        auto* item = m_filteredApps[m_selectedIndex];
        ShellExecuteW(nullptr, L"open", item->targetPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        Show(false);
    }
}

void DrawerWindow::OnKeyDown(WPARAM vk) {
    if (vk == VK_ESCAPE) {
        Show(false);
    } else if (vk == VK_RETURN) {
        LaunchSelected();
    } else if (vk == VK_LEFT) {
        if (m_selectedIndex > 0) {
            m_selectedIndex--;
            int row = m_selectedIndex / m_columns;
            float rowTop = row * m_cellHeight;
            if (rowTop < m_scrollOffset) m_scrollOffset = rowTop;
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    } else if (vk == VK_RIGHT) {
        if (m_selectedIndex < static_cast<int>(m_filteredApps.size()) - 1) {
            m_selectedIndex++;
            int row = m_selectedIndex / m_columns;
            float rowBottom = (row + 1) * m_cellHeight;
            float viewHeight = m_listBottom - m_listTop;
            if (rowBottom > m_scrollOffset + viewHeight) {
                m_scrollOffset = rowBottom - viewHeight;
            }
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    } else if (vk == VK_UP) {
        if (m_selectedIndex >= m_columns) {
            m_selectedIndex -= m_columns;
            int row = m_selectedIndex / m_columns;
            float rowTop = row * m_cellHeight;
            if (rowTop < m_scrollOffset) m_scrollOffset = rowTop;
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    } else if (vk == VK_DOWN) {
        if (m_selectedIndex + m_columns < static_cast<int>(m_filteredApps.size())) {
            m_selectedIndex += m_columns;
            int row = m_selectedIndex / m_columns;
            float rowBottom = (row + 1) * m_cellHeight;
            float viewHeight = m_listBottom - m_listTop;
            if (rowBottom > m_scrollOffset + viewHeight) {
                m_scrollOffset = rowBottom - viewHeight;
            }
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    } else if (vk == VK_BACK) {
        if (!m_searchText.empty()) {
            m_searchText.pop_back();
            FilterApps();
        }
    }
}

void DrawerWindow::OnChar(wchar_t ch) {
    if (ch >= 32) {
        m_searchText.push_back(ch);
        FilterApps();
    }
}

void DrawerWindow::OnMouseWheel(short delta) {
    int totalRows = (static_cast<int>(m_filteredApps.size()) + m_columns - 1) / m_columns;
    float maxScroll = totalRows * m_cellHeight - (m_listBottom - m_listTop);
    if (maxScroll < 0.0f) maxScroll = 0.0f;

    float step = (delta > 0 ? -m_cellHeight : m_cellHeight);
    m_scrollOffset += step;
    if (m_scrollOffset < 0.0f) m_scrollOffset = 0.0f;
    if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;

    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DrawerWindow::OnLButtonDown(int x, int y) {
    if (m_isHoveringDeleteBtn && m_hoveredDeleteIndex >= 0 && m_hoveredDeleteIndex < static_cast<int>(m_filteredApps.size())) {
        auto* app = m_filteredApps[m_hoveredDeleteIndex];
        std::wstring confirmMsg = L"确定要从启动台中删除应用【" + app->name + L"】吗？\n\n删除后该图标将永久屏蔽且不再在此显示（您可随时在设置中一键恢复）。";
        if (MessageBoxW(m_hwnd, confirmMsg.c_str(), L"从启动台删除应用图标", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            DeleteApp(app->name, app->targetPath);
        }
        return;
    }

    if (x >= m_btnAddRect.left && x <= m_btnAddRect.right && y >= m_btnAddRect.top && y <= m_btnAddRect.bottom) {
        AddCustomAppsFromDialog();
        return;
    }

    if (y >= m_listTop && y <= m_listBottom && x >= m_gridPaddingX && x <= m_winWidth - m_gridPaddingX) {
        int col = static_cast<int>((x - m_gridPaddingX) / m_cellWidth);
        int row = static_cast<int>((y - m_listTop + m_scrollOffset) / m_cellHeight);
        int idx = row * m_columns + col;
        if (col >= 0 && col < m_columns && idx >= 0 && idx < static_cast<int>(m_filteredApps.size())) {
            m_dragIndex = idx;
            m_selectedIndex = idx;
            m_dragStart = { x, y };
            m_dragCurrent = { x, y };
            m_isDragging = false;
            SetCapture(m_hwnd);
            SetTimer(m_hwnd, TIMER_LONG_PRESS_ID, 220, nullptr);
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }
}

void DrawerWindow::OnMouseMove(int x, int y) {
    bool inAddBtn = (x >= m_btnAddRect.left && x <= m_btnAddRect.right && y >= m_btnAddRect.top && y <= m_btnAddRect.bottom);
    if (m_isAddBtnHovered != inAddBtn) {
        m_isAddBtnHovered = inAddBtn;
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    if (m_dragIndex != -1) {
        m_dragCurrent = { x, y };
        int dx = x - m_dragStart.x;
        int dy = y - m_dragStart.y;
        if (!m_isDragging && (dx * dx + dy * dy) > 16) { // 4px threshold to activate dragging
            KillTimer(m_hwnd, TIMER_LONG_PRESS_ID);
            m_isDragging = true;
            m_selectedIndex = m_dragIndex;
            if (m_onDragStart && m_dragIndex >= 0 && m_dragIndex < static_cast<int>(m_filteredApps.size())) {
                POINT screenPt = { x, y };
                ClientToScreen(m_hwnd, &screenPt);
                auto* app = m_filteredApps[m_dragIndex];
                m_onDragStart(app->name, app->targetPath, !app->iconPath.empty() ? app->iconPath : app->targetPath, screenPt.x, screenPt.y);
            }
        }

        if (m_isDragging) {
            POINT screenPt = m_dragCurrent;
            ClientToScreen(m_hwnd, &screenPt);
            if (m_onDragMove) {
                m_onDragMove(screenPt.x, screenPt.y);
            }
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        }
    }

    bool oldHoverDelete = m_isHoveringDeleteBtn;
    int oldHoverDeleteIdx = m_hoveredDeleteIndex;
    m_isHoveringDeleteBtn = false;
    m_hoveredDeleteIndex = -1;

    if (y >= m_listTop && y <= m_listBottom && x >= m_gridPaddingX && x <= m_winWidth - m_gridPaddingX) {
        int col = static_cast<int>((x - m_gridPaddingX) / m_cellWidth);
        int row = static_cast<int>((y - m_listTop + m_scrollOffset) / m_cellHeight);
        int idx = row * m_columns + col;
        if (col >= 0 && col < m_columns && idx >= 0 && idx < static_cast<int>(m_filteredApps.size())) {
            float cellX = m_gridPaddingX + col * m_cellWidth;
            float rowTop = m_listTop + row * m_cellHeight - m_scrollOffset;
            float iconX = cellX + (m_cellWidth - m_iconSize) / 2.0f;
            float iconY = rowTop + 12.0f;
            float badgeCenterX = iconX + m_iconSize - 2.0f;
            float badgeCenterY = iconY + 2.0f;

            float distSq = (x - badgeCenterX) * (x - badgeCenterX) + (y - badgeCenterY) * (y - badgeCenterY);
            if (distSq <= 12.0f * 12.0f) {
                m_isHoveringDeleteBtn = true;
                m_hoveredDeleteIndex = idx;
            }

            if (m_isHoveringDeleteBtn || inAddBtn) {
                SetCursor(LoadCursor(nullptr, IDC_HAND));
            } else {
                SetCursor(LoadCursor(nullptr, IDC_ARROW));
            }

            if (m_hoveredIndex != idx || oldHoverDelete != m_isHoveringDeleteBtn || oldHoverDeleteIdx != m_hoveredDeleteIndex) {
                m_hoveredIndex = idx;
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return;
        }
    }

    if (inAddBtn) {
        SetCursor(LoadCursor(nullptr, IDC_HAND));
    } else {
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
    }

    if (m_hoveredIndex != -1 || oldHoverDelete) {
        m_hoveredIndex = -1;
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void DrawerWindow::OnLButtonUp(int x, int y) {
    KillTimer(m_hwnd, TIMER_LONG_PRESS_ID);
    if (GetCapture() == m_hwnd) {
        ReleaseCapture();
    }

    if (m_isDragging && m_dragIndex >= 0 && m_dragIndex < static_cast<int>(m_filteredApps.size())) {
        POINT screenPt = { x, y };
        ClientToScreen(m_hwnd, &screenPt);
        auto* app = m_filteredApps[m_dragIndex];
        if (m_onDropApp) {
            m_onDropApp(app->name, app->targetPath, screenPt.x, screenPt.y);
        }
        if (m_onDragEnd) {
            m_onDragEnd();
        }
        m_isDragging = false;
        m_dragIndex = -1;
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return;
    }

    if (m_isDragging && m_onDragEnd) {
        m_onDragEnd();
    }

    m_isDragging = false;
    int clickedIdx = m_dragIndex;
    m_dragIndex = -1;

    if (y >= m_listTop && y <= m_listBottom && x >= m_gridPaddingX && x <= m_winWidth - m_gridPaddingX) {
        int col = static_cast<int>((x - m_gridPaddingX) / m_cellWidth);
        int row = static_cast<int>((y - m_listTop + m_scrollOffset) / m_cellHeight);
        int idx = row * m_columns + col;
        if (col >= 0 && col < m_columns && idx >= 0 && idx < static_cast<int>(m_filteredApps.size()) && idx == clickedIdx) {
            m_selectedIndex = idx;
            LaunchSelected();
        }
    }
}

void DrawerWindow::Render() {
    auto rt = m_d2d.GetRenderTarget();
    if (!rt) return;

    m_d2d.BeginDraw();
    rt->Clear(D2D1::ColorF(0, 0, 0, 0.0f));

    // 1. Draw rounded frosted card
    D2D1_ROUNDED_RECT cardRect = D2D1::RoundedRect(
        D2D1::RectF(0.0f, 0.0f, m_winWidth, m_winHeight),
        m_cornerRadius,
        m_cornerRadius
    );
    rt->FillRoundedRectangle(cardRect, m_pBrushBg);
    rt->DrawRoundedRectangle(cardRect, m_pBrushBorder, 1.2f);

    // 2. Draw Centered Pill Search Bar at top
    float searchW = 500.0f;
    float searchH = 42.0f;
    float searchLeft = (m_winWidth - searchW) / 2.0f;
    float searchTop = 24.0f;
    float searchRight = searchLeft + searchW;
    float searchBottom = searchTop + searchH;

    D2D1_ROUNDED_RECT searchRect = D2D1::RoundedRect(
        D2D1::RectF(searchLeft, searchTop, searchRight, searchBottom),
        21.0f,
        21.0f
    );
    rt->FillRoundedRectangle(searchRect, m_pBrushSearchBg);
    rt->DrawRoundedRectangle(searchRect, m_pBrushSearchBorder, 1.0f);

    // Add App button right of search bar
    float btnW = 126.0f;
    float btnH = searchH;
    float btnX = searchRight + 16.0f;
    float btnY = searchTop;
    m_btnAddRect = D2D1::RectF(btnX, btnY, btnX + btnW, btnY + btnH);

    D2D1_ROUNDED_RECT addBtnRounded = D2D1::RoundedRect(m_btnAddRect, 21.0f, 21.0f);
    rt->FillRoundedRectangle(addBtnRounded, m_isAddBtnHovered ? m_pBrushAddBtnHover : m_pBrushAddBtn);
    rt->DrawRoundedRectangle(addBtnRounded, m_pBrushBorder, 1.0f);

    if (m_pAddBtnFormat) {
        const wchar_t addText[] = L"➕ 添加应用";
        rt->DrawTextW(addText, 6, m_pAddBtnFormat, m_btnAddRect, m_isAddBtnHovered ? m_pBrushSearchText : m_pBrushItemText);
    }

    float textPaddingX = searchLeft + 16.0f;
    if (m_searchText.empty()) {
        if (m_pSearchFormat && m_pBrushPlaceholder) {
            const wchar_t placeholder[] = L"🔍 搜索桌面应用...";
            rt->DrawTextW(
                placeholder,
                static_cast<UINT32>(wcslen(placeholder)),
                m_pSearchFormat,
                D2D1::RectF(textPaddingX, searchTop, searchRight - 16.0f, searchBottom),
                m_pBrushPlaceholder
            );
        }
    } else {
        if (m_pSearchFormat && m_pBrushSearchText) {
            rt->DrawTextW(
                m_searchText.c_str(),
                static_cast<UINT32>(m_searchText.length()),
                m_pSearchFormat,
                D2D1::RectF(textPaddingX, searchTop, searchRight - 16.0f, searchBottom),
                m_pBrushSearchText
            );
        }
    }

    // Blinking caret
    if (m_caretVisible) {
        float caretX = textPaddingX;
        if (!m_searchText.empty() && m_d2d.GetDWriteFactory()) {
            IDWriteTextLayout* pLayout = nullptr;
            m_d2d.GetDWriteFactory()->CreateTextLayout(
                m_searchText.c_str(),
                static_cast<UINT32>(m_searchText.length()),
                m_pSearchFormat,
                searchRight - searchLeft,
                searchBottom - searchTop,
                &pLayout
            );
            if (pLayout) {
                DWRITE_TEXT_METRICS tm;
                pLayout->GetMetrics(&tm);
                caretX += tm.widthIncludingTrailingWhitespace + 2.0f;
                pLayout->Release();
            }
        }
        rt->DrawLine(
            D2D1::Point2F(caretX, searchTop + 8.0f),
            D2D1::Point2F(caretX, searchBottom - 8.0f),
            m_pBrushCaret,
            1.8f
        );
    }

    // 3. Draw Grid/Table of Apps with clipping
    D2D1_RECT_F clipRect = D2D1::RectF(m_gridPaddingX - 10.0f, m_listTop, m_winWidth - m_gridPaddingX + 10.0f, m_listBottom);
    rt->PushAxisAlignedClip(clipRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    if (m_filteredApps.empty()) {
        if (m_pEmptyFormat && m_pBrushPlaceholder) {
            const wchar_t noMatch[] = L"未找到相关应用";
            rt->DrawTextW(
                noMatch,
                static_cast<UINT32>(wcslen(noMatch)),
                m_pEmptyFormat,
                D2D1::RectF(0.0f, m_listTop + 140.0f, m_winWidth, m_listTop + 200.0f),
                m_pBrushPlaceholder
            );
        }
    } else {
        int startRow = static_cast<int>(m_scrollOffset / m_cellHeight);
        int visibleRows = static_cast<int>((m_listBottom - m_listTop) / m_cellHeight) + 2;
        int endRow = startRow + visibleRows;

        int totalApps = static_cast<int>(m_filteredApps.size());

        for (int row = startRow; row < endRow; ++row) {
            float rowTop = m_listTop + row * m_cellHeight - m_scrollOffset;
            float rowBottom = rowTop + m_cellHeight;

            for (int col = 0; col < m_columns; ++col) {
                int idx = row * m_columns + col;
                if (idx >= totalApps) break;

                auto* item = m_filteredApps[idx];
                float cellX = m_gridPaddingX + col * m_cellWidth;

                // Draw tile selection / hover background
                D2D1_ROUNDED_RECT tileRect = D2D1::RoundedRect(
                    D2D1::RectF(cellX + 6.0f, rowTop + 4.0f, cellX + m_cellWidth - 6.0f, rowBottom - 4.0f),
                    12.0f,
                    12.0f
                );

                bool isItemBeingDragged = (m_isDragging && idx == m_dragIndex);

                if (isItemBeingDragged) {
                    rt->DrawRoundedRectangle(tileRect, m_pBrushSearchBorder, 1.2f);
                } else if (idx == m_selectedIndex) {
                    rt->FillRoundedRectangle(tileRect, m_pBrushSelected);
                    rt->DrawRoundedRectangle(tileRect, m_pBrushSearchBorder, 1.2f);
                } else if (idx == m_hoveredIndex) {
                    rt->FillRoundedRectangle(tileRect, m_pBrushHover);
                }

                // Lazy load and render 48x48 icon
                item->EnsureIcon(m_d2d.GetWICFactory(), rt);
                float iconX = cellX + (m_cellWidth - m_iconSize) / 2.0f;
                float iconY = rowTop + 12.0f;
                float itemAlpha = isItemBeingDragged ? 0.30f : 1.0f;

                if (item->pBitmap) {
                    rt->DrawBitmap(
                        item->pBitmap,
                        D2D1::RectF(iconX, iconY, iconX + m_iconSize, iconY + m_iconSize),
                        itemAlpha,
                        D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
                    );
                } else {
                    D2D1_ROUNDED_RECT fallbackRect = D2D1::RoundedRect(
                        D2D1::RectF(iconX, iconY, iconX + m_iconSize, iconY + m_iconSize),
                        12.0f,
                        12.0f
                    );
                    rt->FillRoundedRectangle(fallbackRect, m_pBrushBorder);
                }

                // Draw app name centered below icon
                if (m_pItemFormat && m_pBrushItemText) {
                    D2D1_RECT_F textRect = D2D1::RectF(
                        cellX + 6.0f,
                        iconY + m_iconSize + 6.0f,
                        cellX + m_cellWidth - 6.0f,
                        rowBottom - 6.0f
                    );
                    rt->DrawTextW(
                        item->name.c_str(),
                        static_cast<UINT32>(item->name.length()),
                        m_pItemFormat,
                        textRect,
                        m_pBrushItemText
                    );
                }

                // Draw delicate red deletion badge (✕) on top-right of icon when hovered
                if (idx == m_hoveredIndex && !isItemBeingDragged) {
                    float badgeCenterX = iconX + m_iconSize - 2.0f;
                    float badgeCenterY = iconY + 2.0f;
                    float badgeRadius = 9.0f;

                    D2D1_ELLIPSE badgeCircle = D2D1::Ellipse(
                        D2D1::Point2F(badgeCenterX, badgeCenterY),
                        badgeRadius,
                        badgeRadius
                    );

                    bool isHoverThisBadge = (m_isHoveringDeleteBtn && m_hoveredDeleteIndex == idx);
                    ID2D1SolidColorBrush* badgeBrush = isHoverThisBadge ? m_pBrushDeleteBadgeHover : m_pBrushDeleteBadge;
                    if (badgeBrush) {
                        rt->FillEllipse(badgeCircle, badgeBrush);
                    }

                    // Draw 'X' cross
                    if (m_pBrushWhite) {
                        float crossHalf = 3.5f;
                        rt->DrawLine(
                            D2D1::Point2F(badgeCenterX - crossHalf, badgeCenterY - crossHalf),
                            D2D1::Point2F(badgeCenterX + crossHalf, badgeCenterY + crossHalf),
                            m_pBrushWhite,
                            1.8f
                        );
                        rt->DrawLine(
                            D2D1::Point2F(badgeCenterX + crossHalf, badgeCenterY - crossHalf),
                            D2D1::Point2F(badgeCenterX - crossHalf, badgeCenterY + crossHalf),
                            m_pBrushWhite,
                            1.8f
                        );
                    }
                }
            }
        }
    }

    rt->PopAxisAlignedClip();

    // 4. Draw Scrollbar on right edge if needed
    int totalRows = (static_cast<int>(m_filteredApps.size()) + m_columns - 1) / m_columns;
    float totalContentHeight = totalRows * m_cellHeight;
    float viewHeight = m_listBottom - m_listTop;
    if (totalContentHeight > viewHeight) {
        float trackHeight = viewHeight;
        float thumbHeight = (viewHeight / totalContentHeight) * trackHeight;
        if (thumbHeight < 28.0f) thumbHeight = 28.0f;

        float thumbY = m_listTop + (m_scrollOffset / (totalContentHeight - viewHeight)) * (trackHeight - thumbHeight);
        float scrollX = m_winWidth - 12.0f;

        D2D1_ROUNDED_RECT thumbRect = D2D1::RoundedRect(
            D2D1::RectF(scrollX, thumbY, scrollX + 4.0f, thumbY + thumbHeight),
            2.0f,
            2.0f
        );
        rt->FillRoundedRectangle(thumbRect, m_pBrushScrollThumb);
    }

    m_d2d.EndDraw();
}

LRESULT CALLBACK DrawerWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    DrawerWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<DrawerWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<DrawerWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT DrawerWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_KEYDOWN:
        OnKeyDown(wParam);
        return 0;

    case WM_CHAR:
        OnChar(static_cast<wchar_t>(wParam));
        return 0;

    case WM_MOUSEWHEEL:
        OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;

    case WM_MOUSEMOVE:
        OnMouseMove(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_LBUTTONDOWN:
        OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_LBUTTONUP:
        OnLButtonUp(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_RBUTTONUP:
        OnRButtonUp(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_DROPFILES:
        OnDropFiles(reinterpret_cast<HDROP>(wParam));
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_CARET_ID && IsVisible()) {
            m_caretVisible = !m_caretVisible;
            InvalidateRect(m_hwnd, nullptr, FALSE);
        } else if (wParam == TIMER_LONG_PRESS_ID) {
            KillTimer(m_hwnd, TIMER_LONG_PRESS_ID);
            if (m_dragIndex != -1 && !m_isDragging && m_dragIndex < static_cast<int>(m_filteredApps.size())) {
                m_isDragging = true;
                if (m_onDragStart) {
                    POINT screenPt = m_dragCurrent;
                    ClientToScreen(m_hwnd, &screenPt);
                    auto* app = m_filteredApps[m_dragIndex];
                    m_onDragStart(app->name, app->targetPath, !app->iconPath.empty() ? app->iconPath : app->targetPath, screenPt.x, screenPt.y);
                }
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        }
        return 0;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE && !m_isDragging) {
            if (m_autoCloseOnBlur) {
                Show(false);
            }
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);
        Render();
        EndPaint(m_hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcW(m_hwnd, msg, wParam, lParam);
}

} // namespace Drawer
