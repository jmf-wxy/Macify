#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include "../core/D2DContext.h"
#include "AppItem.h"

namespace Drawer {

class DrawerWindow {
public:
    DrawerWindow();
    ~DrawerWindow();

    bool Create();
    void Toggle();
    void Show(bool show = true);
    bool IsVisible() const;
    HWND GetHwnd() const { return m_hwnd; }

    void ReloadApps();
    void ApplySettings(int columns, float iconSize, bool autoCloseOnBlur, bool autoFocusSearch);
    void ApplyTheme(D2D1_COLOR_F bg, D2D1_COLOR_F border, float cornerRadius = 22.0f);
    bool AddCustomApp(const std::wstring& filePath, const std::wstring& customName = L"");
    void AddCustomAppsFromDialog();
    void RemoveCustomApp(const std::wstring& targetPath);
    void DeleteApp(const std::wstring& name, const std::wstring& targetPath);

    void SetOnDragStart(std::function<void(const std::wstring& name, const std::wstring& targetPath, const std::wstring& iconPath, int screenX, int screenY)> cb) { m_onDragStart = cb; }
    void SetOnDragMove(std::function<void(int screenX, int screenY)> cb) { m_onDragMove = cb; }
    void SetOnDropApp(std::function<void(const std::wstring& name, const std::wstring& path, int screenX, int screenY)> cb) { m_onDropApp = cb; }
    void SetOnDragEnd(std::function<void()> cb) { m_onDragEnd = cb; }
    void SetOnOpenSettings(std::function<void()> cb) { m_onOpenSettings = cb; }
    void SetOnPinToDock(std::function<void(const std::wstring& name, const std::wstring& path)> cb) { m_onPinToDock = cb; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void Render();
    void FilterApps();
    void LaunchSelected();

    void OnKeyDown(WPARAM vk);
    void OnChar(wchar_t ch);
    void OnMouseMove(int x, int y);
    void OnMouseWheel(short delta);
    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);
    void OnRButtonUp(int x, int y);
    void OnDropFiles(HDROP hDrop);

private:
    HWND m_hwnd;
    Core::D2DContext m_d2d;

    std::function<void(const std::wstring& name, const std::wstring& targetPath, const std::wstring& iconPath, int screenX, int screenY)> m_onDragStart;
    std::function<void(int screenX, int screenY)> m_onDragMove;
    std::function<void(const std::wstring& name, const std::wstring& path, int screenX, int screenY)> m_onDropApp;
    std::function<void()> m_onDragEnd;
    std::function<void()> m_onOpenSettings;
    std::function<void(const std::wstring& name, const std::wstring& path)> m_onPinToDock;

    // Drag tracking
    int m_dragIndex;
    POINT m_dragStart;
    POINT m_dragCurrent;
    bool m_isDragging;

    std::vector<std::unique_ptr<AppItem>> m_allApps;
    std::vector<AppItem*> m_filteredApps;

    // Window metrics
    float m_winWidth;
    float m_winHeight;

    // Grid metrics
    int m_columns;
    float m_cellWidth;
    float m_cellHeight;
    float m_gridPaddingX;
    float m_iconSize;
    bool m_autoCloseOnBlur;
    bool m_autoFocusSearch;
    float m_cornerRadius;

    // Search state
    std::wstring m_searchText;
    bool m_caretVisible;
    int m_selectedIndex;
    int m_hoveredIndex;

    // Scrolling state
    float m_scrollOffset;
    float m_listTop;
    float m_listBottom;

    // Brushes & Text Formats
    ID2D1SolidColorBrush* m_pBrushBg;
    ID2D1SolidColorBrush* m_pBrushBorder;
    ID2D1SolidColorBrush* m_pBrushSearchBg;
    ID2D1SolidColorBrush* m_pBrushSearchBorder;
    ID2D1SolidColorBrush* m_pBrushSearchText;
    ID2D1SolidColorBrush* m_pBrushPlaceholder;
    ID2D1SolidColorBrush* m_pBrushCaret;
    ID2D1SolidColorBrush* m_pBrushHover;
    ID2D1SolidColorBrush* m_pBrushSelected;
    ID2D1SolidColorBrush* m_pBrushItemText;
    ID2D1SolidColorBrush* m_pBrushScrollThumb;
    ID2D1SolidColorBrush* m_pBrushAddBtn;
    ID2D1SolidColorBrush* m_pBrushAddBtnHover;
    ID2D1SolidColorBrush* m_pBrushDeleteBadge;
    ID2D1SolidColorBrush* m_pBrushDeleteBadgeHover;
    ID2D1SolidColorBrush* m_pBrushWhite;

    IDWriteTextFormat* m_pSearchFormat;
    IDWriteTextFormat* m_pItemFormat;
    IDWriteTextFormat* m_pEmptyFormat;
    IDWriteTextFormat* m_pAddBtnFormat;

    D2D1_RECT_F m_btnAddRect;
    bool m_isAddBtnHovered;
    bool m_isHoveringDeleteBtn;
    int m_hoveredDeleteIndex;
};

} // namespace Drawer
