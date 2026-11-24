#pragma once
#include "IWidget.h"
#include <dwrite.h>
#include <vector>

namespace Widgets {

struct DiskInfo {
    std::wstring letter;
    std::wstring label;
    double totalGB = 0.0;
    double freeGB = 0.0;
    float usedRatio = 0.0f;
};

class DiskWidget : public IWidget {
public:
    DiskWidget(float x = 60.0f, float y = 170.0f);
    ~DiskWidget() override;

    std::wstring GetId() const override { return L"widget_disk"; }
    std::wstring GetName() const override { return L"磁盘监控小组件"; }

    void OnInit(ID2D1RenderTarget* pRT, IDWriteFactory* pDWrite) override;
    void OnUpdate(float deltaTime) override;
    void OnRender(ID2D1RenderTarget* pRT) override;
    void OnMouseEvent(const MouseEvent& evt) override;

    D2D1_RECT_F GetBounds() const override;
    void SetPosition(float x, float y) override;
    bool IsEnabled() const override { return m_enabled; }
    void SetEnabled(bool enable) override { m_enabled = enable; }
    void SetRefreshInterval(float seconds) { m_refreshInterval = (seconds < 60.0f) ? 60.0f : seconds; }
    void ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, D2D1_COLOR_F barColor);

private:
    void RefreshDisks();

private:
    float m_x;
    float m_y;
    float m_width;
    float m_height;
    bool m_enabled;
    float m_refreshInterval;

    float m_timerAccumulator;
    std::vector<DiskInfo> m_disks;

    ID2D1SolidColorBrush* m_pBrushBg;
    ID2D1SolidColorBrush* m_pBrushBorder;
    ID2D1SolidColorBrush* m_pBrushTrack;
    ID2D1SolidColorBrush* m_pBrushBar;
    ID2D1SolidColorBrush* m_pBrushText;
    ID2D1SolidColorBrush* m_pBrushSubText;

    IDWriteTextFormat* m_pTitleFormat;
    IDWriteTextFormat* m_pSubFormat;
};

} // namespace Widgets
