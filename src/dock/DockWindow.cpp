#include "DockWindow.h"
#include "../core/Backdrop.h"
#include "../settings/AppSettings.h"
#include <cmath>
#include <algorithm>
#include <shellapi.h>
#include <fstream>
#include <sstream>

namespace Dock {

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

constexpr UINT_PTR TIMER_ANIM_ID = 1001;
constexpr UINT_PTR TIMER_SLIDE_ID = 1002;
constexpr UINT_PTR TIMER_POLL_ID = 1003;
constexpr float PI = 3.14159265358979323846f;

DockWindow::DockWindow()
    : m_hwnd(nullptr)
    , m_baseSize(58.0f)
    , m_maxSize(92.0f)
    , m_spread(155.0f)
    , m_padding(20.0f)
    , m_endPadding(28.0f)
    , m_barVisualHeight(78.0f)
    , m_bottomMargin(36.0f)
    , m_barHeight(150.0f)
    , m_barWidth(450.0f)
    , m_bgOpacity(0.80f)
    , m_showDot(true)
    , m_showTooltip(true)
    , m_autoHideEnabled(false)
    , m_cornerRadius(26.0f)
    , m_themeBgColor(D2D1::ColorF(0.10f, 0.11f, 0.14f, 0.80f))
    , m_themeBorderColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.16f))
    , m_onOpenSettings(nullptr)
    , m_isMouseTracking(false)
    , m_isMouseInside(false)
    , m_mouseX(-1000)
    , m_mouseY(-1000)
    , m_isAnimating(false)
    , m_hoveredIndex(-1)
    , m_isContextMenuOpen(false)
    , m_isOverlapped(false)
    , m_hideProgress(0.0f)
    , m_targetHideProgress(0.0f)
    , m_hideGraceFrames(0)
    , m_isSlideAnimating(false)
    , m_externalDragActive(false)
    , m_insertIndex(-1)
    , m_insertGap(0.0f)
    , m_targetInsertGap(0.0f)
    , m_dragItemIndex(-1)
    , m_dragStartPos{ 0, 0 }
    , m_dragItemCurrentX(0.0f)
    , m_dragItemCurrentY(0.0f)
    , m_isDraggingItem(false)
    , m_isDragItemOutOfDock(false)
    , m_pBrushBg(nullptr)
    , m_pBrushBorder(nullptr)
    , m_pBrushDot(nullptr)
    , m_pBrushText(nullptr)
    , m_pBrushSmoke(nullptr)
    , m_pBrushInsertSlot(nullptr)
    , m_pTextFormatTooltip(nullptr) {
}

DockWindow::~DockWindow() {
    if (m_hwnd) {
        KillTimer(m_hwnd, TIMER_ANIM_ID);
        KillTimer(m_hwnd, TIMER_SLIDE_ID);
        KillTimer(m_hwnd, TIMER_POLL_ID);
    }
    for (auto& eff : m_vanishEffects) {
        if (eff.pBitmap) eff.pBitmap->Release();
    }
    m_vanishEffects.clear();

    if (m_pTextFormatTooltip) m_pTextFormatTooltip->Release();
    if (m_pBrushInsertSlot) m_pBrushInsertSlot->Release();
    if (m_pBrushSmoke) m_pBrushSmoke->Release();
    if (m_pBrushText) m_pBrushText->Release();
    if (m_pBrushDot) m_pBrushDot->Release();
    if (m_pBrushBorder) m_pBrushBorder->Release();
    if (m_pBrushBg) m_pBrushBg->Release();
}

bool DockWindow::Create() {
    const wchar_t CLASS_NAME[] = L"MinimalMacDockWindow";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DockWindow::WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    RECT workArea;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

    // Initial estimation of window coordinates
    int initialW = 480;
    int initialH = static_cast<int>(m_barHeight);
    int initialX = (screenWidth - initialW) / 2;
    int initialY = static_cast<int>(workArea.bottom - m_barHeight - m_bottomMargin);

    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"MacDock",
        WS_POPUP,
        initialX,
        initialY,
        initialW,
        initialH,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );

    if (!m_hwnd) return false;

    // Remove any Windows 11 outer border / frame to keep window 100% borderless & transparent
    Core::RemoveWindowBorder(m_hwnd);

    // Initialize Direct2D
    if (!m_d2d.Initialize(m_hwnd)) return false;

    // Create brushes
    auto rt = m_d2d.GetRenderTarget();
    if (rt) {
        // Deep modern frosted glass dark background
        rt->CreateSolidColorBrush(D2D1::ColorF(0.10f, 0.11f, 0.14f, 0.80f), &m_pBrushBg);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.16f), &m_pBrushBorder);
        rt->CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.95f, 0.98f, 0.95f), &m_pBrushDot);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.96f), &m_pBrushText);
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.85f), &m_pBrushSmoke);
        rt->CreateSolidColorBrush(D2D1::ColorF(0.28f, 0.60f, 1.0f, 0.22f), &m_pBrushInsertSlot);
    }

    // Create tooltip font
    auto dwrite = m_d2d.GetDWriteFactory();
    if (dwrite) {
        dwrite->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_MEDIUM,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.0f,
            L"zh-CN",
            &m_pTextFormatTooltip
        );
        if (m_pTextFormatTooltip) {
            m_pTextFormatTooltip->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_pTextFormatTooltip->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    UpdateLayout();
    SetTimer(m_hwnd, TIMER_POLL_ID, 50, nullptr);
    return true;
}

void DockWindow::Show(bool show) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, show ? SW_SHOW : SW_HIDE);
        UpdateWindow(m_hwnd);
    }
}

void DockWindow::Suspend() {
    if (!m_hwnd) return;
    ShowWindow(m_hwnd, SW_HIDE);
    KillTimer(m_hwnd, TIMER_ANIM_ID);
    KillTimer(m_hwnd, TIMER_SLIDE_ID);
    KillTimer(m_hwnd, TIMER_POLL_ID);
    m_isSlideAnimating = false;
    m_isAnimating = false;
}

