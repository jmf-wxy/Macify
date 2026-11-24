#include "DiskWidget.h"
#include <cwchar>

namespace Widgets {

DiskWidget::DiskWidget(float x, float y)
    : m_x(x)
    , m_y(y)
    , m_width(220.0f)
    , m_height(100.0f)
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

    m_height = 20.0f + m_disks.size() * 38.0f;
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

    // 1. Card backdrop
    D2D1_ROUNDED_RECT cardRect = D2D1::RoundedRect(
        D2D1::RectF(m_x, m_y, m_x + m_width, m_y + m_height),
        16.0f,
        16.0f
    );
    pRT->FillRoundedRectangle(cardRect, m_pBrushBg);
    pRT->DrawRoundedRectangle(cardRect, m_pBrushBorder, 1.2f);

    // 2. Render disks
    float curY = m_y + 12.0f;
    for (const auto& disk : m_disks) {
        // Label: "C: 系统", "D: 软件", "E: 游戏", etc.
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
                D2D1::RectF(m_x + 14.0f, curY, m_x + 95.0f, curY + 16.0f),
                m_pBrushText
            );
        }

        // Subtext: "余 24G/256G" right aligned
        wchar_t capBuf[64];
        swprintf_s(capBuf, L"余 %.0fG/%.0fG", disk.freeGB, disk.totalGB);
        if (m_pSubFormat && m_pBrushSubText) {
            pRT->DrawTextW(
                capBuf,
                static_cast<UINT32>(wcslen(capBuf)),
                m_pSubFormat,
                D2D1::RectF(m_x + 95.0f, curY, m_x + m_width - 14.0f, curY + 16.0f),
                m_pBrushSubText
            );
        }

        // Progress bar
        float barLeft = m_x + 14.0f;
        float barRight = m_x + m_width - 14.0f;
        float barTop = curY + 18.0f;
        float barHeight = 6.0f;

        // Background track
        D2D1_ROUNDED_RECT trackRect = D2D1::RoundedRect(
            D2D1::RectF(barLeft, barTop, barRight, barTop + barHeight),
            3.0f,
            3.0f
        );
        pRT->FillRoundedRectangle(trackRect, m_pBrushTrack);

        // Active filled bar
        float activeWidth = (barRight - barLeft) * disk.usedRatio;
        if (activeWidth > 2.0f) {
            D2D1_ROUNDED_RECT activeRect = D2D1::RoundedRect(
                D2D1::RectF(barLeft, barTop, barLeft + activeWidth, barTop + barHeight),
                3.0f,
                3.0f
            );
            pRT->FillRoundedRectangle(activeRect, m_pBrushBar);
        }

        curY += 36.0f;
    }
}

void DiskWidget::OnMouseEvent(const MouseEvent& evt) {
}

D2D1_RECT_F DiskWidget::GetBounds() const {
    return D2D1::RectF(m_x, m_y, m_x + m_width, m_y + m_height);
}

void DiskWidget::SetPosition(float x, float y) {
    m_x = x;
    m_y = y;
}

} // namespace Widgets
