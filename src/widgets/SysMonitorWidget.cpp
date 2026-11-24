#include "SysMonitorWidget.h"
#include <cwchar>
#include <algorithm>

namespace Widgets {

static ULONGLONG FileTimeToUInt64(const FILETIME& ft) {
    return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

SysMonitorWidget::SysMonitorWidget(float x, float y, float width, float height)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_baseWidth(220.0f)
    , m_baseHeight(115.0f)
    , m_minWidth(140.0f)
    , m_minHeight(75.0f)
    , m_enabled(true)
    , m_timerAccumulator(2.0f)
    , m_cpuUsage(0.08f)
    , m_memUsage(0.45f)
    , m_memUsedGB(7.2)
    , m_memTotalGB(16.0)
    , m_prevSysIdle({})
    , m_prevSysKernel({})
    , m_prevSysUser({})
    , m_hasCpuSample(false)
    , m_pBrushBg(nullptr)
    , m_pBrushBorder(nullptr)
    , m_pBrushTrack(nullptr)
    , m_pBrushCpuBar(nullptr)
    , m_pBrushMemBar(nullptr)
    , m_pBrushText(nullptr)
    , m_pBrushSubText(nullptr)
    , m_pTitleFormat(nullptr)
    , m_pSubFormat(nullptr) {
    if (m_width <= 0) m_width = m_baseWidth;
    if (m_height <= 0) m_height = m_baseHeight;
}

SysMonitorWidget::~SysMonitorWidget() {
    if (m_pSubFormat) m_pSubFormat->Release();
    if (m_pTitleFormat) m_pTitleFormat->Release();
    if (m_pBrushSubText) m_pBrushSubText->Release();
    if (m_pBrushText) m_pBrushText->Release();
    if (m_pBrushMemBar) m_pBrushMemBar->Release();
    if (m_pBrushCpuBar) m_pBrushCpuBar->Release();
    if (m_pBrushTrack) m_pBrushTrack->Release();
    if (m_pBrushBorder) m_pBrushBorder->Release();
    if (m_pBrushBg) m_pBrushBg->Release();
}

void SysMonitorWidget::ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, D2D1_COLOR_F accent) {
    if (m_pBrushBg) m_pBrushBg->SetColor(bg);
    if (m_pBrushBorder) m_pBrushBorder->SetColor(border);
    if (m_pBrushCpuBar) m_pBrushCpuBar->SetColor(accent);
}

void SysMonitorWidget::OnInit(ID2D1RenderTarget* pRT, IDWriteFactory* pDWrite) {
    if (pRT) {
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.08f, 0.09f, 0.12f, 0.72f), &m_pBrushBg);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.18f), &m_pBrushBorder);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.12f), &m_pBrushTrack);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.00f, 0.55f, 1.0f, 0.90f), &m_pBrushCpuBar); // macOS Blue
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.65f, 0.35f, 0.95f, 0.90f), &m_pBrushMemBar); // Purple
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.95f, 0.95f, 0.98f, 0.95f), &m_pBrushText);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.7f, 0.73f, 0.8f, 0.8f), &m_pBrushSubText);
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
            10.5f,
            L"zh-CN",
            &m_pSubFormat
        );
        if (m_pSubFormat) {
            m_pSubFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
            m_pSubFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    SampleCpuAndMemory();
}

void SysMonitorWidget::SampleCpuAndMemory() {
    // 1. Memory status
    MEMORYSTATUSEX memInfo = {};
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        m_memUsage = static_cast<float>(memInfo.dwMemoryLoad) / 100.0f;
        m_memTotalGB = static_cast<double>(memInfo.ullTotalPhys) / (1024.0 * 1024.0 * 1024.0);
        double availGB = static_cast<double>(memInfo.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0);
        m_memUsedGB = m_memTotalGB - availGB;
    }

    // 2. CPU status
    FILETIME idleTime = {}, kernelTime = {}, userTime = {};
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        if (m_hasCpuSample) {
            ULONGLONG idle = FileTimeToUInt64(idleTime) - FileTimeToUInt64(m_prevSysIdle);
            ULONGLONG kernel = FileTimeToUInt64(kernelTime) - FileTimeToUInt64(m_prevSysKernel);
            ULONGLONG user = FileTimeToUInt64(userTime) - FileTimeToUInt64(m_prevSysUser);
            ULONGLONG total = kernel + user;

            if (total > 0 && total >= idle) {
                float sampleCpu = static_cast<float>(total - idle) / static_cast<float>(total);
                // Exponential smoothing for pleasing smooth visual transitions
                m_cpuUsage = m_cpuUsage * 0.4f + sampleCpu * 0.6f;
            }
        }
        m_prevSysIdle = idleTime;
        m_prevSysKernel = kernelTime;
        m_prevSysUser = userTime;
        m_hasCpuSample = true;
    }
}

