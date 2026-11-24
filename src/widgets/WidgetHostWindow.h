#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include "../core/D2DContext.h"
#include "IWidget.h"

namespace Widgets {

class WidgetHostWindow {
public:
    WidgetHostWindow();
    ~WidgetHostWindow();

    bool Create();
    void Show(bool show = true);
    void Suspend();
    void Resume();
    HWND GetHwnd() const { return m_hwnd; }

    void AddWidget(std::unique_ptr<IWidget> widget);
    void ApplySettings(bool clockEnabled, bool is24Hour, bool showSeconds, bool showDate, bool diskEnabled, float diskInterval);
    void ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, D2D1_COLOR_F accent);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void Render();
    void OnTimer();

private:
    HWND m_hwnd;
    Core::D2DContext m_d2d;
    std::vector<std::unique_ptr<IWidget>> m_widgets;
};

} // namespace Widgets
