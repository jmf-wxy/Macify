#include "WidgetHostWindow.h"
#include "ClockWidget.h"
#include "DiskWidget.h"
#include "SysMonitorWidget.h"
#include "CalendarWidget.h"
#include "MemoWidget.h"
#include "../core/Backdrop.h"
#include <algorithm>
#include <cmath>

namespace Widgets {

constexpr UINT_PTR TIMER_UPDATE_ID = 2001;
constexpr float RESIZE_GRIP_SIZE = 18.0f;

WidgetHostWindow::WidgetHostWindow()
    : m_hwnd(nullptr)
    , m_lockPosition(false)
    , m_dragMode(DragMode::None)
    , m_activeWidget(nullptr)
    , m_dragStartMouse({})
    , m_dragStartWidgetX(0.0f)
    , m_dragStartWidgetY(0.0f)
    , m_dragStartWidgetW(0.0f)
    , m_dragStartWidgetH(0.0f)
    , m_pBrushGrip(nullptr) {
}

WidgetHostWindow::~WidgetHostWindow() {
    if (m_hwnd) {
        KillTimer(m_hwnd, TIMER_UPDATE_ID);
    }
    if (m_pBrushGrip) {
        m_pBrushGrip->Release();
    }
}

bool WidgetHostWindow::Create() {
    const wchar_t CLASS_NAME[] = L"MinimalDesktopWidgetHost";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WidgetHostWindow::WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    // Cover the full desktop area so widgets can be freely moved and resized anywhere on screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    m_hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        CLASS_NAME,
        L"DesktopWidgetsHost",
        WS_POPUP,
        0,
        0,
        screenWidth,
        screenHeight,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );

    if (!m_hwnd) return false;

    // Enable complete transparency and remove any borders
    Core::RemoveWindowBorder(m_hwnd);

    // Place at bottom of Z-order (desktop level, below normal active windows)
    SetWindowPos(m_hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // Initialize Direct2D
    if (!m_d2d.Initialize(m_hwnd)) return false;

    auto rt = m_d2d.GetRenderTarget();
    if (rt) {
        rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.40f), &m_pBrushGrip);
    }

    // 1-second update timer for clock & widgets
    SetTimer(m_hwnd, TIMER_UPDATE_ID, 1000, nullptr);

    return true;
}

void WidgetHostWindow::Show(bool show) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, show ? SW_SHOWNOACTIVATE : SW_HIDE);
        SetWindowPos(m_hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        UpdateWindow(m_hwnd);
    }
}

void WidgetHostWindow::Suspend() {
    if (!m_hwnd) return;
    ShowWindow(m_hwnd, SW_HIDE);
    KillTimer(m_hwnd, TIMER_UPDATE_ID);
}

