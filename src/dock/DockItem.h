#pragma once
#include <string>
#include <windows.h>
#include <d2d1.h>
#include <wincodec.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <shlobj.h>

namespace Dock {

enum class ItemAction {
    LaunchApp,
    ToggleLaunchpad,
    OpenSettings,
    RecycleBin,
    Power
};

struct DockItem {
    std::wstring name;
    std::wstring targetPath;
    ItemAction action = ItemAction::LaunchApp;

    // Geometry & Animation State
    float baseSize = 48.0f;
    float currentSize = 48.0f;
    float targetSize = 48.0f;
    float centerX = 0.0f;
    float centerY = 0.0f;

    // Running state
    bool isRunning = false;
    bool isHovered = false;

    // Visual asset
    ID2D1Bitmap* pBitmap = nullptr;
    HICON hIcon = nullptr;
    bool isRecycleBinFull = false;

    void ReloadIcon(IWICImagingFactory* pWICFactory, ID2D1RenderTarget* pRT) {
        if (pBitmap) {
            pBitmap->Release();
            pBitmap = nullptr;
        }
        if (hIcon) {
            DestroyIcon(hIcon);
            hIcon = nullptr;
        }
        LoadIconFromPath(pWICFactory, pRT);
    }

    ~DockItem() {
        if (pBitmap) {
            pBitmap->Release();
            pBitmap = nullptr;
        }
        if (hIcon) {
            DestroyIcon(hIcon);
            hIcon = nullptr;
        }
    }

    static std::wstring ResolvePath(const std::wstring& path) {
        if (path.empty()) return L"";
        std::wstring actual = path;
        wchar_t found[MAX_PATH] = {};
        if (SearchPathW(nullptr, path.c_str(), nullptr, MAX_PATH, found, nullptr) > 0) {
            actual = found;
        }

        // If target is a .lnk shortcut, resolve to clean .exe target to avoid shortcut arrow badge
        if (actual.length() >= 4 && (actual.substr(actual.length() - 4) == L".lnk" || actual.substr(actual.length() - 4) == L".LNK")) {
            IShellLinkW* psl = nullptr;
            if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)) && psl) {
                IPersistFile* ppf = nullptr;
                if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) && ppf) {
                    if (SUCCEEDED(ppf->Load(actual.c_str(), STGM_READ))) {
                        wchar_t szPath[MAX_PATH] = {};
                        WIN32_FIND_DATAW wfd = {};
                        if (SUCCEEDED(psl->GetPath(szPath, MAX_PATH, &wfd, SLGP_UNCPRIORITY)) && wcslen(szPath) > 0) {
                            ppf->Release();
                            psl->Release();
                            return szPath;
                        }
                    }
                    ppf->Release();
                }
                psl->Release();
            }
        }
        return actual;
    }

    void LoadIconFromPath(IWICImagingFactory* pWICFactory, ID2D1RenderTarget* pRT) {
        if (pBitmap != nullptr) return;

        // Special handling for Windows Recycle Bin
        if (action == ItemAction::RecycleBin || targetPath == L"shell:RecycleBinFolder" || name == L"回收站") {
            SHQUERYRBINFO qrb = { sizeof(SHQUERYRBINFO) };
            isRecycleBinFull = (SHQueryRecycleBinW(nullptr, &qrb) == S_OK && qrb.i64NumItems > 0);
            UINT iconId = 0;
            // Windows 10/11 imageres.dll: -54 (empty), -55 (full)
            PrivateExtractIconsW(L"imageres.dll", isRecycleBinFull ? -55 : -54, 96, 96, &hIcon, &iconId, 1, 0);
            if (!hIcon) {
                // Windows shell32.dll: -32 (empty), -33 (full)
                PrivateExtractIconsW(L"shell32.dll", isRecycleBinFull ? -33 : -32, 96, 96, &hIcon, &iconId, 1, 0);
            }
            if (!hIcon) {
                SHSTOCKICONINFO sii = { sizeof(SHSTOCKICONINFO) };
                if (SUCCEEDED(SHGetStockIconInfo(isRecycleBinFull ? SIID_RECYCLERFULL : SIID_RECYCLER, SHGSI_ICON | SHGSI_LARGEICON, &sii))) {
                    hIcon = sii.hIcon;
                }
            }
        }

        std::wstring resolvedPath = ResolvePath(targetPath);

        // Special handling for Windows File Explorer: directly extract native folder icon from explorer.exe index 0
        if (!hIcon && (name == L"资源管理器" || resolvedPath.find(L"explorer.exe") != std::wstring::npos)) {
            UINT iconId = 0;
            PrivateExtractIconsW(L"C:\\Windows\\explorer.exe", 0, 96, 96, &hIcon, &iconId, 1, 0);
            if (!hIcon) {
                PrivateExtractIconsW(L"explorer.exe", 0, 96, 96, &hIcon, &iconId, 1, 0);
            }
        }

        // 1. Primary method: PrivateExtractIconsW
        // Directly extracts and scales icon to 96x96, avoiding SHIL_JUMBO blank margin padding
        if (!hIcon && !resolvedPath.empty()) {
            UINT iconId = 0;
            UINT extracted = PrivateExtractIconsW(resolvedPath.c_str(), 0, 96, 96, &hIcon, &iconId, 1, 0);
            if (extracted == 0 || !hIcon) {
                hIcon = nullptr;
            }
        }

        // 2. Fallback to SHGetImageList (using SHIL_EXTRALARGE 48x48)
        if (!hIcon && !resolvedPath.empty()) {
            SHFILEINFOW sfi = {};
            DWORD_PTR res = SHGetFileInfoW(resolvedPath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX);
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

            if (!hIcon) {
                SHGetFileInfoW(resolvedPath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON);
                hIcon = sfi.hIcon;
            }
        }

        // Fallback icon if none found
        if (!hIcon) {
            hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        }

        if (hIcon && pWICFactory && pRT) {
            IWICBitmap* pWicBitmap = nullptr;
            HRESULT hr = pWICFactory->CreateBitmapFromHICON(hIcon, &pWicBitmap);
            if (SUCCEEDED(hr) && pWicBitmap) {
                UINT origW = 0, origH = 0;
                pWicBitmap->GetSize(&origW, &origH);

                IWICFormatConverter* pConverter = nullptr;
                hr = pWICFactory->CreateFormatConverter(&pConverter);
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
                        // Keep memory footprint minimal: downscale to 96x96 (~36KB per icon)
                        if (origW != 96 || origH != 96) {
                            IWICBitmapScaler* pScaler = nullptr;
                            hr = pWICFactory->CreateBitmapScaler(&pScaler);
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

} // namespace Dock
