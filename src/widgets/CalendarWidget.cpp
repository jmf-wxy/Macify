#include "CalendarWidget.h"
#include <cwchar>

namespace Widgets {

CalendarWidget::CalendarWidget(float x, float y, float width, float height)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_baseWidth(220.0f)
    , m_baseHeight(165.0f)
    , m_minWidth(150.0f)
    , m_minHeight(115.0f)
    , m_enabled(true)
    , m_timerAccumulator(60.0f)
    , m_year(2026)
    , m_month(11)
    , m_today(24)
    , m_firstDayWeekday(0)
    , m_daysInMonth(30)
    , m_pBrushBg(nullptr)
    , m_pBrushBorder(nullptr)
    , m_pBrushText(nullptr)
    , m_pBrushSubText(nullptr)
    , m_pBrushAccent(nullptr)
    , m_pBrushTodayText(nullptr)
    , m_pHeaderFormat(nullptr)
    , m_pWeekdayFormat(nullptr)
    , m_pDayFormat(nullptr) {
    if (m_width <= 0) m_width = m_baseWidth;
    if (m_height <= 0) m_height = m_baseHeight;
}

CalendarWidget::~CalendarWidget() {
    if (m_pDayFormat) m_pDayFormat->Release();
    if (m_pWeekdayFormat) m_pWeekdayFormat->Release();
    if (m_pHeaderFormat) m_pHeaderFormat->Release();
    if (m_pBrushTodayText) m_pBrushTodayText->Release();
    if (m_pBrushAccent) m_pBrushAccent->Release();
    if (m_pBrushSubText) m_pBrushSubText->Release();
    if (m_pBrushText) m_pBrushText->Release();
    if (m_pBrushBorder) m_pBrushBorder->Release();
    if (m_pBrushBg) m_pBrushBg->Release();
}

void CalendarWidget::ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, D2D1_COLOR_F accent) {
    if (m_pBrushBg) m_pBrushBg->SetColor(bg);
    if (m_pBrushBorder) m_pBrushBorder->SetColor(border);
    if (m_pBrushAccent) m_pBrushAccent->SetColor(accent);
}

void CalendarWidget::OnInit(ID2D1RenderTarget* pRT, IDWriteFactory* pDWrite) {
    if (pRT) {
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.08f, 0.09f, 0.12f, 0.72f), &m_pBrushBg);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.18f), &m_pBrushBorder);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.95f, 0.98f, 0.95f), &m_pBrushText);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.6f, 0.63f, 0.7f, 0.75f), &m_pBrushSubText);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.00f, 0.50f, 1.0f, 0.95f), &m_pBrushAccent);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), &m_pBrushTodayText);
    }

    if (pDWrite) {
        pDWrite->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            13.0f,
            L"zh-CN",
            &m_pHeaderFormat
        );
        if (m_pHeaderFormat) {
            m_pHeaderFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_pHeaderFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        pDWrite->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            9.5f,
            L"zh-CN",
            &m_pWeekdayFormat
        );
        if (m_pWeekdayFormat) {
            m_pWeekdayFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_pWeekdayFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        pDWrite->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            10.0f,
            L"zh-CN",
            &m_pDayFormat
        );
        if (m_pDayFormat) {
            m_pDayFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            m_pDayFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    UpdateCalendarInfo();
}

void CalendarWidget::UpdateCalendarInfo() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    m_year = st.wYear;
    m_month = st.wMonth;
    m_today = st.wDay;

    wchar_t hBuf[32];
    swprintf_s(hBuf, L"%d年 %d月", m_year, m_month);
    m_headerTitle = hBuf;

    int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    bool isLeap = (m_year % 4 == 0 && m_year % 100 != 0) || (m_year % 400 == 0);
    if (m_month == 2 && isLeap) {
        m_daysInMonth = 29;
    } else {
        m_daysInMonth = days[m_month - 1];
    }

    SYSTEMTIME firstDay = st;
    firstDay.wDay = 1;
    firstDay.wHour = 12;
    firstDay.wMinute = 0;
    firstDay.wSecond = 0;
    firstDay.wMilliseconds = 0;
    FILETIME ft;
    SystemTimeToFileTime(&firstDay, &ft);
    FileTimeToSystemTime(&ft, &firstDay);
    m_firstDayWeekday = firstDay.wDayOfWeek;
}