void WidgetHostWindow::Resume() {
    if (!m_hwnd) return;
    SetTimer(m_hwnd, TIMER_UPDATE_ID, 1000, nullptr);
    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    SetWindowPos(m_hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    Render();
}

void WidgetHostWindow::AddWidget(std::unique_ptr<IWidget> widget) {
    if (widget) {
        widget->OnInit(m_d2d.GetRenderTarget(), m_d2d.GetDWriteFactory());
        m_widgets.push_back(std::move(widget));
    }
}

IWidget* WidgetHostWindow::FindWidget(const std::wstring& id) {
    for (auto& w : m_widgets) {
        if (w->GetId() == id) return w.get();
    }
    return nullptr;
}

void WidgetHostWindow::ApplySettings(bool clockEnabled, bool is24Hour, bool showSeconds, bool showDate,
                                    bool diskEnabled, float diskInterval,
                                    bool sysMonEnabled, bool calendarEnabled, bool memoEnabled,
                                    bool lockPosition) {
    m_lockPosition = lockPosition;

    for (auto& w : m_widgets) {
        std::wstring id = w->GetId();
        if (id == L"widget_clock") {
            w->SetEnabled(clockEnabled);
            if (auto clock = dynamic_cast<ClockWidget*>(w.get())) {
                clock->Set24Hour(is24Hour);
                clock->SetShowSeconds(showSeconds);
                clock->SetShowDate(showDate);
                clock->OnUpdate(1.0f);
            }
        } else if (id == L"widget_disk") {
            w->SetEnabled(diskEnabled);
            if (auto disk = dynamic_cast<DiskWidget*>(w.get())) {
                disk->SetRefreshInterval(diskInterval);
            }
        } else if (id == L"widget_sysmon") {
            w->SetEnabled(sysMonEnabled);
        } else if (id == L"widget_calendar") {
            w->SetEnabled(calendarEnabled);
        } else if (id == L"widget_memo") {
            w->SetEnabled(memoEnabled);
        }
    }

    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void WidgetHostWindow::ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, D2D1_COLOR_F accent) {
    for (auto& w : m_widgets) {
        w->ApplyTheme(bg, border, accent);
    }
    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void WidgetHostWindow::OnTimer() {
    for (auto& w : m_widgets) {
        if (w->IsEnabled()) {
            w->OnUpdate(1.0f);
        }
    }
    Render();
}

void WidgetHostWindow::Render() {
    auto rt = m_d2d.GetRenderTarget();
    if (!rt) return;

    m_d2d.BeginDraw();
    rt->Clear(D2D1::ColorF(0, 0, 0, 0.0f));

    for (const auto& w : m_widgets) {
        if (!w->IsEnabled()) continue;

        w->OnRender(rt);

        // If widgets are not locked, draw subtle diagonal resize grip dots at the bottom-right corner
        if (!m_lockPosition && m_pBrushGrip) {
            D2D1_RECT_F b = w->GetBounds();
            float rx = b.right - 8.0f;
            float ry = b.bottom - 8.0f;

            // 3 subtle grip dots
            rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(rx, ry), 1.5f, 1.5f), m_pBrushGrip);
            rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(rx - 5.0f, ry), 1.5f, 1.5f), m_pBrushGrip);
            rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(rx, ry - 5.0f), 1.5f, 1.5f), m_pBrushGrip);
        }
    }

    m_d2d.EndDraw();
}

