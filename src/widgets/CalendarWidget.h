#pragma once
#include "IWidget.h"
#include <dwrite.h>

namespace Widgets {

class CalendarWidget : public IWidget {
public:
    CalendarWidget(float x = 40.0f, float y = 400.0f, float width = 220.0f, float height = 165.0f);
    ~CalendarWidget() override;

    std::wstring GetId() const override { return L"widget_calendar"; }
    std::wstring GetName() const override { return L"桌面日历小组件"; }

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
    void ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, D2D1_COLOR_F accent) override;

private:
    void UpdateCalendarInfo();

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
    float m_timerAccumulator;

    int m_year;
    int m_month;
    int m_today;
    int m_firstDayWeekday; // 0 = Sunday
    int m_daysInMonth;

    std::wstring m_headerTitle;

    ID2D1SolidColorBrush* m_pBrushBg;
    ID2D1SolidColorBrush* m_pBrushBorder;
    ID2D1SolidColorBrush* m_pBrushText;
    ID2D1SolidColorBrush* m_pBrushSubText;
    ID2D1SolidColorBrush* m_pBrushAccent;
    ID2D1SolidColorBrush* m_pBrushTodayText;

    IDWriteTextFormat* m_pHeaderFormat;
    IDWriteTextFormat* m_pWeekdayFormat;
    IDWriteTextFormat* m_pDayFormat;
};

} // namespace Widgets
