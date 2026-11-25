#include "SettingsWindow.h"
#include "../core/Backdrop.h"
#include "../core/Version.h"
#include <algorithm>
#include <cmath>

namespace Settings {

static std::wstring GetExecutableDir() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path = exePath;
    size_t lastSlash = path.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        return path.substr(0, lastSlash);
    }
    return L".";
}

SettingsWindow::SettingsWindow()
    : m_hwnd(nullptr)
    , m_winWidth(860.0f)
    , m_winHeight(680.0f)
    , m_activeTab(0)
    , m_hoveredTab(-1)
    , m_activeSlider(-1)
    , m_hoveredButton(-1)
    , m_mouseX(-1)
    , m_mouseY(-1)
    , m_pBrushBg(nullptr)
    , m_pBrushSidebarBg(nullptr)
    , m_pBrushCardBg(nullptr)
    , m_pBrushBorder(nullptr)
    , m_pBrushTextTitle(nullptr)
    , m_pBrushTextDesc(nullptr)
    , m_pBrushAccent(nullptr)
    , m_pBrushToggleOff(nullptr)
    , m_pBrushWhite(nullptr)
    , m_pFormatTitle(nullptr)
    , m_pFormatSubtitle(nullptr)
    , m_pFormatItemTitle(nullptr)
    , m_pFormatItemDesc(nullptr)
    , m_pFormatTab(nullptr)
    , m_pFormatButton(nullptr) {
}

SettingsWindow::~SettingsWindow() {
    if (m_pFormatButton) m_pFormatButton->Release();
    if (m_pFormatTab) m_pFormatTab->Release();
    if (m_pFormatItemDesc) m_pFormatItemDesc->Release();
    if (m_pFormatItemTitle) m_pFormatItemTitle->Release();
    if (m_pFormatSubtitle) m_pFormatSubtitle->Release();
    if (m_pFormatTitle) m_pFormatTitle->Release();

    if (m_pBrushWhite) m_pBrushWhite->Release();
    if (m_pBrushToggleOff) m_pBrushToggleOff->Release();
    if (m_pBrushAccent) m_pBrushAccent->Release();
    if (m_pBrushTextDesc) m_pBrushTextDesc->Release();
    if (m_pBrushTextTitle) m_pBrushTextTitle->Release();
    if (m_pBrushBorder) m_pBrushBorder->Release();
    if (m_pBrushCardBg) m_pBrushCardBg->Release();
    if (m_pBrushSidebarBg) m_pBrushSidebarBg->Release();
    if (m_pBrushBg) m_pBrushBg->Release();
}

bool SettingsWindow::Create() {
    const wchar_t CLASS_NAME[] = L"MacDesktopSettingsWindow";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SettingsWindow::WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth - static_cast<int>(m_winWidth)) / 2;
    int posY = (screenHeight - static_cast<int>(m_winHeight)) / 2;

    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"Mac 偏好设置",
        WS_POPUP,
        posX,
        posY,
        static_cast<int>(m_winWidth),
        static_cast<int>(m_winHeight),
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );

    if (!m_hwnd) return false;

    // Remove window border and enable clean transparent alpha channel without gray acrylic corner artifacts
    Core::RemoveWindowBorder(m_hwnd);

    if (!m_d2d.Initialize(m_hwnd)) return false;

    auto rt = m_d2d.GetRenderTarget();
    if (rt) {
        rt->CreateSolidColorBrush(D2D1::ColorF(0.11f, 0.12f, 0.15f, 0.98f), &m_pBrushBg);
        rt->CreateSolidColorBrush(D2D1::ColorF(0.08f, 0.09f, 0.11f, 0.98f), &m_pBrushSidebarBg);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.045f), &m_pBrushCardBg);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.10f), &m_pBrushBorder);
        rt->CreateSolidColorBrush(D2D1::ColorF(0.96f, 0.96f, 0.98f, 0.98f), &m_pBrushTextTitle);
        rt->CreateSolidColorBrush(D2D1::ColorF(0.58f, 0.62f, 0.69f, 0.90f), &m_pBrushTextDesc);
        rt->CreateSolidColorBrush(GetAccentColor(), &m_pBrushAccent);
        rt->CreateSolidColorBrush(D2D1::ColorF(0.22f, 0.23f, 0.26f, 0.95f), &m_pBrushToggleOff);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &m_pBrushWhite);
        ApplyTheme();
    }

    auto dwrite = m_d2d.GetDWriteFactory();
    if (dwrite) {
        dwrite->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 19.0f, L"zh-CN", &m_pFormatTitle);
        if (m_pFormatTitle) {
            m_pFormatTitle->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        dwrite->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"zh-CN", &m_pFormatSubtitle);
        if (m_pFormatSubtitle) {
            m_pFormatSubtitle->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        dwrite->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.5f, L"zh-CN", &m_pFormatItemTitle);
        if (m_pFormatItemTitle) {
            m_pFormatItemTitle->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        dwrite->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 11.5f, L"zh-CN", &m_pFormatItemDesc);
        if (m_pFormatItemDesc) {
            m_pFormatItemDesc->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        dwrite->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 13.0f, L"zh-CN", &m_pFormatTab);
        if (m_pFormatTab) {
            m_pFormatTab->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        dwrite->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.5f, L"zh-CN", &m_pFormatButton);
        if (m_pFormatButton) {
            m_pFormatButton->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_pFormatButton->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    return true;
}

void SettingsWindow::Show(bool show) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, show ? SW_SHOW : SW_HIDE);
        if (show) {
            SetForegroundWindow(m_hwnd);
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }
}

bool SettingsWindow::IsVisible() const {
    return m_hwnd && IsWindowVisible(m_hwnd);
}

void SettingsWindow::SyncConfig(const AppConfig& cfg) {
    m_config = cfg;
    ApplyTheme();
}

