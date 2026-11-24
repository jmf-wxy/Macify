#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <functional>
#include "../core/D2DContext.h"
#include "IWidget.h"

namespace Widgets {

enum class DragMode { None, Move, Resize };

class WidgetHostWindow {
public:
    using OnBoundsChangedCallback = std::function<void(const std::wstring& id, float x, float y, float w, float h)>;

    WidgetHostWindow();
    ~WidgetHostWindow();

    bool Create();
    void Show(bool show = true);
    void Suspend();
    void Resume();
    HWND GetHwnd() const { return m_hwnd; }

    void AddWidget(std::unique_ptr<IWidget> widget);
    IWidget* FindWidget(const std::wstring& id);

    void ApplySettings(bool clockEnabled, bool is24Hour, bool showSeconds, bool showDate,
                       bool diskEnabled, float diskInterval,
                       bool sysMonEnabled, bool calendarEnabled, bool memoEnabled,
                       bool lockPosition);
    void ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, D2D1_COLOR_F accent);
    void SetOnBoundsChanged(OnBoundsChangedCallback cb) { m_onBoundsChanged = cb; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void Render();
    void OnTimer();
    void ApplyMagneticSnapping(IWidget* activeWidget, float& x, float& y, float w, float h);

private:
    HWND m_hwnd;
    Core::D2DContext m_d2d;
    std::vector<std::unique_ptr<IWidget>> m_widgets;

    bool m_lockPosition;
    DragMode m_dragMode;
    IWidget* m_activeWidget;
    POINT m_dragStartMouse;
    float m_dragStartWidgetX;
    float m_dragStartWidgetY;
    float m_dragStartWidgetW;
    float m_dragStartWidgetH;

    ID2D1SolidColorBrush* m_pBrushGrip;
    OnBoundsChangedCallback m_onBoundsChanged;
};

} // namespace Widgets