void DockWindow::Resume() {
    if (!m_hwnd) return;
    m_hideProgress = 0.0f;
    m_targetHideProgress = 0.0f;
    m_isOverlapped = false;
    SetTimer(m_hwnd, TIMER_POLL_ID, 50, nullptr);
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateLayoutPosition();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DockWindow::AddItem(const std::wstring& name, const std::wstring& path, ItemAction action) {
    auto item = std::make_unique<DockItem>();
    item->name = name;
    item->targetPath = path;
    item->action = action;
    item->baseSize = m_baseSize;
    item->currentSize = m_baseSize;
    item->targetSize = m_baseSize;

    if (action != ItemAction::ToggleLaunchpad && action != ItemAction::OpenSettings) {
        item->LoadIconFromPath(m_d2d.GetWICFactory(), m_d2d.GetRenderTarget());
    }

    m_items.push_back(std::move(item));
    UpdateLayout();
}

bool DockWindow::PinApp(const std::wstring& name, const std::wstring& path) {
    return PinAppAtIndex(name, path, -1);
}

bool DockWindow::PinAppAtIndex(const std::wstring& name, const std::wstring& path, int targetIndex) {
    // Check if app already exists in dock
    int existingIndex = -1;
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->name == name || (!path.empty() && m_items[i]->targetPath == path)) {
            existingIndex = static_cast<int>(i);
            break;
        }
    }

    if (existingIndex != -1) {
        // Already pinned: if targetIndex specified, move it to targetIndex!
        if (targetIndex >= 0 && targetIndex < static_cast<int>(m_items.size()) && targetIndex != existingIndex) {
            auto item = std::move(m_items[existingIndex]);
            m_items.erase(m_items.begin() + existingIndex);
            if (targetIndex > existingIndex) targetIndex--;
            m_items.insert(m_items.begin() + targetIndex, std::move(item));
            m_externalDragActive = false;
            m_insertIndex = -1;
            m_insertGap = 0.0f;
            m_targetInsertGap = 0.0f;
            UpdateLayout();
            SaveConfig();
            return true;
        }
        m_externalDragActive = false;
        m_insertIndex = -1;
        m_insertGap = 0.0f;
        m_targetInsertGap = 0.0f;
        UpdateLayout();
        return false;
    }

    auto item = std::make_unique<DockItem>();
    item->name = name;
    item->targetPath = path;
    item->action = ItemAction::LaunchApp;
    item->baseSize = m_baseSize;
    // Start with physical landing scale for smooth bounce!
    item->currentSize = m_baseSize * 1.35f;
    item->targetSize = m_baseSize;
    item->LoadIconFromPath(m_d2d.GetWICFactory(), m_d2d.GetRenderTarget());

    if (targetIndex >= 0 && targetIndex <= static_cast<int>(m_items.size())) {
        m_items.insert(m_items.begin() + targetIndex, std::move(item));
    } else {
        // Default insertion: before settings and recycle bin
        auto it = m_items.end();
        for (auto i = m_items.begin(); i != m_items.end(); ++i) {
            if ((*i)->action == ItemAction::OpenSettings || (*i)->action == ItemAction::RecycleBin) {
                it = i;
                break;
            }
        }
        m_items.insert(it, std::move(item));
    }

    m_externalDragActive = false;
    m_insertIndex = -1;
    m_insertGap = 0.0f;
    m_targetInsertGap = 0.0f;

    UpdateLayout();
    SaveConfig();

    if (!m_isAnimating) {
        SetTimer(m_hwnd, TIMER_ANIM_ID, 16, nullptr);
        m_isAnimating = true;
    }
    return true;
}

void DockWindow::OnExternalDragMove(int screenX, int screenY) {
    RECT winRect = {};
    GetWindowRect(m_hwnd, &winRect);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    bool isNearBottom = (screenY >= winRect.top - 60 && screenY <= screenHeight);
    bool isHorzOver = (screenX >= winRect.left - 50 && screenX <= winRect.right + 50);

    if (isNearBottom) {
        m_targetHideProgress = 0.0f;
        m_hideGraceFrames = 25;
        if (!m_isSlideAnimating) {
            SetTimer(m_hwnd, TIMER_SLIDE_ID, 16, nullptr);
            m_isSlideAnimating = true;
        }
    }

    if (isNearBottom && isHorzOver && !m_items.empty()) {
        m_externalDragActive = true;
        float localX = static_cast<float>(screenX - winRect.left);

        if (localX <= m_items[0]->centerX) {
            m_insertIndex = 0;
        } else if (localX >= m_items.back()->centerX) {
            m_insertIndex = static_cast<int>(m_items.size());
        } else {
            m_insertIndex = static_cast<int>(m_items.size());
            for (size_t i = 0; i < m_items.size() - 1; ++i) {
                float midX = (m_items[i]->centerX + m_items[i + 1]->centerX) / 2.0f;
                if (localX < midX) {
                    m_insertIndex = static_cast<int>(i + 1);
                    break;
                }
            }
        }
        m_targetInsertGap = m_baseSize + m_padding;
    } else {
        m_externalDragActive = false;
        m_insertIndex = -1;
        m_targetInsertGap = 0.0f;
    }

    if (!m_isAnimating) {
        SetTimer(m_hwnd, TIMER_ANIM_ID, 16, nullptr);
        m_isAnimating = true;
    }
}

void DockWindow::OnExternalDragLeave() {
    m_externalDragActive = false;
    m_insertIndex = -1;
    m_targetInsertGap = 0.0f;
    if (!m_isAnimating) {
        SetTimer(m_hwnd, TIMER_ANIM_ID, 16, nullptr);
        m_isAnimating = true;
    }
}

void DockWindow::TriggerVanishEffect(int itemIndex) {
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_items.size())) return;
    auto& item = m_items[itemIndex];

    VanishEffect effect;
    effect.centerX = item->centerX;
    effect.centerY = item->centerY;
    effect.progress = 0.0f;
    effect.pBitmap = item->pBitmap;
    if (effect.pBitmap) {
        effect.pBitmap->AddRef();
    }

    // Generate 8 smoke puff particles spreading outward in a circle
    for (int i = 0; i < 8; ++i) {
        float angle = (i / 8.0f) * 2.0f * PI + (rand() % 100 / 100.0f) * 0.4f;
        float speed = 1.8f + (rand() % 100 / 100.0f) * 2.2f;
        SmokeParticle p;
        p.x = effect.centerX + std::cos(angle) * 8.0f;
        p.y = effect.centerY + std::sin(angle) * 8.0f;
        p.vx = std::cos(angle) * speed;
        p.vy = std::sin(angle) * speed;
        p.radius = 8.0f + (rand() % 100 / 100.0f) * 5.0f;
        p.alpha = 0.85f;
        effect.particles.push_back(p);
    }

    m_vanishEffects.push_back(effect);

    m_items.erase(m_items.begin() + itemIndex);
    UpdateLayout();
    SaveConfig();

    if (!m_isAnimating) {
        SetTimer(m_hwnd, TIMER_ANIM_ID, 16, nullptr);
        m_isAnimating = true;
    }
}