void SettingsWindow::ApplyTheme() {
    ThemePalette p = GetThemePalette(m_config.themePreset, m_config.accentColor, m_config.dockBgOpacity, m_config.widgetOpacity);
    if (m_pBrushBg) m_pBrushBg->SetColor(p.windowBg);
    if (m_pBrushSidebarBg) m_pBrushSidebarBg->SetColor(p.sidebarBg);
    if (m_pBrushCardBg) m_pBrushCardBg->SetColor(p.cardBg);
    if (m_pBrushBorder) m_pBrushBorder->SetColor(p.border);
    if (m_pBrushAccent) m_pBrushAccent->SetColor(p.accent);
    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

D2D1_COLOR_F SettingsWindow::GetAccentColor() const {
    switch (m_config.accentColor) {
        case 1: return D2D1::ColorF(0.188f, 0.820f, 0.345f, 1.0f); // 翡翠绿 #30D158
        case 2: return D2D1::ColorF(0.749f, 0.353f, 0.949f, 1.0f); // 霓虹紫 #BF5AF2
        case 3: return D2D1::ColorF(1.000f, 0.271f, 0.227f, 1.0f); // 珊瑚红 #FF453A
        default: return D2D1::ColorF(0.039f, 0.518f, 1.000f, 1.0f); // Mac经典蓝 #0A84FF
    }
}

LRESULT CALLBACK SettingsWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SettingsWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<SettingsWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<SettingsWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT SettingsWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(m_hwnd, &ps);
            Render();
            EndPaint(m_hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            int x = static_cast<short>(LOWORD(lParam));
            int y = static_cast<short>(HIWORD(lParam));
            OnLButtonDown(x, y);
            return 0;
        }
        case WM_LBUTTONUP: {
            int x = static_cast<short>(LOWORD(lParam));
            int y = static_cast<short>(HIWORD(lParam));
            OnLButtonUp(x, y);
            return 0;
        }
        case WM_MOUSEMOVE: {
            int x = static_cast<short>(LOWORD(lParam));
            int y = static_cast<short>(HIWORD(lParam));
            OnMouseMove(x, y);
            return 0;
        }
        case WM_KEYDOWN: {
            OnKeyDown(wParam);
            return 0;
        }
        case WM_DESTROY: {
            m_hwnd = nullptr;
            return 0;
        }
    }
    return DefWindowProcW(m_hwnd, msg, wParam, lParam);
}

void SettingsWindow::Render() {
    auto rt = m_d2d.GetRenderTarget();
    if (!rt) return;

    m_d2d.BeginDraw();
    rt->Clear(D2D1::ColorF(0, 0, 0, 0.0f));

    // Outer rounded card (clean, non-artifacting true macOS window curvature)
    D2D1_ROUNDED_RECT mainCard = D2D1::RoundedRect(
        D2D1::RectF(0.0f, 0.0f, m_winWidth, m_winHeight),
        16.0f, 16.0f
    );
    rt->FillRoundedRectangle(mainCard, m_pBrushBg);
    rt->DrawRoundedRectangle(mainCard, m_pBrushBorder, 1.0f);

    // 1. Sidebar background
    float sidebarW = 216.0f;
    D2D1_ROUNDED_RECT sideCard = D2D1::RoundedRect(
        D2D1::RectF(0.0f, 0.0f, sidebarW, m_winHeight),
        16.0f, 16.0f
    );
    rt->FillRoundedRectangle(sideCard, m_pBrushSidebarBg);
    rt->DrawLine(D2D1::Point2F(sidebarW, 0.0f), D2D1::Point2F(sidebarW, m_winHeight), m_pBrushBorder, 1.0f);

    // 2. Header and Mac buttons
    DrawHeader(rt);

    // 3. Left tabs
    DrawSidebar(rt);

    // 4. Right content area
    DrawTabContent(rt);

    m_d2d.EndDraw();
}

void SettingsWindow::DrawHeader(ID2D1RenderTarget* rt) {
    // Mac Window control dots
    float cy = 25.0f;
    float r = 6.0f;

    // Red (Close)
    ID2D1SolidColorBrush* pBrushRed = nullptr;
    rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.36f, 0.34f, 1.0f), &pBrushRed);
    if (pBrushRed) {
        rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(22.0f, cy), r, r), pBrushRed);
        pBrushRed->Release();
    }

    // Yellow (Minimize)
    ID2D1SolidColorBrush* pBrushYellow = nullptr;
    rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.76f, 0.20f, 1.0f), &pBrushYellow);
    if (pBrushYellow) {
        rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(42.0f, cy), r, r), pBrushYellow);
        pBrushYellow->Release();
    }

    // Green (Disabled/Placeholder)
    ID2D1SolidColorBrush* pBrushGreen = nullptr;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.18f, 0.80f, 0.35f, 1.0f), &pBrushGreen);
    if (pBrushGreen) {
        rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(62.0f, cy), r, r), pBrushGreen);
        pBrushGreen->Release();
    }

    // Window title in header (strictly aligned and vertically centered with dots)
    if (m_pFormatItemTitle && m_pBrushTextTitle) {
        D2D1_RECT_F titleRect = D2D1::RectF(88.0f, cy - 14.0f, 210.0f, cy + 14.0f);
        rt->DrawTextW(L"系统偏好设置", 6, m_pFormatItemTitle, titleRect, m_pBrushTextTitle);
    }
}

void SettingsWindow::DrawSidebar(ID2D1RenderTarget* rt) {
    const wchar_t* tabNames[] = {
        L"🖥️  常规与系统",
        L"⚓  底部 Dock 栏",
        L"🚀  应用启动台",
        L"🧩  桌面小组件",
        L"🎨  个性化主题",
        L"⚙️  配置与关于"
    };

    float tabY = 58.0f;
    float tabH = 38.0f;
    float tabW = 192.0f;
    float tabX = 12.0f;

    for (int i = 0; i < 6; ++i) {
        D2D1_ROUNDED_RECT tabPill = D2D1::RoundedRect(
            D2D1::RectF(tabX, tabY, tabX + tabW, tabY + tabH),
            8.0f, 8.0f
        );

        if (i == m_activeTab) {
            if (m_pBrushAccent) m_pBrushAccent->SetColor(GetAccentColor());
            rt->FillRoundedRectangle(tabPill, m_pBrushAccent);
        } else if (i == m_hoveredTab) {
            ID2D1SolidColorBrush* pHover = nullptr;
            rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.07f), &pHover);
            if (pHover) {
                rt->FillRoundedRectangle(tabPill, pHover);
                pHover->Release();
            }
        }

        if (m_pFormatTab) {
            D2D1_RECT_F textRect = D2D1::RectF(tabX + 16.0f, tabY, tabX + tabW - 8.0f, tabY + tabH);
            rt->DrawTextW(tabNames[i], static_cast<UINT32>(wcslen(tabNames[i])), m_pFormatTab, textRect, (i == m_activeTab) ? m_pBrushWhite : m_pBrushTextTitle);
        }

        tabY += 46.0f;
    }
}

void SettingsWindow::DrawTabContent(ID2D1RenderTarget* rt) {
    float contentX = 244.0f;
    float contentY = 30.0f;

    switch (m_activeTab) {
        case 0: DrawGeneralTab(rt, contentX, contentY); break;
        case 1: DrawDockTab(rt, contentX, contentY); break;
        case 2: DrawDrawerTab(rt, contentX, contentY); break;
        case 3: DrawWidgetsTab(rt, contentX, contentY); break;
        case 4: DrawAppearanceTab(rt, contentX, contentY); break;
        case 5: DrawAboutTab(rt, contentX, contentY); break;
    }
}

