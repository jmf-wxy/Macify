#include "MemoWidget.h"
#include <cwchar>

namespace Widgets {

MemoWidget::MemoWidget(float x, float y, float width, float height)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_baseWidth(220.0f)
    , m_baseHeight(120.0f)
    , m_minWidth(140.0f)
    , m_minHeight(85.0f)
    , m_enabled(true)
    , m_pBrushBg(nullptr)
    , m_pBrushBorder(nullptr)
    , m_pBrushTitle(nullptr)
    , m_pBrushText(nullptr)
    , m_pBrushDivider(nullptr)
    , m_pTitleFormat(nullptr)
    , m_pTextFormat(nullptr) {
    if (m_width <= 0) m_width = m_baseWidth;
    if (m_height <= 0) m_height = m_baseHeight;

    m_lines = {
        L"• 保持专注，优雅从容",
        L"• 享受纯净美观的桌面空间",
        L"• Macify 开源桌面套件"
    };
}

MemoWidget::~MemoWidget() {
    if (m_pTextFormat) m_pTextFormat->Release();
    if (m_pTitleFormat) m_pTitleFormat->Release();
    if (m_pBrushDivider) m_pBrushDivider->Release();
    if (m_pBrushText) m_pBrushText->Release();
    if (m_pBrushTitle) m_pBrushTitle->Release();
    if (m_pBrushBorder) m_pBrushBorder->Release();
    if (m_pBrushBg) m_pBrushBg->Release();
}

void MemoWidget::ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, D2D1_COLOR_F accent) {
    if (m_pBrushBg) m_pBrushBg->SetColor(bg);
    if (m_pBrushBorder) m_pBrushBorder->SetColor(border);
    if (m_pBrushTitle) m_pBrushTitle->SetColor(accent);
}

void MemoWidget::OnInit(ID2D1RenderTarget* pRT, IDWriteFactory* pDWrite) {
    if (pRT) {
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.08f, 0.09f, 0.12f, 0.72f), &m_pBrushBg);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.18f), &m_pBrushBorder);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.82f, 0.28f, 0.95f), &m_pBrushTitle); // Warm memo gold
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.92f, 0.93f, 0.96f, 0.92f), &m_pBrushText);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.10f), &m_pBrushDivider);
    }

    if (pDWrite) {
        pDWrite->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            12.0f,
            L"zh-CN",
            &m_pTitleFormat
        );

        pDWrite->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            11.0f,
            L"zh-CN",
            &m_pTextFormat
        );
    }
}

void MemoWidget::OnUpdate(float deltaTime) {
    (void)deltaTime;
}

void MemoWidget::OnRender(ID2D1RenderTarget* pRT) {
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

    // 2. Title: "📝 灵感便签"
    if (m_pTitleFormat && m_pBrushTitle) {
        D2D1_RECT_F titleRect = D2D1::RectF(m_x + 14.0f, m_y + 12.0f, m_x + m_baseWidth - 14.0f, m_y + 28.0f);
        pRT->DrawTextW(
            L"📝 灵感便签",
            6,
            m_pTitleFormat,
            titleRect,
            m_pBrushTitle
        );
    }

    // 3. Divider line
    if (m_pBrushDivider) {
        pRT->DrawLine(
            D2D1::Point2F(m_x + 14.0f, m_y + 32.0f),
            D2D1::Point2F(m_x + m_baseWidth - 14.0f, m_y + 32.0f),
            m_pBrushDivider,
            1.0f
        );
    }

    // 4. Memo Content lines
    if (m_pTextFormat && m_pBrushText) {
        float lineY = m_y + 38.0f;
        for (const auto& line : m_lines) {
            D2D1_RECT_F lineRect = D2D1::RectF(m_x + 14.0f, lineY, m_x + m_baseWidth - 14.0f, lineY + 18.0f);
            pRT->DrawTextW(
                line.c_str(),
                static_cast<UINT32>(line.length()),
                m_pTextFormat,
                lineRect,
                m_pBrushText
            );
            lineY += 24.0f;
        }
    }

    pRT->SetTransform(oldTrans);
}

void MemoWidget::OnMouseEvent(const MouseEvent& evt) {
    (void)evt;
}

} // namespace Widgets