void DockWindow::LoadConfig() {
    auto& cfg = Settings::SettingsManager::Instance().Config();
    if (!cfg.pinnedApps.empty()) {
        m_items.clear();
        for (const auto& item : cfg.pinnedApps) {
            ItemAction act = ItemAction::LaunchApp;
            if (item.action == L"launchpad") act = ItemAction::ToggleLaunchpad;
            else if (item.action == L"settings") act = ItemAction::OpenSettings;
            else if (item.action == L"recyclebin" || item.name == L"回收站" || item.path == L"shell:RecycleBinFolder") act = ItemAction::RecycleBin;

            AddItem(item.name, item.path, act);
        }
        return;
    }

    // Default items
    m_items.clear();
    AddItem(L"启动台", L"", ItemAction::ToggleLaunchpad);
    AddItem(L"资源管理器", L"C:\\Windows\\explorer.exe", ItemAction::LaunchApp);
    AddItem(L"回收站", L"shell:RecycleBinFolder", ItemAction::RecycleBin);
    AddItem(L"设置", L"", ItemAction::OpenSettings);
}

void DockWindow::SaveConfig() {
    auto& cfg = Settings::SettingsManager::Instance().Config();
    cfg.pinnedApps.clear();

    for (size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        Settings::PinnedAppConfig pa;
        pa.name = item->name;
        pa.path = item->targetPath;
        if (item->action == ItemAction::ToggleLaunchpad) {
            pa.action = L"launchpad";
            pa.icon = L"launchpad";
        } else if (item->action == ItemAction::OpenSettings) {
            pa.action = L"settings";
            pa.icon = L"settings";
        } else if (item->action == ItemAction::RecycleBin) {
            pa.action = L"recyclebin";
            pa.path = L"shell:RecycleBinFolder";
        } else {
            pa.action = L"launch";
        }
        cfg.pinnedApps.push_back(pa);
    }

    std::wstring configPath = GetExecutableDir() + L"\\config.json";
    cfg.SaveToFile(configPath);
}

void DockWindow::DrawLaunchpadIcon(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect) {
    float width = rect.right - rect.left;
    float height = rect.bottom - rect.top;
    float size = (width < height) ? width : height;
    float radius = size * 0.22f;

    // 1. Base squircle
    D2D1_ROUNDED_RECT baseSquircle = D2D1::RoundedRect(rect, radius, radius);
    ID2D1SolidColorBrush* pBrush = nullptr;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.13f, 0.15f, 0.21f, 0.95f), &pBrush);
    if (pBrush) {
        rt->FillRoundedRectangle(baseSquircle, pBrush);
        pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.22f));
        rt->DrawRoundedRectangle(baseSquircle, pBrush, 1.2f);
        pBrush->Release();
    }

    // 2. 3x3 colorful app grid inside
    float margin = size * 0.19f;
    float gridW = size - margin * 2.0f;
    float tileSize = gridW / 3.6f;
    float gap = (gridW - 3.0f * tileSize) / 2.0f;
    float tileRadius = tileSize * 0.28f;

    const D2D1_COLOR_F colors[3][3] = {
        { D2D1::ColorF(0.18f, 0.54f, 0.98f, 1.0f), D2D1::ColorF(0.96f, 0.32f, 0.35f, 1.0f), D2D1::ColorF(0.18f, 0.80f, 0.44f, 1.0f) },
        { D2D1::ColorF(0.98f, 0.60f, 0.12f, 1.0f), D2D1::ColorF(0.65f, 0.35f, 0.98f, 1.0f), D2D1::ColorF(0.15f, 0.78f, 0.90f, 1.0f) },
        { D2D1::ColorF(0.95f, 0.32f, 0.65f, 1.0f), D2D1::ColorF(0.98f, 0.80f, 0.18f, 1.0f), D2D1::ColorF(0.32f, 0.45f, 0.98f, 1.0f) }
    };

    ID2D1SolidColorBrush* pTileBrush = nullptr;
    rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &pTileBrush);
    if (pTileBrush) {
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                float tLeft = rect.left + margin + c * (tileSize + gap);
                float tTop = rect.top + margin + r * (tileSize + gap);
                D2D1_ROUNDED_RECT tile = D2D1::RoundedRect(
                    D2D1::RectF(tLeft, tTop, tLeft + tileSize, tTop + tileSize),
                    tileRadius,
                    tileRadius
                );
                pTileBrush->SetColor(colors[r][c]);
                rt->FillRoundedRectangle(tile, pTileBrush);
            }
        }
        pTileBrush->Release();
    }
}