void SettingsWindow::DrawGeneralTab(ID2D1RenderTarget* rt, float contentX, float contentY) {
    // Title
    rt->DrawTextW(L"常规与系统", 5, m_pFormatTitle, D2D1::RectF(contentX, contentY, contentX + 400.0f, contentY + 26.0f), m_pBrushTextTitle);
    rt->DrawTextW(L"管理自启动、原生任务栏智能托管与游戏全屏休眠机制", 24, m_pFormatSubtitle, D2D1::RectF(contentX, contentY + 28.0f, contentX + 550.0f, contentY + 48.0f), m_pBrushTextDesc);

    float cardY = contentY + 58.0f;
    float cardW = 586.0f;
    float cardH = 54.0f;

    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"开机自动启动", L"随 Windows 用户登录自动无感启动桌面套件", m_config.autoStart, 1);
    cardY += cardH + 10.0f;

    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"智能隐藏 Windows 原生任务栏", L"自动隐藏原生底部任务栏，尽享沉浸式纯净 Mac 桌面体验", m_config.hideWindowsTaskbar, 2);
    cardY += cardH + 10.0f;

    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"全屏与游戏模式自动休眠", L"检测到全屏应用或大型 3D 游戏时，立即暂停渲染定时器以节省 CPU 与显卡", m_config.gameModeSuspend, 3);
    cardY += cardH + 10.0f;

    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"常驻系统托盘控制图标", L"在任务栏右下角托盘区保留程序控制图标与快捷管理入口", m_config.showTrayIcon, 4);
    cardY += cardH + 10.0f;

    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"拖拽固定应用即时气泡提醒", L"从应用启动台拖拽图标至底部 Dock 栏固定成功时显示系统提示", m_config.notifyOnAppDrop, 5);
}

void SettingsWindow::DrawDockTab(ID2D1RenderTarget* rt, float contentX, float contentY) {
    rt->DrawTextW(L"底部 Dock 栏精细控制", 12, m_pFormatTitle, D2D1::RectF(contentX, contentY, contentX + 400.0f, contentY + 26.0f), m_pBrushTextTitle);
    rt->DrawTextW(L"实时调节 Dock 图标尺寸、高斯钟形放大曲线、边距与自动避让策略", 33, m_pFormatSubtitle, D2D1::RectF(contentX, contentY + 28.0f, contentX + 550.0f, contentY + 48.0f), m_pBrushTextDesc);

    float cardY = contentY + 58.0f;
    float cardW = 586.0f;
    float cardH = 48.0f;

    DrawSliderRow(rt, contentX, cardY, cardW, cardH, L"基础图标尺寸", L"未悬停时的静止图标像素大小", m_config.dockBaseSize, 36.0f, 72.0f, L"px", 1);
    cardY += cardH + 8.0f;

    DrawSliderRow(rt, contentX, cardY, cardW, cardH, L"悬停放大尺寸", L"鼠标悬停时的最高像素放大尺寸", m_config.dockMaxSize, 64.0f, 130.0f, L"px", 2);
    cardY += cardH + 8.0f;

    DrawSliderRow(rt, contentX, cardY, cardW, cardH, L"放大感应跨度", L"Mac 钟形曲线影响左右相邻图标的感应半径", m_config.dockSpread, 90.0f, 260.0f, L"px", 3);
    cardY += cardH + 8.0f;

    DrawSliderRow(rt, contentX, cardY, cardW, cardH, L"屏幕底部边距", L"Dock 栏距离屏幕物理底部的悬浮高度", m_config.dockBottomMargin, 0.0f, 80.0f, L"px", 4);
    cardY += cardH + 8.0f;

    DrawSliderRow(rt, contentX, cardY, cardW, cardH, L"底座毛玻璃透明度", L"Dock 栏半透明黑色毛玻璃背景的深邃度", m_config.dockBgOpacity * 100.0f, 20.0f, 100.0f, L"%", 5);
    cardY += cardH + 8.0f;

    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"显示运行状态指示点", L"在正在运行的应用下方显示白光状态指示点", m_config.dockShowRunningDot, 6);
    cardY += cardH + 8.0f;

    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"显示图标悬停文字气泡", L"鼠标悬停在图标上方时弹出浮动应用名称", m_config.dockShowTooltip, 7);
    cardY += cardH + 8.0f;

    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"窗口重叠时自动向下滑出隐藏", L"当最大化窗口或有窗口遮挡 Dock 栏时智能避让隐藏", m_config.dockAutoHide, 8);
}

void SettingsWindow::DrawDrawerTab(ID2D1RenderTarget* rt, float contentX, float contentY) {
    rt->DrawTextW(L"应用启动台 (Launchpad)", 16, m_pFormatTitle, D2D1::RectF(contentX, contentY, contentX + 400.0f, contentY + 26.0f), m_pBrushTextTitle);
    rt->DrawTextW(L"个性化网格列数、从资源管理器添加外部应用与即时全盘索引", 28, m_pFormatSubtitle, D2D1::RectF(contentX, contentY + 28.0f, contentX + 550.0f, contentY + 48.0f), m_pBrushTextDesc);

    float cardY = contentY + 58.0f;
    float cardW = 586.0f;
    float cardH = 54.0f;

    std::vector<std::wstring> colOptions = { L"5 列", L"6 列", L"7 列", L"8 列" };
    int selCol = m_config.drawerColumns - 5;
    if (selCol < 0) selCol = 0; if (selCol > 3) selCol = 3;
    DrawSegmentedRow(rt, contentX, cardY, cardW, cardH, L"应用网格展示列数", L"抽屉横向每行所排列的应用图标数目", colOptions, selCol, 1);
    cardY += cardH + 10.0f;

    DrawSliderRow(rt, contentX, cardY, cardW, cardH, L"启动台图标尺寸", L"抽屉网格内各应用图标的显示像素", m_config.drawerIconSize, 42.0f, 72.0f, L"px", 6);
    cardY += cardH + 10.0f;

    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"失去焦点自动收起", L"鼠标点击启动台窗口外部区域时自动平滑隐藏", m_config.drawerAutoCloseOnBlur, 9);
    cardY += cardH + 10.0f;

    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"打开时自动聚焦搜索框", L"呼出启动台时光标自动闪烁，输入即可秒搜应用", m_config.drawerSearchAutoFocus, 10);
    cardY += cardH + 16.0f;

    DrawButton(rt, contentX, cardY, 240.0f, 38.0f, L"➕ 添加外部应用 (.exe / .lnk)...", true, 5);
    DrawButton(rt, contentX + 252.0f, cardY, 190.0f, 38.0f, L"🔄 重新全盘扫描应用", false, 1);

    if (!m_config.hiddenApps.empty()) {
        cardY += 50.0f;
        std::wstring restoreText = L"♻️ 恢复全部已删除图标 (" + std::to_wstring(m_config.hiddenApps.size()) + L" 项屏蔽)";
        DrawButton(rt, contentX, cardY, 260.0f, 38.0f, restoreText.c_str(), false, 6);
    }
}

