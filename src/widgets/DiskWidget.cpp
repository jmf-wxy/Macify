#include "DiskWidget.h"
#include <cwchar>

namespace Widgets {

DiskWidget::DiskWidget(float x, float y, float width, float height)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_baseWidth(220.0f)
    , m_baseHeight(156.0f)
    , m_minWidth(140.0f)
    , m_minHeight(75.0f)
    , m_enabled(true)
    , m_refreshInterval(900.0f)
    , m_timerAccumulator(900.0f)
    , m_pBrushBg(nullptr)
    , m_pBrushBorder(nullptr)
    , m_pBrushTrack(nullptr)
    , m_pBrushBar(nullptr)
    , m_pBrushText(nullptr)
    , m_pBrushSubText(nullptr)
    , m_pTitleFormat(nullptr)
    , m_pSubFormat(nullptr) {
    if (m_width <= 0) m_width = m_baseWidth;
    if (m_height <= 0) m_height = m_baseHeight;
}

DiskWidget::~DiskWidget() {
    if (m_pSubFormat) m_pSubFormat->Release();
    if (m_pTitleFormat) m_pTitleFormat->Release();
    if (m_pBrushSubText) m_pBrushSubText->Release();
    if (m_pBrushText) m_pBrushText->Release();
    if (m_pBrushBar) m_pBrushBar->Release();
    if (m_pBrushTrack) m_pBrushTrack->Release();
    if (m_pBrushBorder) m_pBrushBorder->Release();
    if (m_pBrushBg) m_pBrushBg->Release();
}

void DiskWidget::ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, D2D1_COLOR_F barColor) {
    if (m_pBrushBg) m_pBrushBg->SetColor(bg);
    if (m_pBrushBorder) m_pBrushBorder->SetColor(border);
    if (m_pBrushBar) m_pBrushBar->SetColor(barColor);
}

void DiskWidget::OnInit(ID2D1RenderTarget* pRT, IDWriteFactory* pDWrite) {
    if (pRT) {
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.08f, 0.09f, 0.12f, 0.72f), &m_pBrushBg);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.18f), &m_pBrushBorder);
        pRT->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.12f), &m_pBrushTrack);
        pRT->CreateSolidColorBrush(D2D1::ColorF(0.25f, 0.65f, 1.0f, 0.85f), &m_pBrushBar); // Clean macOS Blue
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

    RefreshDisks();
}

void DiskWidget::RefreshDisks() {
    m_disks.clear();

    wchar_t drivesBuffer[512] = {};
    DWORD len = GetLogicalDriveStringsW(511, drivesBuffer);
    if (len > 0) {
        wchar_t* pDrive = drivesBuffer;
        while (*pDrive) {
            UINT type = GetDriveTypeW(pDrive);
            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE) {
                ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
                if (GetDiskFreeSpaceExW(pDrive, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes) && totalNumberOfBytes.QuadPart > 0) {
                    DiskInfo info;
                    info.letter = std::wstring(pDrive, 2); // e.g. "C:"

                    wchar_t volName[MAX_PATH] = {};
                    if (GetVolumeInformationW(pDrive, volName, MAX_PATH, nullptr, nullptr, nullptr, nullptr, 0) && wcslen(volName) > 0) {
                        info.label = volName;
                    }

                    info.totalGB = static_cast<double>(totalNumberOfBytes.QuadPart) / (1024.0 * 1024.0 * 1024.0);
                    info.freeGB = static_cast<double>(totalNumberOfFreeBytes.QuadPart) / (1024.0 * 1024.0 * 1024.0);
                    double usedGB = info.totalGB - info.freeGB;
                    info.usedRatio = static_cast<float>(usedGB / info.totalGB);
                    if (info.usedRatio < 0.0f) info.usedRatio = 0.0f;
                    if (info.usedRatio > 1.0f) info.usedRatio = 1.0f;
                    m_disks.push_back(info);
                }
            }
            pDrive += wcslen(pDrive) + 1;
        }
    }

    // Support all drives (C, D, E, F, etc.) with up to 8 drives max
    if (m_disks.size() > 8) {
        m_disks.resize(8);
    }

    // Dynamic baseHeight: 12px top/bottom padding + 36px per drive
    m_baseHeight = 12.0f + static_cast<float>(m_disks.size()) * 36.0f;
    if (m_baseHeight < 75.0f) m_baseHeight = 75.0f;
    m_minHeight = 12.0f + static_cast<float>(m_disks.size()) * 20.0f;

    // If m_height was around the old 3-disk default (<= 125px) and we have 4+ drives, adapt height
    if (m_height <= 125.0f && m_disks.size() >= 4) {
        m_height = m_baseHeight;
    }
}