void DockWindow::DrawSettingsIcon(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect) {
    float width = rect.right - rect.left;
    float height = rect.bottom - rect.top;
    float size = (width < height) ? width : height;
    float radius = size * 0.22f;

    // 1. Base squircle
    D2D1_ROUNDED_RECT baseSquircle = D2D1::RoundedRect(rect, radius, radius);
    ID2D1SolidColorBrush* pBrush = nullptr;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.18f, 0.20f, 0.25f, 0.95f), &pBrush);
    if (pBrush) {
        rt->FillRoundedRectangle(baseSquircle, pBrush);
        pBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.22f));
        rt->DrawRoundedRectangle(baseSquircle, pBrush, 1.2f);
    }

    // 2. Precision mechanical gear
    float cx = (rect.left + rect.right) / 2.0f;
    float cy = (rect.top + rect.bottom) / 2.0f;
    float outerR = size * 0.25f;
    float holeR = size * 0.08f;
    float toothW = size * 0.10f;
    float toothH = size * 0.12f;

    D2D1_MATRIX_3X2_F oldTransform;
    rt->GetTransform(&oldTransform);

    ID2D1SolidColorBrush* pGearBrush = nullptr;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.88f, 0.92f, 0.96f), &pGearBrush);

    if (pGearBrush) {
        for (int i = 0; i < 8; ++i) {
            float angle = i * 45.0f;
            rt->SetTransform(D2D1::Matrix3x2F::Rotation(angle, D2D1::Point2F(cx, cy)) * oldTransform);
            D2D1_ROUNDED_RECT tooth = D2D1::RoundedRect(
                D2D1::RectF(cx - toothW / 2.0f, cy - outerR - toothH * 0.45f, cx + toothW / 2.0f, cy - outerR + toothH * 0.55f),
                2.5f,
                2.5f
            );
            rt->FillRoundedRectangle(tooth, pGearBrush);
        }

        rt->SetTransform(oldTransform);

        D2D1_ELLIPSE body = D2D1::Ellipse(D2D1::Point2F(cx, cy), outerR, outerR);
        rt->FillEllipse(body, pGearBrush);

        pGearBrush->SetColor(D2D1::ColorF(0.18f, 0.20f, 0.25f, 1.0f));
        D2D1_ELLIPSE hole = D2D1::Ellipse(D2D1::Point2F(cx, cy), holeR, holeR);
        rt->FillEllipse(hole, pGearBrush);

        pGearBrush->SetColor(D2D1::ColorF(0.85f, 0.88f, 0.92f, 0.96f));
        D2D1_ELLIPSE axle = D2D1::Ellipse(D2D1::Point2F(cx, cy), holeR * 0.45f, holeR * 0.45f);
        rt->FillEllipse(axle, pGearBrush);

        pGearBrush->Release();
    }

    if (pBrush) pBrush->Release();
}

void DockWindow::OnRButtonUp(int x, int y) {
    int clickedIndex = -1;
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (std::abs(static_cast<float>(x) - m_items[i]->centerX) < m_items[i]->currentSize / 2.0f &&
            std::abs(static_cast<float>(y) - m_items[i]->centerY) < m_items[i]->currentSize / 2.0f) {
            clickedIndex = static_cast<int>(i);
            break;
        }
    }

    RECT winRect = {};
    GetWindowRect(m_hwnd, &winRect);

    float barBottom = m_barHeight - 6.0f;
    float barTop = barBottom - m_barVisualHeight;

    if (clickedIndex >= 0 && clickedIndex < static_cast<int>(m_items.size())) {
        auto& item = m_items[clickedIndex];
        HMENU hMenu = CreatePopupMenu();
        std::wstring openText = L"打开 " + item->name;
        AppendMenuW(hMenu, MF_STRING, 1001, openText.c_str());

        if (item->action == ItemAction::LaunchApp) {
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, 1002, L"从程序坞中移除");
        } else if (item->action == ItemAction::RecycleBin) {
            SHQUERYRBINFO qrb = { sizeof(SHQUERYRBINFO) };
            bool hasItems = (SHQueryRecycleBinW(nullptr, &qrb) == S_OK && qrb.i64NumItems > 0);
            UINT emptyFlags = hasItems ? MF_STRING : (MF_STRING | MF_DISABLED | MF_GRAYED);
            AppendMenuW(hMenu, emptyFlags, 1003, L"清空回收站");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, 1002, L"从程序坞中移除");
        }

        // Position popup cleanly ABOVE the dock and centered horizontally on the clicked icon
        int itemScreenCenterX = winRect.left + static_cast<int>(item->centerX);
        float iconBaseline = barBottom - (m_barVisualHeight - m_baseSize) / 2.0f;
        float iconTop = iconBaseline - item->currentSize;
        float visualTopY = (std::min)(barTop, iconTop);
        int menuScreenY = winRect.top + static_cast<int>(visualTopY) - 10;

        m_isContextMenuOpen = true;
        SetForegroundWindow(m_hwnd);

        int cmd = TrackPopupMenuEx(
            hMenu,
            TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_CENTERALIGN,
            itemScreenCenterX,
            menuScreenY,
            m_hwnd,
            nullptr
        );

        DestroyMenu(hMenu);
        PostMessageW(m_hwnd, WM_NULL, 0, 0);
        m_isContextMenuOpen = false;

        // Check if mouse left dock while menu was active
        POINT curPt;
        GetCursorPos(&curPt);
        if (!PtInRect(&winRect, curPt)) {
            OnMouseLeave();
            if (!m_isAnimating) {
                SetTimer(m_hwnd, TIMER_ANIM_ID, 16, nullptr);
                m_isAnimating = true;
            }
        }

        if (cmd == 1001) {
            if (item->action == ItemAction::LaunchApp) {
                ShellExecuteW(nullptr, L"open", item->targetPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            } else if (item->action == ItemAction::ToggleLaunchpad && m_onToggleLaunchpad) {
                m_onToggleLaunchpad();
            } else if (item->action == ItemAction::OpenSettings) {
                if (m_onOpenSettings) {
                    m_onOpenSettings();
                } else {
                    MessageBoxW(m_hwnd, L"设置面板模块就绪！", L"设置", MB_OK | MB_ICONINFORMATION);
                }
            } else if (item->action == ItemAction::RecycleBin) {
                ShellExecuteW(nullptr, L"open", L"shell:RecycleBinFolder", nullptr, nullptr, SW_SHOWNORMAL);
            }
        } else if (cmd == 1002) {
            TriggerVanishEffect(clickedIndex);
        } else if (cmd == 1003 && item->action == ItemAction::RecycleBin) {
            if (SUCCEEDED(SHEmptyRecycleBinW(m_hwnd, nullptr, 0))) {
                item->ReloadIcon(m_d2d.GetWICFactory(), m_d2d.GetRenderTarget());
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        }
    } else {
        // Right clicked blank area of the dock bar
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING | MF_DISABLED, 0, L"Mac 桌面美化");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, 2001, L"打开启动台");
        AppendMenuW(hMenu, MF_STRING, 2002, L"设置");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, 2003, L"退出程序");

        int clickScreenX = winRect.left + x;
        int menuScreenY = winRect.top + static_cast<int>(barTop) - 10;

        m_isContextMenuOpen = true;
        SetForegroundWindow(m_hwnd);

        int cmd = TrackPopupMenuEx(
            hMenu,
            TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_CENTERALIGN,
            clickScreenX,
            menuScreenY,
            m_hwnd,
            nullptr
        );

        DestroyMenu(hMenu);
        PostMessageW(m_hwnd, WM_NULL, 0, 0);
        m_isContextMenuOpen = false;

        POINT curPt;
        GetCursorPos(&curPt);
        if (!PtInRect(&winRect, curPt)) {
            OnMouseLeave();
            if (!m_isAnimating) {
                SetTimer(m_hwnd, TIMER_ANIM_ID, 16, nullptr);
                m_isAnimating = true;
            }
        }

        if (cmd == 2001 && m_onToggleLaunchpad) {
            m_onToggleLaunchpad();
        } else if (cmd == 2002) {
            if (m_onOpenSettings) {
                m_onOpenSettings();
            }
        } else if (cmd == 2003) {
            PostQuitMessage(0);
        }
    }
}