void SettingsWindow::DrawWidgetsTab(ID2D1RenderTarget* rt, float contentX, float contentY) {
    rt->DrawTextW(L"桌面毛玻璃小组件", 8, m_pFormatTitle, D2D1::RectF(contentX, contentY, contentX + 400.0f, contentY + 30.0f), m_pBrushTextTitle);
    rt->DrawTextW(L"时钟、磁盘、性能监控、日历与便签挂件自由布局与拖拽缩放", 29, m_pFormatSubtitle, D2D1::RectF(contentX, contentY + 26.0f, contentX + 500.0f, contentY + 46.0f), m_pBrushTextDesc);

    float cardY = contentY + 50.0f;
    float cardW = 574.0f;
    float cardH = 44.0f;
    float gap = 7.0f;

    // 1. Clock
    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"启用数字时钟挂件", L"在桌面上显示优雅时钟与公历日期", m_config.widgetClockEnabled, 11);
    cardY += cardH + gap;

    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"24 小时制时间制式", L"关闭则使用 12 小时制显示时间", m_config.widgetClock24Hour, 12);
    cardY += cardH + gap;

    // 2. Disk
    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"启用磁盘容量监控组件", L"实时统计系统盘与各硬盘驱动器剩余容量", m_config.widgetDiskEnabled, 15);
    cardY += cardH + gap;

    DrawSliderRow(rt, contentX, cardY, cardW, cardH, L"磁盘刷新轮询周期", L"读取硬盘驱动器剩余容量的时间间隔（极致节能）", static_cast<float>(m_config.widgetDiskRefreshMin), 10.0f, 30.0f, L"分钟", 7);
    cardY += cardH + gap;

    // 3. SysMon
    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"启用系统性能监控组件", L"实时统计 CPU 负载与物理内存使用率 (双圆角量规)", m_config.widgetSysMonEnabled, 17);
    cardY += cardH + gap;

    // 4. Calendar
    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"启用桌面迷你日历组件", L"桌面常驻当月公历网格并高亮今日焦点", m_config.widgetCalendarEnabled, 18);
    cardY += cardH + gap;

    // 5. Memo
    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"启用桌面灵感便签组件", L"在桌面上记录待办备忘与灵感清单", m_config.widgetMemoEnabled, 19);
    cardY += cardH + gap;

    // 6. Lock
    DrawToggleRow(rt, contentX, cardY, cardW, cardH, L"锁定小组件位置与大小", L"锁定后禁止鼠标随意拖动或缩放", m_config.widgetLockPosition, 16);
    cardY += cardH + gap;

    // 7. Opacity
    DrawSliderRow(rt, contentX, cardY, cardW, cardH, L"小组件半透明度", L"组件亚克力半透明磨砂卡片的透明强度", m_config.widgetOpacity * 100.0f, 30.0f, 100.0f, L"%", 8);
    cardY += cardH + gap;

    // 8. Widget standard size presets
    std::vector<std::wstring> sizeOptions = { L"紧凑 200px", L"标准 220px", L"宽屏 260px", L"超大 300px" };
    DrawSegmentedRow(rt, contentX, cardY, cardW, cardH, L"组件标准规范尺寸", L"一键将所有桌面组件统一设置为规范宽度并靠右对齐", sizeOptions, m_config.widgetSizePreset, 4);
    cardY += cardH + gap + 4.0f;

    // 9. Reset widgets button
    DrawButton(rt, contentX, cardY, 280.0f, 36.0f, L"↺ 恢复全部组件默认尺寸与右上排布", false, 7);
}

void SettingsWindow::DrawAppearanceTab(ID2D1RenderTarget* rt, float contentX, float contentY) {
    rt->DrawTextW(L"个性化主题与外观", 8, m_pFormatTitle, D2D1::RectF(contentX, contentY, contentX + 400.0f, contentY + 30.0f), m_pBrushTextTitle);
    rt->DrawTextW(L"选择全局色彩主题、强调高光色与圆角曲率", 20, m_pFormatSubtitle, D2D1::RectF(contentX, contentY + 26.0f, contentX + 500.0f, contentY + 46.0f), m_pBrushTextDesc);

    float cardY = contentY + 54.0f;
    float cardW = 574.0f;
    float cardH = 58.0f;

    std::vector<std::wstring> themeOptions = { L"深邃黑", L"太空灰", L"冰晶玻璃", L"极光紫" };
    DrawSegmentedRow(rt, contentX, cardY, cardW, cardH, L"主题视觉预设", L"更改全局毛玻璃色调与微光边框对比度", themeOptions, m_config.themePreset, 2);
    cardY += cardH + 12.0f;

    std::vector<std::wstring> colorOptions = { L"经典蓝", L"翡翠绿", L"紫罗兰", L"珊瑚红" };
    DrawSegmentedRow(rt, contentX, cardY, cardW, cardH, L"系统高亮强调色", L"滑动条轨道、开关激活与选定项的高亮色彩", colorOptions, m_config.accentColor, 3);
    cardY += cardH + 12.0f;

    DrawSliderRow(rt, contentX, cardY, cardW, cardH, L"全局卡片圆角曲率", L"调整 Dock 底座、启动台与挂件的弧度大小", m_config.cornerRadius, 12.0f, 32.0f, L"px", 9);
}

void SettingsWindow::DrawAboutTab(ID2D1RenderTarget* rt, float contentX, float contentY) {
    rt->DrawTextW(L"配置管理与关于", 7, m_pFormatTitle, D2D1::RectF(contentX, contentY, contentX + 400.0f, contentY + 30.0f), m_pBrushTextTitle);
    std::wstring aboutSubtitle = L"Mac 风格超轻量桌面美化套件 v" + std::wstring(APP_VERSION_WSTRING) + L" - 专为 Windows 设计";
    rt->DrawTextW(aboutSubtitle.c_str(), static_cast<UINT32>(aboutSubtitle.length()), m_pFormatSubtitle, D2D1::RectF(contentX, contentY + 26.0f, contentX + 550.0f, contentY + 46.0f), m_pBrushTextDesc);

    float cardY = contentY + 54.0f;
    float cardW = 574.0f;

    // Feature card
    D2D1_ROUNDED_RECT infoCard = D2D1::RoundedRect(
        D2D1::RectF(contentX, cardY, contentX + cardW, cardY + 120.0f),
        12.0f, 12.0f
    );
    rt->FillRoundedRectangle(infoCard, m_pBrushCardBg);
    rt->DrawRoundedRectangle(infoCard, m_pBrushBorder, 1.0f);

    const wchar_t* lines[] = {
        L"🌟 Direct2D 硬件加速极致平滑渲染 (60/120Hz 丝滑钟形曲线)",
        L"⚡ 超低开销架构：鼠标离开即完全暂停定时器，静止待机 0.00% CPU",
        L"🛡️ 内存优化版：WIC 原生 96px 约束与即时 GDI 句柄释放，无内存泄漏",
        L"📁 配置文件就绪：设置将自动实时持久化保存于 config.json"
    };

    float lineY = cardY + 16.0f;
    for (int i = 0; i < 4; ++i) {
        rt->DrawTextW(lines[i], static_cast<UINT32>(wcslen(lines[i])), m_pFormatItemDesc, D2D1::RectF(contentX + 16.0f, lineY, contentX + cardW - 16.0f, lineY + 22.0f), m_pBrushTextTitle);
        lineY += 24.0f;
    }

    cardY += 140.0f;

    // Actions
    DrawButton(rt, contentX, cardY, 170.0f, 38.0f, L"↺ 恢复默认设置", false, 2);
    DrawButton(rt, contentX + 185.0f, cardY, 180.0f, 38.0f, L"💾 保存并应用设置", true, 3);
    DrawButton(rt, contentX + 380.0f, cardY, 180.0f, 38.0f, L"⚡ 快速平滑重启", false, 4);
}

