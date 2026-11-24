#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <functional>
#include "../core/D2DContext.h"
#include "AppSettings.h"

namespace Settings {

struct SettingsCallbacks {
    std::function<void(const AppConfig&)> onDockChanged;
    std::function<void(const AppConfig&)> onDrawerChanged;
    std::function<void(const AppConfig&)> onWidgetsChanged;
    std::function<void(const AppConfig&)> onSystemChanged;
    std::function<void(const AppConfig&)> onAppearanceChanged;
    std::function<void()> onReloadApps;
    std::function<void()> onRestartApp;
    std::function<void()> onAddAppsFromExplorer;
};

class SettingsWindow {
public:
    SettingsWindow();
    ~SettingsWindow();

    bool Create();
    void Show(bool show = true);
    bool IsVisible() const;
    HWND GetHwnd() const { return m_hwnd; }

    void SetCallbacks(const SettingsCallbacks& cb) { m_callbacks = cb; }
    void SyncConfig(const AppConfig& cfg);
    const AppConfig& GetConfig() const { return m_config; }
    void ApplyTheme();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void Render();
    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);
    void OnMouseMove(int x, int y);
    void OnKeyDown(WPARAM vk);

    // Render helpers
    void DrawHeader(ID2D1RenderTarget* rt);
    void DrawSidebar(ID2D1RenderTarget* rt);
    void DrawTabContent(ID2D1RenderTarget* rt);

    void DrawGeneralTab(ID2D1RenderTarget* rt, float contentX, float contentY);
    void DrawDockTab(ID2D1RenderTarget* rt, float contentX, float contentY);
    void DrawDrawerTab(ID2D1RenderTarget* rt, float contentX, float contentY);
    void DrawWidgetsTab(ID2D1RenderTarget* rt, float contentX, float contentY);
    void DrawAppearanceTab(ID2D1RenderTarget* rt, float contentX, float contentY);
    void DrawAboutTab(ID2D1RenderTarget* rt, float contentX, float contentY);

    // UI Widget Drawing
    void DrawToggleRow(ID2D1RenderTarget* rt, float x, float y, float w, float h, 
                       const wchar_t* title, const wchar_t* desc, bool state, int toggleId);
    void DrawSliderRow(ID2D1RenderTarget* rt, float x, float y, float w, float h,
                       const wchar_t* title, const wchar_t* desc, float value, float minVal, float maxVal, 
                       const wchar_t* unit, int sliderId);
    void DrawSegmentedRow(ID2D1RenderTarget* rt, float x, float y, float w, float h,
                          const wchar_t* title, const wchar_t* desc, const std::vector<std::wstring>& options, int selectedIdx, int segId);
    void DrawButton(ID2D1RenderTarget* rt, float x, float y, float w, float h,
                    const wchar_t* text, bool isPrimary, int buttonId);

    void HandleToggleClick(int toggleId);
    void HandleSliderDrag(int sliderId, float ratio);
    void HandleSegmentClick(int segId, int optionIdx);
    void HandleButtonClick(int buttonId);

    void SaveAndApply();

private:
    HWND m_hwnd;
    Core::D2DContext m_d2d;
    AppConfig m_config;
    SettingsCallbacks m_callbacks;

    // Window dimensions
    float m_winWidth;
    float m_winHeight;

    // Navigation state
    int m_activeTab;
    int m_hoveredTab;

    // Interaction state
    int m_activeSlider;
    int m_hoveredButton;
    int m_mouseX;
    int m_mouseY;

    // Color theme
    D2D1_COLOR_F GetAccentColor() const;

    // Direct2D brushes
    ID2D1SolidColorBrush* m_pBrushBg;
    ID2D1SolidColorBrush* m_pBrushSidebarBg;
    ID2D1SolidColorBrush* m_pBrushCardBg;
    ID2D1SolidColorBrush* m_pBrushBorder;
    ID2D1SolidColorBrush* m_pBrushTextTitle;
    ID2D1SolidColorBrush* m_pBrushTextDesc;
    ID2D1SolidColorBrush* m_pBrushAccent;
    ID2D1SolidColorBrush* m_pBrushToggleOff;
    ID2D1SolidColorBrush* m_pBrushWhite;

    // DirectWrite Text Formats
    IDWriteTextFormat* m_pFormatTitle;
    IDWriteTextFormat* m_pFormatSubtitle;
    IDWriteTextFormat* m_pFormatItemTitle;
    IDWriteTextFormat* m_pFormatItemDesc;
    IDWriteTextFormat* m_pFormatTab;
    IDWriteTextFormat* m_pFormatButton;
};

} // namespace Settings