void DockWindow::UpdateLayout() {
    if (m_items.empty()) return;

    // Calculate total width generously with end padding, inter-item spacing and insertion gap
    float totalItemWidth = 0.0f;
    for (const auto& item : m_items) {
        totalItemWidth += item->currentSize;
    }
    m_barWidth = totalItemWidth + (m_items.size() - 1) * m_padding + m_endPadding * 2.0f + m_insertGap;

    UpdateLayoutPosition();
}

void DockWindow::UpdateLayoutPosition() {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    RECT workArea;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

    int restingY = static_cast<int>(workArea.bottom - m_barHeight - m_bottomMargin);
    // When hidden, dock slides below bottom of screen, leaving top 2px as touch trigger
    int hiddenY = screenHeight - 2;

    int currentY = static_cast<int>(restingY + (hiddenY - restingY) * m_hideProgress);
    int winX = static_cast<int>((screenWidth - m_barWidth) / 2);
    int winW = static_cast<int>(m_barWidth);
    int winH = static_cast<int>(m_barHeight);

    SetWindowPos(m_hwnd, nullptr, winX, currentY, winW, winH, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
    m_d2d.Resize(winW, winH);

    // Compute center X of each icon (leaving insertion gap if dragging over dock)
    float currentX = m_endPadding;
    float barBottom = m_barHeight - 6.0f;
    float iconBaseline = barBottom - (m_barVisualHeight - m_baseSize) / 2.0f;

    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_insertIndex >= 0 && static_cast<int>(i) == m_insertIndex && m_insertGap > 0.0f) {
            currentX += m_insertGap;
        }
        m_items[i]->centerX = currentX + m_items[i]->currentSize / 2.0f;
        m_items[i]->centerY = iconBaseline - m_items[i]->currentSize / 2.0f;
        currentX += m_items[i]->currentSize + m_padding;
    }

    InvalidateRect(m_hwnd, nullptr, FALSE);
}

struct OverlapContext {
    HWND dockHwnd;
    int dockTop;
    int dockLeft;
    int dockRight;
    int screenHeight;
    bool hasOverlap;
};

static BOOL CALLBACK EnumOverlapProc(HWND hwnd, LPARAM lParam) {
    auto* ctx = reinterpret_cast<OverlapContext*>(lParam);
    if (!hwnd || hwnd == ctx->dockHwnd) return TRUE;

    if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) return TRUE;

    // Filter invisible or non-app styles
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    if (!(style & WS_VISIBLE)) return TRUE;

    // Tool windows without taskbar entry are usually popups/tooltips, ignore unless large
    LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    if ((exStyle & WS_EX_TOOLWINDOW) && !(exStyle & WS_EX_APPWINDOW)) {
        return TRUE;
    }

    wchar_t cls[64] = {};
    GetClassNameW(hwnd, cls, 63);
    // Ignore desktop wallpaper and system shells
    if (wcscmp(cls, L"Progman") == 0 || wcscmp(cls, L"WorkerW") == 0 ||
        wcscmp(cls, L"Shell_TrayWnd") == 0 || wcscmp(cls, L"Shell_SecondaryTrayWnd") == 0) {
        return TRUE;
    }
    // Ignore our own windows
    if (wcscmp(cls, L"MinimalMacDockWindow") == 0 || 
        wcscmp(cls, L"MinimalMacDrawerWindow") == 0 || 
        wcscmp(cls, L"MinimalDesktopWidgetHost") == 0 ||
        wcscmp(cls, L"MinimalMacSettingsWindow") == 0) {
        return TRUE;
    }

    // 1. If any visible application window is maximized, it definitely overlaps the bottom
    if (IsZoomed(hwnd)) {
        ctx->hasOverlap = true;
        return FALSE; // Stop enumeration
    }

    // 2. Check window bounds
    RECT rc;
    if (GetWindowRect(hwnd, &rc)) {
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        if (w >= 200 && h >= 150) {
            // Window bottom enters the dock vertical area
            if (rc.bottom > ctx->dockTop && rc.top < ctx->screenHeight) {
                // Window horizontally intersects the dock area
                if (rc.right > ctx->dockLeft && rc.left < ctx->dockRight) {
                    ctx->hasOverlap = true;
                    return FALSE; // Found overlap!
                }
            }
        }
    }

    return TRUE;
}