void SettingsWindow::DrawToggleRow(ID2D1RenderTarget* rt, float x, float y, float w, float h,
                                  const wchar_t* title, const wchar_t* desc, bool state, int toggleId) {
    D2D1_ROUNDED_RECT card = D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), 10.0f, 10.0f);
    rt->FillRoundedRectangle(card, m_pBrushCardBg);
    rt->DrawRoundedRectangle(card, m_pBrushBorder, 0.8f);

    // Left text (strictly vertically centered relative to card height)
    float textTop = y + (h - 32.0f) / 2.0f;
    if (m_pFormatItemTitle) {
        rt->DrawTextW(title, static_cast<UINT32>(wcslen(title)), m_pFormatItemTitle, D2D1::RectF(x + 16.0f, textTop, x + w - 75.0f, textTop + 17.0f), m_pBrushTextTitle);
    }
    if (m_pFormatItemDesc) {
        rt->DrawTextW(desc, static_cast<UINT32>(wcslen(desc)), m_pFormatItemDesc, D2D1::RectF(x + 16.0f, textTop + 17.0f, x + w - 75.0f, textTop + 32.0f), m_pBrushTextDesc);
    }

    // Right capsule switch (vertically centered)
    float switchW = 46.0f;
    float switchH = 24.0f;
    float switchX = x + w - switchW - 16.0f;
    float switchY = y + (h - switchH) / 2.0f;

    D2D1_ROUNDED_RECT pill = D2D1::RoundedRect(D2D1::RectF(switchX, switchY, switchX + switchW, switchY + switchH), 12.0f, 12.0f);
    if (state) {
        if (m_pBrushAccent) m_pBrushAccent->SetColor(GetAccentColor());
        rt->FillRoundedRectangle(pill, m_pBrushAccent);
    } else {
        rt->FillRoundedRectangle(pill, m_pBrushToggleOff);
    }

    float knobR = 9.0f;
    float knobX = state ? (switchX + switchW - knobR - 3.0f) : (switchX + knobR + 3.0f);
    float knobY = switchY + switchH / 2.0f;
    rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(knobX, knobY), knobR, knobR), m_pBrushWhite);
}

void SettingsWindow::DrawSliderRow(ID2D1RenderTarget* rt, float x, float y, float w, float h,
                                  const wchar_t* title, const wchar_t* desc, float value, float minVal, float maxVal,
                                  const wchar_t* unit, int sliderId) {
    D2D1_ROUNDED_RECT card = D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), 10.0f, 10.0f);
    rt->FillRoundedRectangle(card, m_pBrushCardBg);
    rt->DrawRoundedRectangle(card, m_pBrushBorder, 0.8f);

    // Left text (strictly vertically centered)
    float textTop = y + (h - 32.0f) / 2.0f;
    if (m_pFormatItemTitle) {
        rt->DrawTextW(title, static_cast<UINT32>(wcslen(title)), m_pFormatItemTitle, D2D1::RectF(x + 16.0f, textTop, x + 240.0f, textTop + 17.0f), m_pBrushTextTitle);
    }
    if (m_pFormatItemDesc) {
        rt->DrawTextW(desc, static_cast<UINT32>(wcslen(desc)), m_pFormatItemDesc, D2D1::RectF(x + 16.0f, textTop + 17.0f, x + 240.0f, textTop + 32.0f), m_pBrushTextDesc);
    }

    // Slider track
    float trackX = x + w - 265.0f;
    float trackW = 175.0f;
    float trackY = y + h / 2.0f;
    float clampedVal = (value < minVal) ? minVal : ((value > maxVal) ? maxVal : value);
    float ratio = (clampedVal - minVal) / (maxVal - minVal);

    D2D1_ROUNDED_RECT bgTrack = D2D1::RoundedRect(D2D1::RectF(trackX, trackY - 2.5f, trackX + trackW, trackY + 2.5f), 2.5f, 2.5f);
    rt->FillRoundedRectangle(bgTrack, m_pBrushToggleOff);

    if (m_pBrushAccent) m_pBrushAccent->SetColor(GetAccentColor());
    D2D1_ROUNDED_RECT fillTrack = D2D1::RoundedRect(D2D1::RectF(trackX, trackY - 2.5f, trackX + trackW * ratio, trackY + 2.5f), 2.5f, 2.5f);
    rt->FillRoundedRectangle(fillTrack, m_pBrushAccent);

    // Knob
    float knobX = trackX + trackW * ratio;
    rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(knobX, trackY), 7.5f, 7.5f), m_pBrushWhite);

    // Value text badge (vertically centered with track)
    wchar_t valBuf[32];
    swprintf_s(valBuf, L"%d %s", static_cast<int>(clampedVal), unit);
    D2D1_RECT_F textRect = D2D1::RectF(trackX + trackW + 12.0f, y, x + w - 10.0f, y + h);
    rt->DrawTextW(valBuf, static_cast<UINT32>(wcslen(valBuf)), m_pFormatButton, textRect, m_pBrushTextTitle);
}