void CalendarWidget::OnUpdate(float deltaTime) {
    m_timerAccumulator += deltaTime;
    // Check local time update every 60 seconds
    if (m_timerAccumulator >= 60.0f) {
        m_timerAccumulator = 0.0f;
        UpdateCalendarInfo();
    }
}

void CalendarWidget::OnRender(ID2D1RenderTarget* pRT) {
    if (!m_enabled || !pRT) return;

    D2D1_MATRIX_3X2_F oldTrans;
    pRT->GetTransform(&oldTrans);

    float scaleX = m_width / m_baseWidth;
    float scaleY = m_height / m_baseHeight;
    D2D1_MATRIX_3X2_F scaleTrans = D2D1::Matrix3x2F::Scale(scaleX, scaleY, D2D1::Point2F(m_x, m_y));
    pRT->SetTransform(scaleTrans * oldTrans);

    // 1. Card backdrop
    D2D1_ROUNDED_RECT cardRect = D2D1::RoundedRect(
        D2D1::RectF(m_x, m_y, m_x + m_baseWidth, m_y + m_baseHeight),
        16.0f,
        16.0f
    );
    if (m_pBrushBg) pRT->FillRoundedRectangle(cardRect, m_pBrushBg);
    if (m_pBrushBorder) pRT->DrawRoundedRectangle(cardRect, m_pBrushBorder, 1.2f);

    // 2. Month & Year Title
    if (m_pHeaderFormat && m_pBrushText) {
        D2D1_RECT_F titleRect = D2D1::RectF(m_x, m_y + 10.0f, m_x + m_baseWidth, m_y + 28.0f);
        pRT->DrawTextW(
            m_headerTitle.c_str(),
            static_cast<UINT32>(m_headerTitle.length()),
            m_pHeaderFormat,
            titleRect,
            m_pBrushText
        );
    }

    // 3. Weekday Headers
    const wchar_t* weekdays[] = { L"日", L"一", L"二", L"三", L"四", L"五", L"六" };
    float colW = (m_baseWidth - 20.0f) / 7.0f;
    float startX = m_x + 10.0f;
    float headerY = m_y + 32.0f;

    if (m_pWeekdayFormat && m_pBrushSubText) {
        for (int i = 0; i < 7; ++i) {
            D2D1_RECT_F colRect = D2D1::RectF(startX + i * colW, headerY, startX + (i + 1) * colW, headerY + 16.0f);
            pRT->DrawTextW(
                weekdays[i],
                1,
                m_pWeekdayFormat,
                colRect,
                m_pBrushSubText
            );
        }
    }

    // 4. Days Grid
    float gridTop = headerY + 18.0f;
    float rowH = 17.0f;
    int curCol = m_firstDayWeekday;
    int curRow = 0;

    for (int day = 1; day <= m_daysInMonth; ++day) {
        float cellX = startX + curCol * colW;
        float cellY = gridTop + curRow * rowH;
        D2D1_RECT_F cellRect = D2D1::RectF(cellX, cellY, cellX + colW, cellY + rowH);

        bool isToday = (day == m_today);
        if (isToday && m_pBrushAccent) {
            // Draw highlight capsule for Today
            float centerX = cellX + colW * 0.5f;
            float centerY = cellY + rowH * 0.5f;
            D2D1_ROUNDED_RECT todayBg = D2D1::RoundedRect(
                D2D1::RectF(centerX - 8.5f, centerY - 8.0f, centerX + 8.5f, centerY + 8.0f),
                4.0f, 4.0f
            );
            pRT->FillRoundedRectangle(todayBg, m_pBrushAccent);
        }

        wchar_t dayBuf[8];
        swprintf_s(dayBuf, L"%d", day);
        if (m_pDayFormat) {
            ID2D1SolidColorBrush* textBrush = isToday ? m_pBrushTodayText : m_pBrushText;
            if (textBrush) {
                pRT->DrawTextW(
                    dayBuf,
                    static_cast<UINT32>(wcslen(dayBuf)),
                    m_pDayFormat,
                    cellRect,
                    textBrush
                );
            }
        }

        curCol++;
        if (curCol >= 7) {
            curCol = 0;
            curRow++;
        }
    }

    pRT->SetTransform(oldTrans);
}

void CalendarWidget::OnMouseEvent(const MouseEvent& evt) {
    (void)evt;
}

} // namespace Widgets
