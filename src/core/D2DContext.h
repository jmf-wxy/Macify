#pragma once
#include <windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace Core {

class D2DContext {
public:
    D2DContext();
    ~D2DContext();

    bool Initialize(HWND hwnd);
    void Resize(UINT width, UINT height);
    void CleanupRenderTarget();

    ID2D1Factory* GetFactory() const { return m_pD2DFactory; }
    ID2D1HwndRenderTarget* GetRenderTarget() const { return m_pRenderTarget; }
    IDWriteFactory* GetDWriteFactory() const { return m_pDWriteFactory; }
    IWICImagingFactory* GetWICFactory() const { return m_pWICFactory; }

    void BeginDraw() {
        if (m_pRenderTarget) {
            m_pRenderTarget->BeginDraw();
        }
    }

    HRESULT EndDraw() {
        if (m_pRenderTarget) {
            return m_pRenderTarget->EndDraw();
        }
        return E_FAIL;
    }

private:
    HWND m_hwnd;
    ID2D1Factory* m_pD2DFactory;
    ID2D1HwndRenderTarget* m_pRenderTarget;
    IDWriteFactory* m_pDWriteFactory;
    IWICImagingFactory* m_pWICFactory;
};

} // namespace Core
