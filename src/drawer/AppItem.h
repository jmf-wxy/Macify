#pragma once
#include <string>
#include <windows.h>
#include <d2d1.h>
#include <wincodec.h>
#include <commctrl.h>
#include <commoncontrols.h>

namespace Drawer {

struct AppItem {
    std::wstring name;
    std::wstring targetPath; // Used for launching (e.g. .lnk or .exe)
    std::wstring iconPath;   // Used for clean icon extraction (.exe without shortcut arrow)
    std::wstring searchKey;  // Lowercase name for quick matching
    
    // Direct2D visual assets (lazy loaded)
    ID2D1Bitmap* pBitmap = nullptr;
    HICON hIcon = nullptr;

    ~AppItem() {
        if (pBitmap) {
            pBitmap->Release();
            pBitmap = nullptr;
        }
        if (hIcon) {
            DestroyIcon(hIcon);
            hIcon = nullptr;
        }
    }

    void EnsureIcon(IWICImagingFactory* pWIC, ID2D1RenderTarget* pRT) {
        if (pBitmap != nullptr) return;

        std::wstring pathToExtract = !iconPath.empty() ? iconPath : targetPath;

        // Special handling for File Explorer: extract native folder icon from explorer.exe index 0
        if (!hIcon && (name == L"资源管理器" || pathToExtract.find(L"explorer.exe") != std::wstring::npos)) {
            UINT iconId = 0;
            PrivateExtractIconsW(L"C:\\Windows\\explorer.exe", 0, 96, 96, &hIcon, &iconId, 1, 0);
        }

        // 1. Primary method: PrivateExtractIconsW
        // Accurately extracts and fits the icon to 96x96 directly from executable resources.
        // This solves the bug where SHIL_JUMBO puts 32px/48px legacy icons into a 256x256 canvas with giant blank padding,
        // which caused Bandicam, Mem Reduct, 4DDiG etc. to appear as tiny 10px dots.
        if (!pathToExtract.empty()) {
            UINT iconId = 0;
            UINT extracted = PrivateExtractIconsW(pathToExtract.c_str(), 0, 96, 96, &hIcon, &iconId, 1, 0);
            if (extracted == 0 || !hIcon) {
                hIcon = nullptr;
            }
        }

        // 2. Fallback to SHGetImageList (using SHIL_EXTRALARGE 48x48 to avoid giant blank padding)
        if (!hIcon && !pathToExtract.empty()) {
            SHFILEINFOW sfi = {};
            DWORD_PTR res = SHGetFileInfoW(pathToExtract.c_str(), 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX);
            if (res != 0) {
                typedef HRESULT(WINAPI* PFN_SHGetImageList)(int iImageList, REFIID riid, void** ppvObj);
                static PFN_SHGetImageList pfnSHGetImageList = (PFN_SHGetImageList)GetProcAddress(GetModuleHandleW(L"shell32.dll"), "SHGetImageList");
                if (pfnSHGetImageList) {
                    IImageList* piml = nullptr;
                    const IID IID_IImageList_val = { 0x46EB5926, 0x582E, 0x4017, { 0x9F, 0xDF, 0xE8, 0x99, 0x8D, 0xAA, 0x09, 0x50 } };
                    if (SUCCEEDED(pfnSHGetImageList(2 /* SHIL_EXTRALARGE */, IID_IImageList_val, (void**)&piml)) && piml) {
                        piml->GetIcon(sfi.iIcon, ILD_TRANSPARENT, &hIcon);
                        piml->Release();
                        piml = nullptr;
                    }
                }
            }
        }

        // 3. Fallback to standard SHGetFileInfo
        if (!hIcon && !pathToExtract.empty()) {
            SHFILEINFOW sfi = {};
            SHGetFileInfoW(pathToExtract.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON);
            hIcon = sfi.hIcon;
        }

        if (!hIcon) {
            hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        }

        if (hIcon && pWIC && pRT) {
            IWICBitmap* pWicBitmap = nullptr;
            HRESULT hr = pWIC->CreateBitmapFromHICON(hIcon, &pWicBitmap);
            if (SUCCEEDED(hr) && pWicBitmap) {
                UINT origW = 0, origH = 0;
                pWicBitmap->GetSize(&origW, &origH);

                IWICFormatConverter* pConverter = nullptr;
                hr = pWIC->CreateFormatConverter(&pConverter);
                if (SUCCEEDED(hr) && pConverter) {
                    hr = pConverter->Initialize(
                        pWicBitmap,
                        GUID_WICPixelFormat32bppPBGRA,
                        WICBitmapDitherTypeNone,
                        nullptr,
                        0.0f,
                        WICBitmapPaletteTypeCustom
                    );
                    if (SUCCEEDED(hr)) {
                        // Standardize to 96x96 for low memory & sharp rendering
                        if (origW != 96 || origH != 96) {
                            IWICBitmapScaler* pScaler = nullptr;
                            hr = pWIC->CreateBitmapScaler(&pScaler);
                            if (SUCCEEDED(hr) && pScaler) {
                                hr = pScaler->Initialize(pConverter, 96, 96, WICBitmapInterpolationModeFant);
                                if (SUCCEEDED(hr)) {
                                    pRT->CreateBitmapFromWicBitmap(pScaler, nullptr, &pBitmap);
                                }
                                pScaler->Release();
                            }
                        }
                        if (!pBitmap) {
                            pRT->CreateBitmapFromWicBitmap(pConverter, nullptr, &pBitmap);
                        }
                    }
                    pConverter->Release();
                }
                pWicBitmap->Release();
            }
            // Free GDI/USER icon handle immediately
            DestroyIcon(hIcon);
            hIcon = nullptr;
        }
    }
};

} // namespace Drawer