bool DockWindow::CheckOverlap() {
    // If Launchpad is currently open, do not auto-hide the dock
    if (m_isLaunchpadOpen && m_isLaunchpadOpen()) {
        return false;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int dockTop = screenHeight - static_cast<int>(m_barVisualHeight + m_bottomMargin + 25.0f);

    RECT dockRect;
    GetWindowRect(m_hwnd, &dockRect);

    // Left and right bounds of the dock bar with 20px margin
    int dockLeft = dockRect.left - 20;
    int dockRight = dockRect.right + 20;

    OverlapContext ctx;
    ctx.dockHwnd = m_hwnd;
    ctx.dockTop = dockTop;
    ctx.dockLeft = dockLeft;
    ctx.dockRight = dockRight;
    ctx.screenHeight = screenHeight;
    ctx.hasOverlap = false;

    EnumWindows(EnumOverlapProc, reinterpret_cast<LPARAM>(&ctx));

    return ctx.hasOverlap;
}

void DockWindow::ApplySettings(float baseSize, float maxSize, float spread, float bottomMargin, 
                               float barVisualHeight, float bgOpacity, bool showDot, bool showTooltip, bool autoHide) {
    m_baseSize = baseSize;
    m_maxSize = maxSize;
    m_spread = spread;
    m_bottomMargin = bottomMargin;
    m_barVisualHeight = barVisualHeight;
    m_bgOpacity = bgOpacity;
    m_showDot = showDot;
    m_showTooltip = showTooltip;
    m_autoHideEnabled = autoHide;

    if (m_pBrushBg) {
        m_pBrushBg->SetColor(D2D1::ColorF(m_themeBgColor.r, m_themeBgColor.g, m_themeBgColor.b, m_bgOpacity));
    }

    for (auto& item : m_items) {
        item->baseSize = m_baseSize;
        item->currentSize = m_baseSize;
        item->targetSize = m_baseSize;
    }

    UpdateLayout();
    UpdateLayoutPosition();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DockWindow::ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, float cornerRadius) {
    m_themeBgColor = bg;
    m_themeBorderColor = border;
    m_cornerRadius = cornerRadius;

    if (m_pBrushBg) {
        m_pBrushBg->SetColor(D2D1::ColorF(bg.r, bg.g, bg.b, m_bgOpacity));
    }
    if (m_pBrushBorder) {
        m_pBrushBorder->SetColor(border);
    }
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void DockWindow::UpdateAutoHide() {
    if (!m_autoHideEnabled) {
        if (m_hideProgress > 0.0f) {
            m_targetHideProgress = 0.0f;
            SetTimer(m_hwnd, TIMER_SLIDE_ID, 16, nullptr);
            m_isSlideAnimating = true;
        }
        return;
    }

    if (m_isContextMenuOpen) {
        return;
    }

    m_isOverlapped = CheckOverlap();

    POINT pt;
    GetCursorPos(&pt);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    RECT winRect;
    GetWindowRect(m_hwnd, &winRect);
    bool isMouseOverDock = PtInRect(&winRect, pt);
    // Mouse within bottom 8px of screen triggers reveal
    bool isMouseAtBottom = (pt.y >= screenHeight - 8);

    if (!m_isOverlapped) {
        // On desktop: dock always stays fully visible
        m_targetHideProgress = 0.0f;
        m_hideGraceFrames = 0;
    } else {
        // When overlapping window exists
        if (isMouseOverDock || isMouseAtBottom || m_isMouseInside) {
            m_targetHideProgress = 0.0f;
            m_hideGraceFrames = 10; // ~500ms grace period after mouse leaves
        } else {
            if (m_hideGraceFrames > 0) {
                m_hideGraceFrames--;
            } else {
                m_targetHideProgress = 1.0f;
            }
        }
    }

    if (m_hideProgress != m_targetHideProgress) {
        if (!m_isSlideAnimating) {
            SetTimer(m_hwnd, TIMER_SLIDE_ID, 16, nullptr);
            m_isSlideAnimating = true;
        }
    }
}

void DockWindow::AnimateSlide() {
    float diff = m_targetHideProgress - m_hideProgress;
    if (std::abs(diff) > 0.005f) {
        // Silky smooth ease-out interpolation
        m_hideProgress += diff * 0.22f;
    } else {
        m_hideProgress = m_targetHideProgress;
    }

    UpdateLayoutPosition();

    if (m_hideProgress == m_targetHideProgress) {
        KillTimer(m_hwnd, TIMER_SLIDE_ID);
        m_isSlideAnimating = false;
    }
}

void DockWindow::OnMouseMove(int x, int y) {
    if (m_isContextMenuOpen) {
        return;
    }

    // Internal icon dragging (reorder within dock or drag out to remove)
    if (m_dragItemIndex >= 0 && m_dragItemIndex < static_cast<int>(m_items.size())) {
        int dx = x - m_dragStartPos.x;
        int dy = y - m_dragStartPos.y;
        if (!m_isDraggingItem && (dx * dx + dy * dy) > 25) { // 5px drag activation threshold
            m_isDraggingItem = true;
        }

        if (m_isDraggingItem) {
            m_dragItemCurrentX = static_cast<float>(x);
            m_dragItemCurrentY = static_cast<float>(y);

            // Dragged far above or below the dock onto the desktop
            m_isDragItemOutOfDock = (y < -35 || y > m_barHeight + 50);

            if (!m_isDragItemOutOfDock) {
                // Reorder: swap with left neighbor
                if (m_dragItemIndex > 0 && x < m_items[m_dragItemIndex - 1]->centerX) {
                    std::swap(m_items[m_dragItemIndex], m_items[m_dragItemIndex - 1]);
                    m_dragItemIndex--;
                    UpdateLayout();
                }
                // Reorder: swap with right neighbor
                else if (m_dragItemIndex < static_cast<int>(m_items.size()) - 1 && x > m_items[m_dragItemIndex + 1]->centerX) {
                    std::swap(m_items[m_dragItemIndex], m_items[m_dragItemIndex + 1]);
                    m_dragItemIndex++;
                    UpdateLayout();
                }
            }
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        }
    }

    m_mouseX = x;
    m_mouseY = y;
    m_isMouseInside = true;

    if (!m_isMouseTracking) {
        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hwnd;
        TrackMouseEvent(&tme);
        m_isMouseTracking = true;
    }

    if (!m_isAnimating) {
        SetTimer(m_hwnd, TIMER_ANIM_ID, 16, nullptr);
        m_isAnimating = true;
    }

    // Calculate target sizes for all icons based on distance to mouse X
    m_hoveredIndex = -1;
    for (size_t i = 0; i < m_items.size(); ++i) {
        auto& item = m_items[i];
        float dist = std::abs(static_cast<float>(x) - item->centerX);
        if (dist < m_spread) {
            float factor = std::cos((dist / m_spread) * (PI / 2.0f));
            factor = factor * factor; // Cosine squared smoothing
            item->targetSize = m_baseSize + (m_maxSize - m_baseSize) * factor;
        } else {
            item->targetSize = m_baseSize;
        }

        // Check hover
        if (std::abs(static_cast<float>(x) - item->centerX) < item->currentSize / 2.0f &&
            std::abs(static_cast<float>(y) - item->centerY) < item->currentSize / 2.0f) {
            m_hoveredIndex = static_cast<int>(i);
        }
    }
}

void DockWindow::OnMouseLeave() {
    if (m_isContextMenuOpen) {
        return;
    }

    m_isMouseTracking = false;
    m_isMouseInside = false;
    m_mouseX = -1000;
    m_mouseY = -1000;
    m_hoveredIndex = -1;

    for (auto& item : m_items) {
        item->targetSize = m_baseSize;
    }
}

void DockWindow::AnimateStep() {
    bool hasDelta = false;
    for (auto& item : m_items) {
        float diff = item->targetSize - item->currentSize;
        if (std::abs(diff) > 0.35f) {
            item->currentSize += diff * 0.35f;
            hasDelta = true;
        } else {
            item->currentSize = item->targetSize;
        }
    }

    // Smoothly animate insertion gap
    float gapDiff = m_targetInsertGap - m_insertGap;
    if (std::abs(gapDiff) > 0.4f) {
        m_insertGap += gapDiff * 0.30f;
        hasDelta = true;
    } else {
        m_insertGap = m_targetInsertGap;
    }

    // Animate active vanishing smoke effects
    for (auto it = m_vanishEffects.begin(); it != m_vanishEffects.end(); ) {
        it->progress += 0.05f; // ~20 frames (~330ms)
        for (auto& p : it->particles) {
            p.x += p.vx;
            p.y += p.vy;
            p.radius += 0.45f;
            p.alpha = (1.0f - it->progress) * 0.85f;
        }
        if (it->progress >= 1.0f) {
            if (it->pBitmap) it->pBitmap->Release();
            it = m_vanishEffects.erase(it);
        } else {
            ++it;
            hasDelta = true;
        }
    }

    // Update layout smoothly
    float totalItemWidth = 0.0f;
    for (const auto& item : m_items) {
        totalItemWidth += item->currentSize;
    }
    m_barWidth = totalItemWidth + (m_items.size() - 1) * m_padding + m_endPadding * 2.0f + m_insertGap;

    UpdateLayoutPosition();
    Render();

    if (!hasDelta && !m_isMouseInside && !m_externalDragActive && m_vanishEffects.empty() && !m_isDraggingItem) {
        KillTimer(m_hwnd, TIMER_ANIM_ID);
        m_isAnimating = false;
    }
}

void DockWindow::OnLButtonDown(int x, int y) {
    m_dragItemIndex = -1;
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (std::abs(static_cast<float>(x) - m_items[i]->centerX) < m_items[i]->currentSize / 2.0f &&
            std::abs(static_cast<float>(y) - m_items[i]->centerY) < m_items[i]->currentSize / 2.0f) {
            m_dragItemIndex = static_cast<int>(i);
            m_dragStartPos = { x, y };
            m_isDraggingItem = false;
            m_isDragItemOutOfDock = false;
            SetCapture(m_hwnd);
            break;
        }
    }
}

void DockWindow::OnLButtonUp(int x, int y) {
    if (GetCapture() == m_hwnd) {
        ReleaseCapture();
    }

    if (m_isDraggingItem && m_dragItemIndex >= 0 && m_dragItemIndex < static_cast<int>(m_items.size())) {
        if (m_isDragItemOutOfDock) {
            // Dragged out onto desktop: remove with smoke poof effect!
            if (m_items[m_dragItemIndex]->action != ItemAction::ToggleLaunchpad) {
                TriggerVanishEffect(m_dragItemIndex);
            }
        } else {
            // Reordering finished! Save new layout order
            SaveConfig();
        }

        m_isDraggingItem = false;
        m_dragItemIndex = -1;
        m_isDragItemOutOfDock = false;
        UpdateLayout();
        return;
    }

    m_isDraggingItem = false;
    m_dragItemIndex = -1;
    m_isDragItemOutOfDock = false;

    for (auto& item : m_items) {
        if (std::abs(static_cast<float>(x) - item->centerX) < item->currentSize / 2.0f &&
            std::abs(static_cast<float>(y) - item->centerY) < item->currentSize / 2.0f) {
            
            if (item->action == ItemAction::LaunchApp) {
                ShellExecuteW(nullptr, L"open", item->targetPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            } else if (item->action == ItemAction::ToggleLaunchpad) {
                if (m_onToggleLaunchpad) {
                    m_onToggleLaunchpad();
                }
            } else if (item->action == ItemAction::OpenSettings) {
                if (m_onOpenSettings) {
                    m_onOpenSettings();
                } else {
                    MessageBoxW(m_hwnd, L"设置面板模块就绪！", L"设置", MB_OK | MB_ICONINFORMATION);
                }
            } else if (item->action == ItemAction::RecycleBin) {
                ShellExecuteW(nullptr, L"open", L"shell:RecycleBinFolder", nullptr, nullptr, SW_SHOWNORMAL);
            }
            break;
        }
    }
}

void DockWindow::Render() {
    auto rt = m_d2d.GetRenderTarget();
    if (!rt) return;

    m_d2d.BeginDraw();
    rt->Clear(D2D1::ColorF(0, 0, 0, 0.0f));

    // 1. Draw rounded dock bar background (clean, no outer border)
    float barBottom = m_barHeight - 6.0f;
    float barTop = barBottom - m_barVisualHeight;

    D2D1_ROUNDED_RECT roundedBar = D2D1::RoundedRect(
        D2D1::RectF(0.0f, barTop, m_barWidth, barBottom),
        m_cornerRadius,
        m_cornerRadius
    );

    rt->FillRoundedRectangle(roundedBar, m_pBrushBg);
    rt->DrawRoundedRectangle(roundedBar, m_pBrushBorder, 1.2f);

    float iconBaseline = barBottom - (m_barVisualHeight - m_baseSize) / 2.0f;

    // Draw Insertion Placeholder Slot if drag is active over dock
    if (m_insertGap > 10.0f && m_insertIndex >= 0 && !m_items.empty()) {
        float gapCenterX = 0.0f;
        if (m_insertIndex == 0) {
            gapCenterX = m_endPadding + m_insertGap / 2.0f;
        } else if (m_insertIndex >= static_cast<int>(m_items.size())) {
            gapCenterX = m_items.back()->centerX + m_items.back()->currentSize / 2.0f + m_padding + m_insertGap / 2.0f;
        } else {
            gapCenterX = m_items[m_insertIndex - 1]->centerX + m_items[m_insertIndex - 1]->currentSize / 2.0f + m_padding + m_insertGap / 2.0f;
        }

        float slotHalf = (std::min)(m_insertGap * 0.45f, m_baseSize * 0.48f);
        D2D1_ROUNDED_RECT slotRect = D2D1::RoundedRect(
            D2D1::RectF(gapCenterX - slotHalf, iconBaseline - m_baseSize, gapCenterX + slotHalf, iconBaseline),
            14.0f, 14.0f
        );

        if (m_pBrushInsertSlot) rt->FillRoundedRectangle(slotRect, m_pBrushInsertSlot);
        if (m_pBrushBorder) rt->DrawRoundedRectangle(slotRect, m_pBrushBorder, 1.5f);
    }

    // 2. Draw Icons
    for (size_t idx = 0; idx < m_items.size(); ++idx) {
        const auto& item = m_items[idx];

        // If this item is actively being dragged internally, draw placeholder at its slot
        if (m_isDraggingItem && static_cast<int>(idx) == m_dragItemIndex) {
            float size = item->currentSize;
            float left = item->centerX - size / 2.0f;
            float bottom = iconBaseline;
            float top = bottom - size;
            D2D1_ROUNDED_RECT ghostSlot = D2D1::RoundedRect(D2D1::RectF(left, top, left + size, bottom), 14.0f, 14.0f);
            if (m_pBrushInsertSlot) rt->FillRoundedRectangle(ghostSlot, m_pBrushInsertSlot);
            continue;
        }

        float size = item->currentSize;
        float left = item->centerX - size / 2.0f;
        float bottom = iconBaseline;
        float top = bottom - size;

        D2D1_RECT_F iconRect = D2D1::RectF(left, top, left + size, bottom);

        if (item->action == ItemAction::ToggleLaunchpad) {
            DrawLaunchpadIcon(rt, iconRect);
        } else if (item->action == ItemAction::OpenSettings) {
            DrawSettingsIcon(rt, iconRect);
        } else if (item->pBitmap) {
            rt->DrawBitmap(
                item->pBitmap,
                iconRect,
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
            );
        } else {
            D2D1_ROUNDED_RECT fallbackRect = D2D1::RoundedRect(iconRect, 14.0f, 14.0f);
            rt->FillRoundedRectangle(fallbackRect, m_pBrushBorder);
        }

        // Draw running dot if active
        if (m_showDot && item->isRunning) {
            D2D1_ELLIPSE dot = D2D1::Ellipse(
                D2D1::Point2F(item->centerX, barBottom - 5.0f),
                2.8f,
                2.8f
            );
            rt->FillEllipse(dot, m_pBrushDot);
        }
    }

    // Draw internally dragged floating item
    if (m_isDraggingItem && m_dragItemIndex >= 0 && m_dragItemIndex < static_cast<int>(m_items.size())) {
        const auto& dragItem = m_items[m_dragItemIndex];
        float floatSize = m_baseSize * (m_isDragItemOutOfDock ? 0.85f : 1.15f);
        float floatAlpha = m_isDragItemOutOfDock ? 0.60f : 0.95f;
        D2D1_RECT_F floatRect = D2D1::RectF(
            m_dragItemCurrentX - floatSize / 2.0f,
            m_dragItemCurrentY - floatSize / 2.0f,
            m_dragItemCurrentX + floatSize / 2.0f,
            m_dragItemCurrentY + floatSize / 2.0f
        );

        if (dragItem->action == ItemAction::ToggleLaunchpad) {
            DrawLaunchpadIcon(rt, floatRect);
        } else if (dragItem->action == ItemAction::OpenSettings) {
            DrawSettingsIcon(rt, floatRect);
        } else if (dragItem->pBitmap) {
            rt->DrawBitmap(dragItem->pBitmap, floatRect, floatAlpha, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        }
    }

    // Draw active vanishing smoke effects
    for (const auto& effect : m_vanishEffects) {
        if (effect.pBitmap && effect.progress < 0.60f) {
            float fadeAlpha = (1.0f - effect.progress / 0.60f);
            float scale = (1.0f - effect.progress * 0.75f);
            float sz = m_baseSize * scale;
            rt->DrawBitmap(
                effect.pBitmap,
                D2D1::RectF(effect.centerX - sz / 2.0f, effect.centerY - sz / 2.0f, effect.centerX + sz / 2.0f, effect.centerY + sz / 2.0f),
                fadeAlpha,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
            );
        }

        if (m_pBrushSmoke) {
            for (const auto& p : effect.particles) {
                if (p.alpha > 0.03f) {
                    m_pBrushSmoke->SetColor(D2D1::ColorF(0.96f, 0.97f, 1.0f, p.alpha));
                    rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(p.x, p.y), p.radius, p.radius), m_pBrushSmoke);
                    m_pBrushSmoke->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, p.alpha * 0.35f));
                    rt->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(p.x, p.y), p.radius, p.radius), m_pBrushSmoke, 1.2f);
                }
            }
        }
    }

    // 3. Draw tooltip pill for hovered item
    if (m_showTooltip && !m_isContextMenuOpen && m_hoveredIndex >= 0 && m_hoveredIndex < static_cast<int>(m_items.size())) {
        const auto& item = m_items[m_hoveredIndex];
        float tipWidth = 88.0f;
        float tipHeight = 26.0f;
        float tipX = item->centerX - tipWidth / 2.0f;
        float iconTop = iconBaseline - item->currentSize;
        float tipY = iconTop - tipHeight - 8.0f;

        if (tipY < 4.0f) tipY = 4.0f;

        D2D1_ROUNDED_RECT tipRect = D2D1::RoundedRect(
            D2D1::RectF(tipX, tipY, tipX + tipWidth, tipY + tipHeight),
            8.0f,
            8.0f
        );

        rt->FillRoundedRectangle(tipRect, m_pBrushBg);
        rt->DrawRoundedRectangle(tipRect, m_pBrushBorder, 1.0f);

        if (m_pTextFormatTooltip) {
            rt->DrawTextW(
                item->name.c_str(),
                static_cast<UINT32>(item->name.length()),
                m_pTextFormatTooltip,
                D2D1::RectF(tipX, tipY, tipX + tipWidth, tipY + tipHeight),
                m_pBrushText
            );
        }
    }

    m_d2d.EndDraw();
}

LRESULT CALLBACK DockWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    DockWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<DockWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<DockWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT DockWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_MOUSEMOVE:
        OnMouseMove(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_MOUSELEAVE:
        OnMouseLeave();
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

    case WM_TIMER:
        if (wParam == TIMER_ANIM_ID) {
            AnimateStep();
        } else if (wParam == TIMER_SLIDE_ID) {
            AnimateSlide();
        } else if (wParam == TIMER_POLL_ID) {
            UpdateAutoHide();
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
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(m_hwnd, msg, wParam, lParam);
}

} // namespace Dock
