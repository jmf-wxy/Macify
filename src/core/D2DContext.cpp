#include "D2DContext.h"

namespace Core {

D2DContext::D2DContext()
    : m_hwnd(nullptr)
    , m_pD2DFactory(nullptr)
    , m_pRenderTarget(nullptr)
    , m_pDWriteFactory(nullptr)
    , m_pWICFactory(nullptr) {
}

D2DContext::~D2DContext() {
    CleanupRenderTarget();

    if (m_pWICFactory) {
        m_pWICFactory->Release();
        m_pWICFactory = nullptr;
    }
    if (m_pDWriteFactory) {
        m_pDWriteFactory->Release();
        m_pDWriteFactory = nullptr;
    }
    if (m_pD2DFactory) {
        m_pD2DFactory->Release();
        m_pD2DFactory = nullptr;
    }
}

bool D2DContext::Initialize(HWND hwnd) {
    m_hwnd = hwnd;

    // 1. Create Direct2D factory
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
    if (FAILED(hr)) return false;

    // 2. Create DirectWrite factory
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&m_pDWriteFactory)
    );
    if (FAILED(hr)) return false;

    // 3. Create WIC Imaging factory (for loading and decoding png / icons)
    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_pWICFactory)
    );
    if (FAILED(hr)) return false;

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    Resize(rc.right - rc.left, rc.bottom - rc.top);

    return true;
}

void D2DContext::Resize(UINT width, UINT height) {
    if (width == 0 || height == 0) return;

    if (m_pRenderTarget) {
        m_pRenderTarget->Resize(D2D1::SizeU(width, height));
    } else if (m_pD2DFactory && m_hwnd) {
        D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
            m_hwnd,
            D2D1::SizeU(width, height),
            D2D1_PRESENT_OPTIONS_IMMEDIATELY
        );

        m_pD2DFactory->CreateHwndRenderTarget(rtProps, hwndProps, &m_pRenderTarget);
        if (m_pRenderTarget) {
            m_pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            m_pRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        }
    }
}

void D2DContext::CleanupRenderTarget() {
    if (m_pRenderTarget) {
        m_pRenderTarget->Release();
        m_pRenderTarget = nullptr;
    }
}

} // namespace Core
