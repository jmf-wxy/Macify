#include "WidgetHostWindow.h"
#include "ClockWidget.h"
#include "DiskWidget.h"
#include "../core/Backdrop.h"

namespace Widgets {

constexpr UINT_PTR TIMER_UPDATE_ID = 2001;

WidgetHostWindow::WidgetHostWindow()
    : m_hwnd(nullptr) {
}

WidgetHostWindow::~WidgetHostWindow() {
    if (m_hwnd) {
        KillTimer(m_hwnd, TIMER_UPDATE_ID);
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

    // Place on desktop top-right or top-left area
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int winW = 240;
    int winH = 460;
    int winX = screenWidth - winW - 30; // top right
    int winY = 40;

    m_hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        CLASS_NAME,
        L"DesktopWidgets",
        WS_POPUP,
        winX,
        winY,
        winW,
        winH,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );

    if (!m_hwnd) return false;

    // Remove window border and enable full transparency
    Core::RemoveWindowBorder(m_hwnd);

    // Send to bottom (desktop level, below normal windows)
    SetWindowPos(m_hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // Initialize Direct2D
    if (!m_d2d.Initialize(m_hwnd)) return false;

    // 1-second update timer for clock & widgets
    SetTimer(m_hwnd, TIMER_UPDATE_ID, 1000, nullptr);

    return true;
}

void WidgetHostWindow::Show(bool show) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, show ? SW_SHOWNOACTIVATE : SW_HIDE);
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

void WidgetHostWindow::ApplySettings(bool clockEnabled, bool is24Hour, bool showSeconds, bool showDate, bool diskEnabled, float diskInterval) {
    for (auto& w : m_widgets) {
        if (w->GetId() == L"widget_clock") {
            w->SetEnabled(clockEnabled);
            auto clock = dynamic_cast<ClockWidget*>(w.get());
            if (clock) {
                clock->Set24Hour(is24Hour);
                clock->SetShowSeconds(showSeconds);
                clock->SetShowDate(showDate);
                clock->OnUpdate(1.0f);
            }
        } else if (w->GetId() == L"widget_disk") {
            w->SetEnabled(diskEnabled);
            auto disk = dynamic_cast<DiskWidget*>(w.get());
            if (disk) {
                disk->SetRefreshInterval(diskInterval);
            }
        }
    }
    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void WidgetHostWindow::ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, D2D1_COLOR_F accent) {
    for (auto& w : m_widgets) {
        if (auto clock = dynamic_cast<ClockWidget*>(w.get())) {
            clock->ApplyTheme(bg, border);
        } else if (auto disk = dynamic_cast<DiskWidget*>(w.get())) {
            disk->ApplyTheme(bg, border, accent);
        }
    }
    if (m_hwnd) {
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void WidgetHostWindow::OnTimer() {
    for (auto& w : m_widgets) {
        w->OnUpdate(1.0f);
    }
    Render();
}

void WidgetHostWindow::Render() {
    auto rt = m_d2d.GetRenderTarget();
    if (!rt) return;

    m_d2d.BeginDraw();
    rt->Clear(D2D1::ColorF(0, 0, 0, 0.0f));

    for (const auto& w : m_widgets) {
        w->OnRender(rt);
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
        short screenX = static_cast<short>(LOWORD(lParam));
        short screenY = static_cast<short>(HIWORD(lParam));
        POINT pt = { screenX, screenY };
        ScreenToClient(m_hwnd, &pt);
        for (const auto& w : m_widgets) {
            D2D1_RECT_F b = w->GetBounds();
            if (pt.x >= b.left && pt.x <= b.right && pt.y >= b.top && pt.y <= b.bottom) {
                return HTCLIENT;
            }
        }
        return HTTRANSPARENT;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcW(m_hwnd, msg, wParam, lParam);
}

} // namespace Widgets
