#pragma once
#include "IWidget.h"
#include <dwrite.h>

namespace Widgets {

class ClockWidget : public IWidget {
public:
    ClockWidget(float x = 40.0f, float y = 40.0f, float width = 220.0f, float height = 90.0f);
    ~ClockWidget() override;

    std::wstring GetId() const override { return L"widget_clock"; }
    std::wstring GetName() const override { return L"数字时钟挂件"; }

    void OnInit(ID2D1RenderTarget* pRT, IDWriteFactory* pDWrite) override;
    void OnUpdate(float deltaTime) override;
    void OnRender(ID2D1RenderTarget* pRT) override;
    void OnMouseEvent(const MouseEvent& evt) override;

    D2D1_RECT_F GetBounds() const override { return D2D1::RectF(m_x, m_y, m_x + m_width, m_y + m_height); }
    void SetPosition(float x, float y) override { m_x = x; m_y = y; }
    void SetSize(float width, float height) override { m_width = width; m_height = height; }
    D2D1_SIZE_F GetSize() const override { return D2D1::SizeF(m_width, m_height); }
    D2D1_SIZE_F GetBaseSize() const override { return D2D1::SizeF(m_baseWidth, m_baseHeight); }
    D2D1_SIZE_F GetMinSize() const override { return D2D1::SizeF(m_minWidth, m_minHeight); }

    bool IsEnabled() const override { return m_enabled; }
    void SetEnabled(bool enable) override { m_enabled = enable; }
    void Set24Hour(bool is24) { m_is24Hour = is24; }
    void SetShowSeconds(bool show) { m_showSeconds = show; }
    void SetShowDate(bool show) { m_showDate = show; }
    void ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, D2D1_COLOR_F accent) override;

private:
    float m_x;
    float m_y;
    float m_width;
    float m_height;
    float m_baseWidth;
    float m_baseHeight;
    float m_minWidth;
    float m_minHeight;

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