void SettingsWindow::DrawSegmentedRow(ID2D1RenderTarget* rt, float x, float y, float w, float h,
                                     const wchar_t* title, const wchar_t* desc, const std::vector<std::wstring>& options, int selectedIdx, int segId) {
    D2D1_ROUNDED_RECT card = D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), 10.0f, 10.0f);
    rt->FillRoundedRectangle(card, m_pBrushCardBg);
    rt->DrawRoundedRectangle(card, m_pBrushBorder, 0.8f);

    // Left text (vertically centered)
    float textTop = y + (h - 32.0f) / 2.0f;
    if (m_pFormatItemTitle) {
        rt->DrawTextW(title, static_cast<UINT32>(wcslen(title)), m_pFormatItemTitle, D2D1::RectF(x + 16.0f, textTop, x + 240.0f, textTop + 17.0f), m_pBrushTextTitle);
    }
    if (m_pFormatItemDesc) {
        rt->DrawTextW(desc, static_cast<UINT32>(wcslen(desc)), m_pFormatItemDesc, D2D1::RectF(x + 16.0f, textTop + 17.0f, x + 240.0f, textTop + 32.0f), m_pBrushTextDesc);
    }

    // Segmented capsule
    float segW = 240.0f;
    float segH = 28.0f;
    float segX = x + w - segW - 16.0f;
    float segY = y + (h - segH) / 2.0f;

    D2D1_ROUNDED_RECT pill = D2D1::RoundedRect(D2D1::RectF(segX, segY, segX + segW, segY + segH), 6.0f, 6.0f);
    rt->FillRoundedRectangle(pill, m_pBrushToggleOff);

    float itemW = segW / options.size();
    for (size_t i = 0; i < options.size(); ++i) {
        float ix = segX + i * itemW;
        if (static_cast<int>(i) == selectedIdx) {
            D2D1_ROUNDED_RECT activePill = D2D1::RoundedRect(D2D1::RectF(ix + 2.0f, segY + 2.0f, ix + itemW - 2.0f, segY + segH - 2.0f), 5.0f, 5.0f);
            if (m_pBrushAccent) m_pBrushAccent->SetColor(GetAccentColor());
            rt->FillRoundedRectangle(activePill, m_pBrushAccent);
        }
        D2D1_RECT_F tRect = D2D1::RectF(ix, segY, ix + itemW, segY + segH);
        rt->DrawTextW(options[i].c_str(), static_cast<UINT32>(options[i].length()), m_pFormatButton, tRect, (static_cast<int>(i) == selectedIdx) ? m_pBrushWhite : m_pBrushTextDesc);
    }
}

void SettingsWindow::DrawButton(ID2D1RenderTarget* rt, float x, float y, float w, float h,
                               const wchar_t* text, bool isPrimary, int buttonId) {
    D2D1_ROUNDED_RECT btnRect = D2D1::RoundedRect(D2D1::RectF(x, y, x + w, y + h), 8.0f, 8.0f);

    if (isPrimary) {
        if (m_pBrushAccent) m_pBrushAccent->SetColor(GetAccentColor());
        rt->FillRoundedRectangle(btnRect, m_pBrushAccent);
    } else {
        ID2D1SolidColorBrush* pBtnBg = nullptr;
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, (m_hoveredButton == buttonId) ? 0.12f : 0.07f), &pBtnBg);
        if (pBtnBg) {
            rt->FillRoundedRectangle(btnRect, pBtnBg);
            pBtnBg->Release();
        }
        rt->DrawRoundedRectangle(btnRect, m_pBrushBorder, 1.0f);
    }

    D2D1_RECT_F tRect = D2D1::RectF(x, y, x + w, y + h);
    if (m_pFormatButton) {
        rt->DrawTextW(text, static_cast<UINT32>(wcslen(text)), m_pFormatButton, tRect, m_pBrushWhite);
    }
}

