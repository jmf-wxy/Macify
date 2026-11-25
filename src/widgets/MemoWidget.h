#pragma once
#include "IWidget.h"
#include <dwrite.h>
#include <vector>
#include <string>
#include <functional>

namespace Widgets {

struct MemoItem {
    std::wstring text;
    bool done = false;
};

class MemoWidget : public IWidget {
public:
    using OnMemoChangedCallback = std::function<void(const std::vector<MemoItem>&)>;

    MemoWidget(float x = 40.0f, float y = 580.0f, float width = 220.0f, float height = 120.0f);
    ~MemoWidget() override;

    std::wstring GetId() const override { return L"widget_memo"; }
    std::wstring GetName() const override { return L"桌面便签小组件"; }

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

    void SetItems(const std::vector<MemoItem>& items);
    const std::vector<MemoItem>& GetItems() const { return m_items; }
    void AddItem(const std::wstring& text);
    void ToggleItem(size_t index);
    void RemoveItem(size_t index);
    void ClearDone();
    void ResetDefaults();
    void SetOnChanged(OnMemoChangedCallback cb) { m_onChanged = cb; }

private:
    void UpdateBaseHeight();
    void CopyAllToClipboard();
    void ShowContextMenu(int screenX, int screenY);

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
    std::vector<MemoItem> m_items;
    OnMemoChangedCallback m_onChanged;

    int m_hoveredIndex;
    bool m_hoverAddBtn;
    bool m_hoverClearBtn;
    bool m_hoverDeleteBtn;
    int m_mouseX;
    int m_mouseY;

    ID2D1SolidColorBrush* m_pBrushBg;
    ID2D1SolidColorBrush* m_pBrushBorder;
    ID2D1SolidColorBrush* m_pBrushTitle;
    ID2D1SolidColorBrush* m_pBrushText;
    ID2D1SolidColorBrush* m_pBrushTextDone;
    ID2D1SolidColorBrush* m_pBrushDivider;
    ID2D1SolidColorBrush* m_pBrushAccent;
    ID2D1SolidColorBrush* m_pBrushHover;

    IDWriteTextFormat* m_pTitleFormat;
    IDWriteTextFormat* m_pTextFormat;
    IDWriteTextFormat* m_pSubFormat;
    IDWriteTextFormat* m_pIconFormat;
};

} // namespace Widgets