void SysMonitorWidget::OnUpdate(float deltaTime) {
    m_timerAccumulator += deltaTime;
    // Update performance stats every 1.5 seconds to keep 0% CPU overhead
    if (m_timerAccumulator >= 1.5f) {
        m_timerAccumulator = 0.0f;
        SampleCpuAndMemory();
    }
}

void SysMonitorWidget::OnRender(ID2D1RenderTarget* pRT) {
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

    float barLeft = m_x + 14.0f;
    float barRight = m_x + m_baseWidth - 14.0f;
    float barHeight = 6.0f;

    // Row 1: CPU
    float curY = m_y + 16.0f;
    if (m_pTitleFormat && m_pBrushText) {
        pRT->DrawTextW(
            L"CPU 负载",
            6,
            m_pTitleFormat,
            D2D1::RectF(m_x + 14.0f, curY, m_x + 95.0f, curY + 16.0f),
            m_pBrushText
        );
    }
    wchar_t cpuBuf[32];
    swprintf_s(cpuBuf, L"%d%%", static_cast<int>(m_cpuUsage * 100.0f + 0.5f));
    if (m_pSubFormat && m_pBrushSubText) {
        pRT->DrawTextW(
            cpuBuf,
            static_cast<UINT32>(wcslen(cpuBuf)),
            m_pSubFormat,
            D2D1::RectF(m_x + 95.0f, curY, barRight, curY + 16.0f),
            m_pBrushSubText
        );
    }

    float barTop1 = curY + 18.0f;
    D2D1_ROUNDED_RECT trackRect1 = D2D1::RoundedRect(
        D2D1::RectF(barLeft, barTop1, barRight, barTop1 + barHeight),
        3.0f, 3.0f
    );
    if (m_pBrushTrack) pRT->FillRoundedRectangle(trackRect1, m_pBrushTrack);

    float activeW1 = (barRight - barLeft) * std::clamp(m_cpuUsage, 0.0f, 1.0f);
    if (activeW1 > 2.0f && m_pBrushCpuBar) {
        D2D1_ROUNDED_RECT activeRect1 = D2D1::RoundedRect(
            D2D1::RectF(barLeft, barTop1, barLeft + activeW1, barTop1 + barHeight),
            3.0f, 3.0f
        );
        pRT->FillRoundedRectangle(activeRect1, m_pBrushCpuBar);
    }

    // Row 2: RAM
    curY += 40.0f;
    if (m_pTitleFormat && m_pBrushText) {
        pRT->DrawTextW(
            L"物理内存",
            4,
            m_pTitleFormat,
            D2D1::RectF(m_x + 14.0f, curY, m_x + 95.0f, curY + 16.0f),
            m_pBrushText
        );
    }
    wchar_t memBuf[48];
    swprintf_s(memBuf, L"%.1fG/%.0fG (%d%%)", m_memUsedGB, m_memTotalGB, static_cast<int>(m_memUsage * 100.0f));
    if (m_pSubFormat && m_pBrushSubText) {
        pRT->DrawTextW(
            memBuf,
            static_cast<UINT32>(wcslen(memBuf)),
            m_pSubFormat,
            D2D1::RectF(m_x + 95.0f, curY, barRight, curY + 16.0f),
            m_pBrushSubText
        );
    }

    float barTop2 = curY + 18.0f;
    D2D1_ROUNDED_RECT trackRect2 = D2D1::RoundedRect(
        D2D1::RectF(barLeft, barTop2, barRight, barTop2 + barHeight),
        3.0f, 3.0f
    );
    if (m_pBrushTrack) pRT->FillRoundedRectangle(trackRect2, m_pBrushTrack);

    float activeW2 = (barRight - barLeft) * std::clamp(m_memUsage, 0.0f, 1.0f);
    if (activeW2 > 2.0f && m_pBrushMemBar) {
        D2D1_ROUNDED_RECT activeRect2 = D2D1::RoundedRect(
            D2D1::RectF(barLeft, barTop2, barLeft + activeW2, barTop2 + barHeight),
            3.0f, 3.0f
        );
        pRT->FillRoundedRectangle(activeRect2, m_pBrushMemBar);
    }

    pRT->SetTransform(oldTrans);
}

void SysMonitorWidget::OnMouseEvent(const MouseEvent& evt) {
    (void)evt;
}

} // namespace Widgets
