#pragma once
#include "IWidget.h"
#include <dwrite.h>

namespace Widgets {

class ClockWidget : public IWidget {
public:
    ClockWidget(float x = 60.0f, float y = 60.0f);
    ~ClockWidget() override;

    std::wstring GetId() const override { return L"widget_clock"; }
    std::wstring GetName() const override { return L"时钟小组件"; }

    void OnInit(ID2D1RenderTarget* pRT, IDWriteFactory* pDWrite) override;
    void OnUpdate(float deltaTime) override;
    void OnRender(ID2D1RenderTarget* pRT) override;
    void OnMouseEvent(const MouseEvent& evt) override;

    D2D1_RECT_F GetBounds() const override;
    void SetPosition(float x, float y) override;
    bool IsEnabled() const override { return m_enabled; }
    void SetEnabled(bool enable) override { m_enabled = enable; }
    void Set24Hour(bool is24) { m_is24Hour = is24; }
    void SetShowSeconds(bool show) { m_showSeconds = show; }
    void SetShowDate(bool show) { m_showDate = show; }
    void ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border);

private:
    float m_x;
    float m_y;
    float m_width;
    float m_height;
    bool m_enabled;
    bool m_is24Hour;
    bool m_showSeconds;
    bool m_showDate;

    std::wstring m_timeString;
    std::wstring m_dateString;
    float m_timerAccumulator;

    ID2D1SolidColorBrush* m_pBrushBg;
    ID2D1SolidColorBrush* m_pBrushBorder;
    ID2D1SolidColorBrush* m_pBrushTimeText;
    ID2D1SolidColorBrush* m_pBrushDateText;

    IDWriteTextFormat* m_pTimeFormat;
    IDWriteTextFormat* m_pDateFormat;
};

} // namespace Widgets