LRESULT CALLBACK WidgetHostWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WidgetHostWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<WidgetHostWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<WidgetHostWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT WidgetHostWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_MOUSEACTIVATE:
        // Do not activate the widget host window when clicked
        return MA_NOACTIVATE;

    case WM_WINDOWPOSCHANGING: {
        auto* pwp = reinterpret_cast<WINDOWPOS*>(lParam);
        if (pwp && !(pwp->flags & SWP_NOZORDER)) {
            // Keep widgets strictly at desktop background layer
            pwp->hwndInsertAfter = HWND_BOTTOM;
        }
        return 0;
    }

    case WM_ACTIVATE: {
        if (LOWORD(wParam) != WA_INACTIVE) {
            SetWindowPos(m_hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        return 0;
    }

    case WM_CAPTURECHANGED: {
        m_dragMode = DragMode::None;
        m_activeWidget = nullptr;
        return 0;
    }

    case WM_TIMER:
        if (wParam == TIMER_UPDATE_ID) {
            OnTimer();
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);
        Render();
        EndPaint(m_hwnd, &ps);
        return 0;
    }

    case WM_NCHITTEST: {
        short screenX = static_cast<short>(LOWORD(lParam));
        short screenY = static_cast<short>(HIWORD(lParam));
        POINT pt = { screenX, screenY };
        ScreenToClient(m_hwnd, &pt);

        for (const auto& w : m_widgets) {
            if (!w->IsEnabled()) continue;
            D2D1_RECT_F b = w->GetBounds();
            if (pt.x >= b.left && pt.x <= b.right && pt.y >= b.top && pt.y <= b.bottom) {
                return HTCLIENT;
            }
        }
        // Transparent outside all widgets: clicks pass straight through to desktop icons/wallpaper!
        return HTTRANSPARENT;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(m_hwnd, &pt);

            if (m_dragMode == DragMode::Resize) {
                SetCursor(LoadCursor(nullptr, IDC_SIZENWSE));
                return TRUE;
            } else if (m_dragMode == DragMode::Move) {
                SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
                return TRUE;
            }

            for (const auto& w : m_widgets) {
                if (!w->IsEnabled()) continue;
                D2D1_RECT_F b = w->GetBounds();
                if (pt.x >= b.left && pt.x <= b.right && pt.y >= b.top && pt.y <= b.bottom) {
                    bool inGrip = (pt.x >= b.right - RESIZE_GRIP_SIZE && pt.y >= b.bottom - RESIZE_GRIP_SIZE);
                    if (inGrip && !m_lockPosition) {
                        SetCursor(LoadCursor(nullptr, IDC_SIZENWSE));
                        return TRUE;
                    } else if (!m_lockPosition) {
                        SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
                        return TRUE;
                    } else {
                        SetCursor(LoadCursor(nullptr, IDC_ARROW));
                        return TRUE;
                    }
                }
            }
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        POINT pt = { static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
        // Check topmost matching widget
        for (auto it = m_widgets.rbegin(); it != m_widgets.rend(); ++it) {
            auto& w = *it;
            if (!w->IsEnabled()) continue;
            D2D1_RECT_F b = w->GetBounds();
            if (pt.x >= b.left && pt.x <= b.right && pt.y >= b.top && pt.y <= b.bottom) {
                m_activeWidget = w.get();
                m_dragStartMouse = pt;
                m_dragStartWidgetX = b.left;
                m_dragStartWidgetY = b.top;
                m_dragStartWidgetW = b.right - b.left;
                m_dragStartWidgetH = b.bottom - b.top;

                bool inGrip = (pt.x >= b.right - RESIZE_GRIP_SIZE && pt.y >= b.bottom - RESIZE_GRIP_SIZE);
                if (inGrip && !m_lockPosition) {
                    m_dragMode = DragMode::Resize;
                    SetCapture(m_hwnd);
                } else if (!m_lockPosition) {
                    m_dragMode = DragMode::Move;
                    SetCapture(m_hwnd);
                } else {
                    m_dragMode = DragMode::None;
                }
                return 0;
            }
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        POINT pt = { static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam)) };
        if (m_dragMode == DragMode::Move && m_activeWidget) {
            float dx = static_cast<float>(pt.x - m_dragStartMouse.x);
            float dy = static_cast<float>(pt.y - m_dragStartMouse.y);
            float newX = m_dragStartWidgetX + dx;
            float newY = m_dragStartWidgetY + dy;
            D2D1_RECT_F b = m_activeWidget->GetBounds();
            float w = b.right - b.left;
            float h = b.bottom - b.top;

            ApplyMagneticSnapping(m_activeWidget, newX, newY, w, h);

            m_activeWidget->SetPosition(newX, newY);
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return 0;
        } else if (m_dragMode == DragMode::Resize && m_activeWidget) {
            float dx = static_cast<float>(pt.x - m_dragStartMouse.x);
            float dy = static_cast<float>(pt.y - m_dragStartMouse.y);
            float newW = (std::max)(m_activeWidget->GetMinSize().width, m_dragStartWidgetW + dx);
            float newH = (std::max)(m_activeWidget->GetMinSize().height, m_dragStartWidgetH + dy);
            m_activeWidget->SetSize(newW, newH);
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    }

    case WM_LBUTTONUP: {
        if (GetCapture() == m_hwnd) {
            ReleaseCapture();
        }
        if (m_dragMode != DragMode::None) {
            if (m_onBoundsChanged && m_activeWidget) {
                D2D1_RECT_F b = m_activeWidget->GetBounds();
                m_onBoundsChanged(m_activeWidget->GetId(), b.left, b.top, b.right - b.left, b.bottom - b.top);
            }
            m_dragMode = DragMode::None;
            m_activeWidget = nullptr;
        }
        SetWindowPos(m_hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcW(m_hwnd, msg, wParam, lParam);
}

void WidgetHostWindow::ApplyMagneticSnapping(IWidget* activeWidget, float& x, float& y, float w, float h) {
    if (!activeWidget) return;

    constexpr float SNAP_GAP = 10.0f;
    constexpr float SNAP_DIST = 14.0f;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    if (screenWidth <= 0) screenWidth = 1920;

    float bestDiffX = SNAP_DIST + 1.0f;
    float snapX = x;
    bool snappedX = false;

    float bestDiffY = SNAP_DIST + 1.0f;
    float snapY = y;
    bool snappedY = false;

    // 1. Screen edge snapping (top 25px & right 25px margins)
    float screenRightMargin = static_cast<float>(screenWidth) - w - 25.0f;
    if (std::abs(x - screenRightMargin) <= SNAP_DIST) {
        bestDiffX = std::abs(x - screenRightMargin);
        snapX = screenRightMargin;
        snappedX = true;
    }
    if (std::abs(y - 25.0f) <= SNAP_DIST) {
        bestDiffY = std::abs(y - 25.0f);
        snapY = 25.0f;
        snappedY = true;
    }

    // 2. Snap to other enabled widgets
    for (const auto& other : m_widgets) {
        if (!other || other.get() == activeWidget || !other->IsEnabled()) continue;

        D2D1_RECT_F ob = other->GetBounds();

        // Horizontal proximity
        bool xNear = (x + w >= ob.left - 40.0f && x <= ob.right + 40.0f);
        // Vertical proximity
        bool yNear = (y + h >= ob.top - 40.0f && y <= ob.bottom + 40.0f);

        // --- Y-axis Snapping ---
        if (xNear) {
            // A below B with 10px gap
            float targetYBelow = ob.bottom + SNAP_GAP;
            float diffBelow = std::abs(y - targetYBelow);
            if (diffBelow <= SNAP_DIST && diffBelow < bestDiffY) {
                bestDiffY = diffBelow;
                snapY = targetYBelow;
                snappedY = true;
            }

            // A above B with 10px gap
            float targetYAbove = ob.top - h - SNAP_GAP;
            float diffAbove = std::abs(y - targetYAbove);
            if (diffAbove <= SNAP_DIST && diffAbove < bestDiffY) {
                bestDiffY = diffAbove;
                snapY = targetYAbove;
                snappedY = true;
            }

            // Top-align with B
            float diffTop = std::abs(y - ob.top);
            if (diffTop <= SNAP_DIST && diffTop < bestDiffY) {
                bestDiffY = diffTop;
                snapY = ob.top;
                snappedY = true;
            }

            // Bottom-align with B
            float diffBottom = std::abs((y + h) - ob.bottom);
            if (diffBottom <= SNAP_DIST && diffBottom < bestDiffY) {
                bestDiffY = diffBottom;
                snapY = ob.bottom - h;
                snappedY = true;
            }
        }

        // --- X-axis Snapping ---
        if (yNear) {
            // A to the right of B with 10px gap
            float targetXRight = ob.right + SNAP_GAP;
            float diffRight = std::abs(x - targetXRight);
            if (diffRight <= SNAP_DIST && diffRight < bestDiffX) {
                bestDiffX = diffRight;
                snapX = targetXRight;
                snappedX = true;
            }

            // A to the left of B with 10px gap
            float targetXLeft = ob.left - w - SNAP_GAP;
            float diffLeft = std::abs(x - targetXLeft);
            if (diffLeft <= SNAP_DIST && diffLeft < bestDiffX) {
                bestDiffX = diffLeft;
                snapX = targetXLeft;
                snappedX = true;
            }

            // Left-align with B
            float diffLeftAlign = std::abs(x - ob.left);
            if (diffLeftAlign <= SNAP_DIST && diffLeftAlign < bestDiffX) {
                bestDiffX = diffLeftAlign;
                snapX = ob.left;
                snappedX = true;
            }

            // Right-align with B
            float diffRightAlign = std::abs((x + w) - ob.right);
            if (diffRightAlign <= SNAP_DIST && diffRightAlign < bestDiffX) {
                bestDiffX = diffRightAlign;
                snapX = ob.right - w;
                snappedX = true;
            }
        }
    }

    if (snappedX) x = snapX;
    if (snappedY) y = snapY;
}

} // namespace Widgets
