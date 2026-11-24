#pragma once
#include <windows.h>
#include <string>
#include "D2DContext.h"
#include "Backdrop.h"

namespace Core {

class DragOverlay {
public:
    DragOverlay()
        : m_hwnd(nullptr)
        , m_pBitmap(nullptr)
        , m_size(72.0f)
        , m_pBrushBg(nullptr)
        , m_pBrushBorder(nullptr) {}

    ~DragOverlay() {
        if (m_pBitmap) {
            m_pBitmap->Release();
            m_pBitmap = nullptr;
        }
        if (m_pBrushBg) {
            m_pBrushBg->Release();
            m_pBrushBg = nullptr;
        }
        if (m_pBrushBorder) {
            m_pBrushBorder->Release();
            m_pBrushBorder = nullptr;
        }
        if (m_hwnd && IsWindow(m_hwnd)) {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }
    }

    bool Create(HINSTANCE hInstance = nullptr) {
        if (m_hwnd) return true;
        if (!hInstance) hInstance = GetModuleHandleW(nullptr);

        const wchar_t CLASS_NAME[] = L"MinimalMacDragOverlay";

        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClassExW(&wc);

        m_hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            CLASS_NAME,
            L"DragOverlay",
            WS_POPUP,
            -2000, -2000,
            static_cast<int>(m_size), static_cast<int>(m_size),
            nullptr, nullptr, hInstance, this
        );

        if (!m_hwnd) return false;

        Core::RemoveWindowBorder(m_hwnd);
        if (!m_d2d.Initialize(m_hwnd)) return false;

        auto rt = m_d2d.GetRenderTarget();
        if (rt) {
            rt->CreateSolidColorBrush(D2D1::ColorF(0.10f, 0.12f, 0.18f, 0.88f), &m_pBrushBg);
            rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.35f), &m_pBrushBorder);
        }

        return true;
    }

    void LoadIconFromPath(const std::wstring& path) {
        if (m_pBitmap) {
            m_pBitmap->Release();
            m_pBitmap = nullptr;
        }
        auto rt = m_d2d.GetRenderTarget();
        auto wic = m_d2d.GetWICFactory();
        if (!rt || !wic || path.empty()) return;

        HICON hIcon = nullptr;
        UINT iconId = 0;
        PrivateExtractIconsW(path.c_str(), 0, 96, 96, &hIcon, &iconId, 1, 0);
        if (!hIcon) {
            SHFILEINFOW sfi = {};
            SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON);
            hIcon = sfi.hIcon;
        }
        if (!hIcon) {
            hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        }
        if (hIcon) {
            IWICBitmap* pWicBitmap = nullptr;
            if (SUCCEEDED(wic->CreateBitmapFromHICON(hIcon, &pWicBitmap)) && pWicBitmap) {
                IWICFormatConverter* pConverter = nullptr;
                if (SUCCEEDED(wic->CreateFormatConverter(&pConverter)) && pConverter) {
                    if (SUCCEEDED(pConverter->Initialize(
                        pWicBitmap,
                        GUID_WICPixelFormat32bppPBGRA,
                        WICBitmapDitherTypeNone,
                        nullptr,
                        0.0f,
                        WICBitmapPaletteTypeCustom))) {
                        rt->CreateBitmapFromWicBitmap(pConverter, nullptr, &m_pBitmap);
                    }
                    pConverter->Release();
                }
                pWicBitmap->Release();
            }
            DestroyIcon(hIcon);
        }
    }

    void StartDrag(const std::wstring& name, const std::wstring& path, int screenX, int screenY) {
        if (!m_hwnd) {
            Create();
        }
        m_name = name;
        LoadIconFromPath(path);

        int half = static_cast<int>(m_size / 2.0f);
        SetWindowPos(m_hwnd, HWND_TOPMOST, screenX - half, screenY - half,
            static_cast<int>(m_size), static_cast<int>(m_size),
            SWP_SHOWWINDOW | SWP_NOACTIVATE);
        Render();
    }

    void UpdateDrag(int screenX, int screenY) {
        if (!m_hwnd || !IsWindowVisible(m_hwnd)) return;
        int half = static_cast<int>(m_size / 2.0f);
        SetWindowPos(m_hwnd, HWND_TOPMOST, screenX - half, screenY - half,
            0, 0,
            SWP_NOSIZE | SWP_NOACTIVATE);
        Render();
    }

    void EndDrag() {
        if (m_hwnd && IsWindowVisible(m_hwnd)) {
            ShowWindow(m_hwnd, SW_HIDE);
        }
        if (m_pBitmap) {
            m_pBitmap->Release();
            m_pBitmap = nullptr;
        }
    }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        DragOverlay* pThis = nullptr;
        if (msg == WM_NCCREATE) {
            auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pThis = reinterpret_cast<DragOverlay*>(cs->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
            pThis->m_hwnd = hwnd;
            return TRUE;
        } else {
            pThis = reinterpret_cast<DragOverlay*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (msg == WM_NCHITTEST) {
            return HTTRANSPARENT;
        }
        if (msg == WM_PAINT) {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            if (pThis) pThis->Render();
            EndPaint(hwnd, &ps);
            return 0;
        }
        if (msg == WM_ERASEBKGND) return 1;
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    void Render() {
        auto rt = m_d2d.GetRenderTarget();
        if (!rt) return;

        m_d2d.BeginDraw();
        rt->Clear(D2D1::ColorF(0, 0, 0, 0.0f));

        // Draw translucent floating card under cursor
        D2D1_ROUNDED_RECT card = D2D1::RoundedRect(
            D2D1::RectF(4.0f, 4.0f, m_size - 4.0f, m_size - 4.0f),
            16.0f, 16.0f
        );

        if (m_pBrushBg) rt->FillRoundedRectangle(card, m_pBrushBg);
        if (m_pBrushBorder) rt->DrawRoundedRectangle(card, m_pBrushBorder, 1.5f);

        // Draw icon in center
        if (m_pBitmap) {
            float iconMargin = 12.0f;
            rt->DrawBitmap(
                m_pBitmap,
                D2D1::RectF(iconMargin, iconMargin, m_size - iconMargin, m_size - iconMargin),
                0.96f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
            );
        }

        m_d2d.EndDraw();
    }

    HWND m_hwnd;
    Core::D2DContext m_d2d;
    ID2D1Bitmap* m_pBitmap;
    std::wstring m_name;
    float m_size;
    ID2D1SolidColorBrush* m_pBrushBg;
    ID2D1SolidColorBrush* m_pBrushBorder;
};

} // namespace Core