void DiskWidget::OnUpdate(float deltaTime) {
    m_timerAccumulator += deltaTime;
    // Check disk usage based on configured refresh interval (e.g. 10 - 30 minutes)
    if (m_timerAccumulator >= m_refreshInterval) {
        m_timerAccumulator = 0.0f;
        RefreshDisks();
    }
}

void DiskWidget::OnRender(ID2D1RenderTarget* pRT) {
    if (!m_enabled || !pRT) return;

    D2D1_MATRIX_3X2_F oldTrans;
    pRT->GetTransform(&oldTrans);

    float scaleX = (m_baseWidth > 0) ? (m_width / m_baseWidth) : 1.0f;
    float scaleY = (m_baseHeight > 0) ? (m_height / m_baseHeight) : 1.0f;
    float scale = (std::min)(scaleX, scaleY);
    if (scale <= 0.01f) scale = 1.0f;
    D2D1_MATRIX_3X2_F scaleTrans = D2D1::Matrix3x2F::Scale(scale, scale, D2D1::Point2F(m_x, m_y));
    pRT->SetTransform(scaleTrans * oldTrans);

    float virtualW = m_width / scale;
    float virtualH = m_height / scale;

    // 1. Card backdrop
    D2D1_ROUNDED_RECT cardRect = D2D1::RoundedRect(
        D2D1::RectF(m_x, m_y, m_x + virtualW, m_y + virtualH),
        16.0f,
        16.0f
    );
    if (m_pBrushBg) pRT->FillRoundedRectangle(cardRect, m_pBrushBg);
    if (m_pBrushBorder) pRT->DrawRoundedRectangle(cardRect, m_pBrushBorder, 1.2f);

    // 2. Render disks
    float curY = m_y + 12.0f;
    for (const auto& disk : m_disks) {
        wchar_t labelBuf[64];
        if (disk.letter == L"C:" || disk.label.find(L"Win") != std::wstring::npos) {
            swprintf_s(labelBuf, L"%s 系统", disk.letter.c_str());
        } else if (!disk.label.empty()) {
            std::wstring cleanLabel = disk.label.substr(0, 5);
            swprintf_s(labelBuf, L"%s %s", disk.letter.c_str(), cleanLabel.c_str());
        } else {
            swprintf_s(labelBuf, L"%s 盘", disk.letter.c_str());
        }
        if (m_pTitleFormat && m_pBrushText) {
            pRT->DrawTextW(
                labelBuf,
                static_cast<UINT32>(wcslen(labelBuf)),
                m_pTitleFormat,
                D2D1::RectF(m_x + 14.0f, curY, m_x + 105.0f, curY + 16.0f),
                m_pBrushText
            );
        }

        // Subtext: "24G/256G" right aligned (without '余')
        wchar_t capBuf[64];
        swprintf_s(capBuf, L"%.0fG/%.0fG", disk.freeGB, disk.totalGB);
        if (m_pSubFormat && m_pBrushSubText) {
            pRT->DrawTextW(
                capBuf,
                static_cast<UINT32>(wcslen(capBuf)),
                m_pSubFormat,
                D2D1::RectF(m_x + 95.0f, curY, m_x + virtualW - 14.0f, curY + 16.0f),
                m_pBrushSubText
            );
        }

        // Progress bar
        float barLeft = m_x + 14.0f;
        float barRight = m_x + virtualW - 14.0f;
        float barTop = curY + 18.0f;
        float barHeight = 6.0f;

        // Background track
        D2D1_ROUNDED_RECT trackRect = D2D1::RoundedRect(
            D2D1::RectF(barLeft, barTop, barRight, barTop + barHeight),
            3.0f,
            3.0f
        );
        if (m_pBrushTrack) pRT->FillRoundedRectangle(trackRect, m_pBrushTrack);

        // Active filled bar
        float activeWidth = (barRight - barLeft) * disk.usedRatio;
        if (activeWidth > 2.0f && m_pBrushBar) {
            D2D1_ROUNDED_RECT activeRect = D2D1::RoundedRect(
                D2D1::RectF(barLeft, barTop, barLeft + activeWidth, barTop + barHeight),
                3.0f,
                3.0f
            );
            pRT->FillRoundedRectangle(activeRect, m_pBrushBar);
        }

        curY += 36.0f;
    }

    pRT->SetTransform(oldTrans);
}

void DiskWidget::OnMouseEvent(const MouseEvent& evt) {
    (void)evt;
}

} // namespace Widgets
