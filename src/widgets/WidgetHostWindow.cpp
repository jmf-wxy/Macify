#include "WidgetHostWindow.h"
#include "ClockWidget.h"
#include "DiskWidget.h"
#include "SysMonitorWidget.h"
#include "CalendarWidget.h"
#include "MemoWidget.h"
#include "../core/Backdrop.h"
#include <algorithm>

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
        WS_EX_TOOLWINDOW,
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
        if (m_dragMode != DragMode::None) {
            return HTCLIENT;
        }

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
            m_activeWidget->SetPosition(m_dragStartWidgetX + dx, m_dragStartWidgetY + dy);
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
        if (m_dragMode != DragMode::None) {
            ReleaseCapture();
            if (m_onBoundsChanged && m_activeWidget) {
                D2D1_RECT_F b = m_activeWidget->GetBounds();
                m_onBoundsChanged(m_activeWidget->GetId(), b.left, b.top, b.right - b.left, b.bottom - b.top);
            }
            m_dragMode = DragMode::None;
            m_activeWidget = nullptr;
            return 0;
        }
        break;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcW(m_hwnd, msg, wParam, lParam);
}

} // namespace Widgets