void SettingsWindow::OnLButtonDown(int x, int y) {
    // 1. Check Mac close button (Red dot)
    if (x >= 12 && x <= 32 && y >= 14 && y <= 34) {
        Show(false);
        return;
    }

    // 2. Drag window header
    if (y < 46 && x > 70) {
        ReleaseCapture();
        SendMessageW(m_hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return;
    }

    // 3. Click sidebar tab
    if (x <= 216) {
        int tabIdx = (y - 58) / 46;
        if (tabIdx >= 0 && tabIdx < 6) {
            m_activeTab = tabIdx;
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        }
    }

    // 4. Click controls in right content area
    float contentX = 244.0f;
    float cardW = 586.0f;

    // Check sliders drag
    if (m_activeTab == 1) { // Dock Tab
        float cardY = 30.0f + 58.0f;
        float cardH = 48.0f;
        for (int sid = 1; sid <= 5; ++sid) {
            float trackX = contentX + cardW - 265.0f;
            float trackW = 175.0f;
            if (x >= trackX - 10 && x <= trackX + trackW + 10 && y >= cardY && y <= cardY + cardH) {
                m_activeSlider = sid;
                SetCapture(m_hwnd);
                float ratio = std::clamp((x - trackX) / trackW, 0.0f, 1.0f);
                HandleSliderDrag(sid, ratio);
                return;
            }
            cardY += cardH + 8.0f;
        }

        // Toggles in Dock tab
        int tids[] = { 6, 7, 8 };
        for (int tid : tids) {
            float switchX = contentX + cardW - 46.0f - 16.0f;
            if (x >= switchX - 10 && x <= switchX + 56 && y >= cardY && y <= cardY + cardH) {
                HandleToggleClick(tid);
                return;
            }
            cardY += cardH + 8.0f;
        }
    } else if (m_activeTab == 0) { // General Tab
        float cardY = 30.0f + 58.0f;
        float cardH = 54.0f;
        for (int tid = 1; tid <= 5; ++tid) {
            float switchX = contentX + cardW - 46.0f - 16.0f;
            if (x >= switchX - 10 && x <= switchX + 56 && y >= cardY && y <= cardY + cardH) {
                HandleToggleClick(tid);
                return;
            }
            cardY += cardH + 10.0f;
        }
    } else if (m_activeTab == 2) { // Drawer Tab
        float cardY = 30.0f + 58.0f;
        float cardH = 54.0f;
        // Segmented columns
        float segX = contentX + cardW - 240.0f - 16.0f;
        if (x >= segX && x <= segX + 240.0f && y >= cardY && y <= cardY + cardH) {
            int opt = static_cast<int>((x - segX) / (240.0f / 4.0f));
            HandleSegmentClick(1, opt);
            return;
        }
        cardY += cardH + 10.0f;

        // Slider 6 (Icon size)
        float trackX = contentX + cardW - 265.0f;
        if (x >= trackX - 10 && x <= trackX + 185 && y >= cardY && y <= cardY + cardH) {
            m_activeSlider = 6;
            SetCapture(m_hwnd);
            float ratio = std::clamp((x - trackX) / 175.0f, 0.0f, 1.0f);
            HandleSliderDrag(6, ratio);
            return;
        }
        cardY += cardH + 10.0f;

        // Toggles 9, 10
        int tids[] = { 9, 10 };
        for (int tid : tids) {
            float switchX = contentX + cardW - 46.0f - 16.0f;
            if (x >= switchX - 10 && x <= switchX + 56 && y >= cardY && y <= cardY + cardH) {
                HandleToggleClick(tid);
                return;
            }
            cardY += cardH + 10.0f;
        }
        cardY += 16.0f;

        // Button 5 (Add from File Explorer)
        if (x >= contentX && x <= contentX + 240.0f && y >= cardY && y <= cardY + 38.0f) {
            HandleButtonClick(5);
            return;
        }
        // Button 1 (Scan apps)
        if (x >= contentX + 252.0f && x <= contentX + 442.0f && y >= cardY && y <= cardY + 38.0f) {
            HandleButtonClick(1);
            return;
        }

        // Button 6 (Restore hidden apps)
        if (!m_config.hiddenApps.empty()) {
            float btnY = cardY + 50.0f;
            if (x >= contentX && x <= contentX + 260.0f && y >= btnY && y <= btnY + 38.0f) {
                HandleButtonClick(6);
                return;
            }
        }
    } else if (m_activeTab == 3) { // Widgets Tab
        float cardY = 28.0f + 50.0f;
        float cardH = 44.0f;
        float gap = 7.0f;
        float switchX = contentX + cardW - 46.0f - 16.0f;
        float trackX = contentX + cardW - 260.0f;

        // Helper lambda to check toggle click
        auto checkToggle = [&](int tid) -> bool {
            if (x >= switchX - 10 && x <= switchX + 56 && y >= cardY && y <= cardY + cardH) {
                HandleToggleClick(tid);
                return true;
            }
            cardY += cardH + gap;
            return false;
        };

        // Row 1: Clock
        if (checkToggle(11)) return;
        // Row 2: Clock 24H
        if (checkToggle(12)) return;
        // Row 3: Disk Enable
        if (checkToggle(15)) return;

        // Row 4: Disk Refresh Slider 7
        if (x >= trackX - 10 && x <= trackX + 185 && y >= cardY && y <= cardY + cardH) {
            m_activeSlider = 7;
            SetCapture(m_hwnd);
            float ratio = std::clamp((x - trackX) / 175.0f, 0.0f, 1.0f);
            HandleSliderDrag(7, ratio);
            return;
        }
        cardY += cardH + gap;

        // Row 5: SysMon
        if (checkToggle(17)) return;
        // Row 6: Calendar
        if (checkToggle(18)) return;
        // Row 7: Memo
        if (checkToggle(19)) return;
        // Row 8: Lock
        if (checkToggle(16)) return;

        // Row 9: Opacity Slider 8
        if (x >= trackX - 10 && x <= trackX + 185 && y >= cardY && y <= cardY + cardH) {
            m_activeSlider = 8;
            SetCapture(m_hwnd);
            float ratio = std::clamp((x - trackX) / 175.0f, 0.0f, 1.0f);
            HandleSliderDrag(8, ratio);
            return;
        }
        cardY += cardH + gap;

        // Row 10: Standard size segmented row (segId = 4)
        float segX = contentX + cardW - 240.0f - 16.0f;
        float segW = 240.0f;
        if (x >= segX && x <= segX + segW && y >= cardY && y <= cardY + cardH) {
            int opt = static_cast<int>((x - segX) / (segW / 4.0f));
            HandleSegmentClick(4, opt);
            return;
        }
        cardY += cardH + gap + 4.0f;

        // Row 11: Button 7 (Reset)
        if (x >= contentX && x <= contentX + 280.0f && y >= cardY && y <= cardY + 36.0f) {
            HandleButtonClick(7);
            return;
        }
    } else if (m_activeTab == 4) { // Appearance Tab
        float cardY = 28.0f + 54.0f;
        float cardH = 58.0f;
        float segX = contentX + cardW - 240.0f - 16.0f;

        // Theme segmented
        if (x >= segX && x <= segX + 240.0f && y >= cardY && y <= cardY + cardH) {
            int opt = (x - segX) / (240.0f / 4.0f);
            HandleSegmentClick(2, opt);
            return;
        }
        cardY += cardH + 12.0f;

        // Color segmented
        if (x >= segX && x <= segX + 240.0f && y >= cardY && y <= cardY + cardH) {
            int opt = (x - segX) / (240.0f / 4.0f);
            HandleSegmentClick(3, opt);
            return;
        }
        cardY += cardH + 12.0f;

        // Slider 9 (Corner radius)
        float trackX = contentX + cardW - 260.0f;
        if (x >= trackX - 10 && x <= trackX + 185 && y >= cardY && y <= cardY + cardH) {
            m_activeSlider = 9;
            SetCapture(m_hwnd);
            float ratio = std::clamp((x - trackX) / 175.0f, 0.0f, 1.0f);
            HandleSliderDrag(9, ratio);
            return;
        }
    } else if (m_activeTab == 5) { // About Tab
        float cardY = 28.0f + 54.0f + 140.0f;
        if (x >= contentX && x <= contentX + 170.0f && y >= cardY && y <= cardY + 38.0f) {
            HandleButtonClick(2);
            return;
        }
        if (x >= contentX + 185.0f && x <= contentX + 365.0f && y >= cardY && y <= cardY + 38.0f) {
            HandleButtonClick(3);
            return;
        }
        if (x >= contentX + 380.0f && x <= contentX + 560.0f && y >= cardY && y <= cardY + 38.0f) {
            HandleButtonClick(4);
            return;
        }
    }
}

void SettingsWindow::OnLButtonUp(int x, int y) {
    if (m_activeSlider != -1) {
        m_activeSlider = -1;
        ReleaseCapture();
        SaveAndApply();
    }
}

void SettingsWindow::OnMouseMove(int x, int y) {
    m_mouseX = x;
    m_mouseY = y;

    // Hover tab
    if (x <= 210) {
        int tabIdx = (y - 56) / 46;
        if (tabIdx >= 0 && tabIdx < 6) {
            if (m_hoveredTab != tabIdx) {
                m_hoveredTab = tabIdx;
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        } else {
            if (m_hoveredTab != -1) {
                m_hoveredTab = -1;
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        }
    } else {
        if (m_hoveredTab != -1) {
            m_hoveredTab = -1;
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    // Active slider dragging
    if (m_activeSlider != -1) {
        float contentX = 236.0f;
        float cardW = 574.0f;
        float trackX = contentX + cardW - 260.0f;
        float trackW = 175.0f;
        float ratio = std::clamp((x - trackX) / trackW, 0.0f, 1.0f);
        HandleSliderDrag(m_activeSlider, ratio);
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void SettingsWindow::OnKeyDown(WPARAM vk) {
    if (vk == VK_ESCAPE) {
        Show(false);
    }
}

void SettingsWindow::HandleToggleClick(int toggleId) {
    switch (toggleId) {
        case 1:
            m_config.autoStart = !m_config.autoStart;
            AppConfig::SetRegistryAutoStart(m_config.autoStart);
            break;
        case 2:
            m_config.hideWindowsTaskbar = !m_config.hideWindowsTaskbar;
            AppConfig::ApplyTaskbarVisibility(m_config.hideWindowsTaskbar);
            break;
        case 3:
            m_config.gameModeSuspend = !m_config.gameModeSuspend;
            break;
        case 4:
            m_config.showTrayIcon = !m_config.showTrayIcon;
            break;
        case 5:
            m_config.notifyOnAppDrop = !m_config.notifyOnAppDrop;
            break;
        case 6:
            m_config.dockShowRunningDot = !m_config.dockShowRunningDot;
            break;
        case 7:
            m_config.dockShowTooltip = !m_config.dockShowTooltip;
            break;
        case 8:
            m_config.dockAutoHide = !m_config.dockAutoHide;
            break;
        case 9:
            m_config.drawerAutoCloseOnBlur = !m_config.drawerAutoCloseOnBlur;
            break;
        case 10:
            m_config.drawerSearchAutoFocus = !m_config.drawerSearchAutoFocus;
            break;
        case 11:
            m_config.widgetClockEnabled = !m_config.widgetClockEnabled;
            break;
        case 12:
            m_config.widgetClock24Hour = !m_config.widgetClock24Hour;
            break;
        case 13:
            m_config.widgetClockShowSeconds = !m_config.widgetClockShowSeconds;
            break;
        case 14:
            m_config.widgetClockShowDate = !m_config.widgetClockShowDate;
            break;
        case 15:
            m_config.widgetDiskEnabled = !m_config.widgetDiskEnabled;
            break;
        case 16:
            m_config.widgetLockPosition = !m_config.widgetLockPosition;
            break;
        case 17:
            m_config.widgetSysMonEnabled = !m_config.widgetSysMonEnabled;
            break;
        case 18:
            m_config.widgetCalendarEnabled = !m_config.widgetCalendarEnabled;
            break;
        case 19:
            m_config.widgetMemoEnabled = !m_config.widgetMemoEnabled;
            break;
    }

    SaveAndApply();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void SettingsWindow::HandleSliderDrag(int sliderId, float ratio) {
    switch (sliderId) {
        case 1: m_config.dockBaseSize = 36.0f + ratio * (72.0f - 36.0f); break;
        case 2: m_config.dockMaxSize = 64.0f + ratio * (130.0f - 64.0f); break;
        case 3: m_config.dockSpread = 90.0f + ratio * (260.0f - 90.0f); break;
        case 4: m_config.dockBottomMargin = ratio * 80.0f; break;
        case 5: m_config.dockBgOpacity = (20.0f + ratio * (100.0f - 20.0f)) / 100.0f; break;
        case 6: m_config.drawerIconSize = 42.0f + ratio * (72.0f - 42.0f); break;
        case 7: m_config.widgetDiskRefreshMin = static_cast<int>(10.0f + ratio * (30.0f - 10.0f)); break;
        case 8: m_config.widgetOpacity = (30.0f + ratio * (100.0f - 30.0f)) / 100.0f; break;
        case 9: m_config.cornerRadius = 12.0f + ratio * (32.0f - 12.0f); break;
    }

    // Live preview during dragging!
    if (m_callbacks.onDockChanged) m_callbacks.onDockChanged(m_config);
    if (m_callbacks.onDrawerChanged) m_callbacks.onDrawerChanged(m_config);
    if (m_callbacks.onWidgetsChanged) m_callbacks.onWidgetsChanged(m_config);
    if (sliderId == 9 && m_callbacks.onAppearanceChanged) m_callbacks.onAppearanceChanged(m_config);
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void SettingsWindow::HandleSegmentClick(int segId, int optionIdx) {
    if (segId == 1) { // Columns
        m_config.drawerColumns = 5 + optionIdx;
    } else if (segId == 2) { // Theme
        m_config.themePreset = optionIdx;
        ApplyTheme();
        if (m_callbacks.onAppearanceChanged) m_callbacks.onAppearanceChanged(m_config);
    } else if (segId == 3) { // Accent
        m_config.accentColor = optionIdx;
        ApplyTheme();
        if (m_callbacks.onAppearanceChanged) m_callbacks.onAppearanceChanged(m_config);
    } else if (segId == 4) { // Widget size preset
        m_config.ApplyWidgetSizePreset(optionIdx);
    }

    SaveAndApply();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void SettingsWindow::HandleButtonClick(int buttonId) {
    if (buttonId == 1) { // Rescan apps
        if (m_callbacks.onReloadApps) m_callbacks.onReloadApps();
        MessageBoxW(m_hwnd, L"全盘桌面与开始菜单应用快捷方式已重新扫描并索引完成！", L"应用启动台", MB_OK | MB_ICONINFORMATION);
    } else if (buttonId == 2) { // Restore defaults
        if (MessageBoxW(m_hwnd, L"确定要恢复出厂默认设置吗？", L"确认恢复", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            m_config.ResetToDefaults();
            ApplyTheme();
            SaveAndApply();
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    } else if (buttonId == 3) { // Save and apply
        SaveAndApply();
        MessageBoxW(m_hwnd, L"设置已成功持久化保存并热生效！", L"保存成功", MB_OK | MB_ICONINFORMATION);
    } else if (buttonId == 4) { // Restart
        SaveAndApply();
        if (m_callbacks.onRestartApp) {
            m_callbacks.onRestartApp();
        }
    } else if (buttonId == 5) { // Add apps from explorer
        if (m_callbacks.onAddAppsFromExplorer) {
            m_callbacks.onAddAppsFromExplorer();
        }
    } else if (buttonId == 6) { // Restore hidden apps
        if (MessageBoxW(m_hwnd, L"确定要恢复所有从启动台中删除的图标吗？\n\n恢复后之前删除的应用图标将重新在启动台中显示。", L"恢复应用图标", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            m_config.RestoreHiddenApps();
            SaveAndApply();
            if (m_callbacks.onReloadApps) {
                m_callbacks.onReloadApps();
            }
            InvalidateRect(m_hwnd, nullptr, FALSE);
            MessageBoxW(m_hwnd, L"已恢复所有已删除的应用图标！", L"恢复成功", MB_OK | MB_ICONINFORMATION);
        }
    } else if (buttonId == 7) { // Reset widgets to default size and top-right layout
        m_config.ResetWidgetLayout();
        SaveAndApply();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        MessageBoxW(m_hwnd, L"已恢复全部小组件的默认尺寸、并靠齐右上角 10px 紧凑排布！", L"组件排布重置", MB_OK | MB_ICONINFORMATION);
    }
}

void SettingsWindow::SaveAndApply() {
    // 1. Notify components
    if (m_callbacks.onDockChanged) m_callbacks.onDockChanged(m_config);
    if (m_callbacks.onDrawerChanged) m_callbacks.onDrawerChanged(m_config);
    if (m_callbacks.onWidgetsChanged) m_callbacks.onWidgetsChanged(m_config);
    if (m_callbacks.onSystemChanged) m_callbacks.onSystemChanged(m_config);
    if (m_callbacks.onAppearanceChanged) m_callbacks.onAppearanceChanged(m_config);

    // 2. Keep global SettingsManager synchronized
    SettingsManager::Instance().Config() = m_config;

    // 3. Save to config.json
    std::wstring configPath = GetExecutableDir() + L"\\config.json";
    m_config.SaveToFile(configPath);
}

} // namespace Settings