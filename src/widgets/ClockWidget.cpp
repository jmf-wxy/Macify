#include "ClockWidget.h"
#include <cwchar>
#include <ctime>

namespace Widgets {

ClockWidget::ClockWidget(float x, float y)
    : m_x(x)
    , m_y(y)
    , m_width(220.0f)
    , m_height(90.0f)
    , m_enabled(true)
    , m_is24Hour(true)
    , m_showSeconds(true)
    , m_showDate(true)
    , m_timerAccumulator(1.0f)
    , m_pBrushBg(nullptr)
    , m_pBrushBorder(nullptr)
    , m_pBrushTimeText(nullptr)
    , m_pBrushDateText(nullptr)
    , m_pTimeFormat(nullptr)
    , m_pDateFormat(nullptr) {
}

ClockWidget::~ClockWidget() {
    if (m_pDateFormat) m_pDateFormat->Release();
    if (m_pTimeFormat) m_pTimeFormat->Release();
    if (m_pBrushDateText) m_pBrushDateText->Release();
    if (m_pBrushTimeText) m_pBrushTimeText->Release();
    if (m_pBrushBorder) m_pBrushBorder->Release();
    if (m_pBrushBg) m_pBrushBg->Release();
}

void ClockWidget::ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border) {
    if (m_pBrushBg) m_pBrushBg->SetColor(bg);
    if (m_pBrushBorder) m_pBrushBorder->SetColor(border);
}

void ClockWidget::OnInit(ID2D1RenderTarget* pRT, IDWriteFactory* pDWrite) {
    if (pRT) {
        // Frosted glass card backdrop: semi-transparent with soft white border
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.08f, 0.09f, 0.12f, 0.72f), &m_pBrushBg);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.18f), &m_pBrushBorder);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.98f, 0.98f, 1.0f, 0.95f), &m_pBrushTimeText);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.75f, 0.78f, 0.85f, 0.85f), &m_pBrushDateText);
    }

    if (pDWrite) {
        // Large digital clock font
        pDWrite->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            36.0f,
            L"zh-CN",
            &m_pTimeFormat
        );
        if (m_pTimeFormat) {
            m_pTimeFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_pTimeFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        // Subtitle date font
        pDWrite->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            13.0f,
            L"zh-CN",
            &m_pDateFormat
        );
        if (m_pDateFormat) {
            m_pDateFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_pDateFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    OnUpdate(1.0f);
}

void ClockWidget::OnUpdate(float deltaTime) {
    m_timerAccumulator += deltaTime;
    if (m_timerAccumulator >= 1.0f) {
        m_timerAccumulator = 0.0f;

        SYSTEMTIME st;
        GetLocalTime(&st);

        wchar_t timeBuf[32];
        int hour = st.wHour;
        if (!m_is24Hour) {
            hour = hour % 12;
            if (hour == 0) hour = 12;
        }

        if (m_showSeconds) {
            swprintf_s(timeBuf, L"%02d:%02d:%02d", hour, st.wMinute, st.wSecond);
        } else {
            swprintf_s(timeBuf, L"%02d:%02d", hour, st.wMinute);
        }
        m_timeString = timeBuf;

        if (m_showDate) {
            const wchar_t* weekDays[] = { L"周日", L"周一", L"周二", L"周三", L"周四", L"周五", L"周六" };
            wchar_t dateBuf[64];
            swprintf_s(dateBuf, L"%d月%d日 %s", st.wMonth, st.wDay, weekDays[st.wDayOfWeek % 7]);
            m_dateString = dateBuf;
        } else {
            m_dateString = L"";
        }
    }
}

void ClockWidget::OnRender(ID2D1RenderTarget* pRT) {
    if (!m_enabled || !pRT) return;

    // 1. Draw rounded card
    D2D1_ROUNDED_RECT cardRect = D2D1::RoundedRect(
        D2D1::RectF(m_x, m_y, m_x + m_width, m_y + m_height),
        16.0f,
        16.0f
    );
    pRT->FillRoundedRectangle(cardRect, m_pBrushBg);
    pRT->DrawRoundedRectangle(cardRect, m_pBrushBorder, 1.2f);

    // 2. Draw Time
    if (m_pTimeFormat && m_pBrushTimeText) {
        D2D1_RECT_F timeRect = D2D1::RectF(m_x, m_y + 8.0f, m_x + m_width, m_y + 54.0f);
        pRT->DrawTextW(
            m_timeString.c_str(),
            static_cast<UINT32>(m_timeString.length()),
            m_pTimeFormat,
            timeRect,
            m_pBrushTimeText
        );
    }

    // 3. Draw Date
    if (m_pDateFormat && m_pBrushDateText) {
        D2D1_RECT_F dateRect = D2D1::RectF(m_x, m_y + 54.0f, m_x + m_width, m_y + 82.0f);
        pRT->DrawTextW(
            m_dateString.c_str(),
            static_cast<UINT32>(m_dateString.length()),
            m_pDateFormat,
            dateRect,
            m_pBrushDateText
        );
    }
}

void ClockWidget::OnMouseEvent(const MouseEvent& evt) {
    // Interactive drag or hover can be handled here
}

D2D1_RECT_F ClockWidget::GetBounds() const {
    return D2D1::RectF(m_x, m_y, m_x + m_width, m_y + m_height);
}

void ClockWidget::SetPosition(float x, float y) {
    m_x = x;
    m_y = y;
}

} // namespace Widgets
