#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <functional>
#include "../core/D2DContext.h"
#include "DockItem.h"

namespace Dock {

class DockWindow {
public:
    DockWindow();
    ~DockWindow();

    bool Create();
    void Show(bool show = true);
    void Suspend();
    void Resume();
    HWND GetHwnd() const { return m_hwnd; }

    void AddItem(const std::wstring& name, const std::wstring& path, ItemAction action);
    bool PinApp(const std::wstring& name, const std::wstring& path);
    bool PinAppAtIndex(const std::wstring& name, const std::wstring& path, int targetIndex = -1);
    void OnExternalDragMove(int screenX, int screenY);
    void OnExternalDragLeave();
    int GetInsertIndex() const { return m_insertIndex; }
    void TriggerVanishEffect(int itemIndex);
    void LoadConfig();
    void SaveConfig();
    void UpdateLayout();
    void SetOnToggleLaunchpad(std::function<void()> callback) { m_onToggleLaunchpad = callback; }
    void SetIsLaunchpadOpenCallback(std::function<bool()> callback) { m_isLaunchpadOpen = callback; }
    void SetOnOpenSettings(std::function<void()> callback) { m_onOpenSettings = callback; }
    void ApplySettings(float baseSize, float maxSize, float spread, float bottomMargin, 
                       float barVisualHeight, float bgOpacity, bool showDot, bool showTooltip, bool autoHide);
    void ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, float cornerRadius = 26.0f);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void Render();
    void OnMouseMove(int x, int y);
    void OnMouseLeave();
    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);
    void OnRButtonUp(int x, int y);
    void AnimateStep();

    void UpdateLayoutPosition();
    bool CheckOverlap();
    void UpdateAutoHide();
    void AnimateSlide();

    void DrawLaunchpadIcon(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect);
    void DrawSettingsIcon(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect);
    void DrawPowerIcon(ID2D1RenderTarget* rt, const D2D1_RECT_F& rect);

    void PerformShutdown();
    void PerformReboot();
    void PerformSleep();

private:
    HWND m_hwnd;
    Core::D2DContext m_d2d;

    std::vector<std::unique_ptr<DockItem>> m_items;
    std::function<void()> m_onToggleLaunchpad;
    std::function<bool()> m_isLaunchpadOpen;
    std::function<void()> m_onOpenSettings;

    // Configurable parameters
    float m_baseSize;
    float m_maxSize;
    float m_spread;
    float m_padding;
    float m_endPadding;
    float m_barVisualHeight;
    float m_bottomMargin;
    float m_barHeight;
    float m_barWidth;
    float m_bgOpacity;
    int m_lastD2DWidth;
    int m_lastD2DHeight;
    bool m_showDot;
    bool m_showTooltip;
    bool m_autoHideEnabled;
    float m_cornerRadius;
    D2D1_COLOR_F m_themeBgColor;
    D2D1_COLOR_F m_themeBorderColor;

    // Animation & Interaction state
    bool m_isMouseTracking;
    bool m_isMouseInside;
    int m_mouseX;
    int m_mouseY;
    bool m_isAnimating;
    int m_hoveredIndex;
    bool m_isContextMenuOpen;

    // Auto-hide & Slide state
    bool m_isOverlapped;
    float m_hideProgress;       // 0.0f = visible, 1.0f = hidden at bottom
    float m_targetHideProgress; // Target
    int m_hideGraceFrames;
    bool m_isSlideAnimating;

    // External Drag & Insertion Preview
    bool m_externalDragActive;
    int m_insertIndex;
    float m_insertGap;
    float m_targetInsertGap;

    // Internal Drag (Reorder & Drag out to remove)
    int m_dragItemIndex;
    POINT m_dragStartPos;
    float m_dragItemCurrentX;
    float m_dragItemCurrentY;
    bool m_isDraggingItem;
    bool m_isDragItemOutOfDock;

    // Vanish / Poof particle effects
    struct SmokeParticle {
        float x, y;
        float vx, vy;
        float radius;
        float alpha;
    };
    struct VanishEffect {
        float centerX = 0.0f;
        float centerY = 0.0f;
        float progress = 0.0f;
        ID2D1Bitmap* pBitmap = nullptr;
        std::vector<SmokeParticle> particles;
    };
    std::vector<VanishEffect> m_vanishEffects;

    // Direct2D cached brushes
    ID2D1SolidColorBrush* m_pBrushBg;
    ID2D1SolidColorBrush* m_pBrushBorder;
    ID2D1SolidColorBrush* m_pBrushDot;
    ID2D1SolidColorBrush* m_pBrushText;
    ID2D1SolidColorBrush* m_pBrushSmoke;
    ID2D1SolidColorBrush* m_pBrushInsertSlot;
    IDWriteTextFormat* m_pTextFormatTooltip;
};

} // namespace Dock
