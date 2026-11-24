#include "AppScanner.h"
#include <initguid.h>
#include <windows.h>
#include <shlobj.h>
#include <algorithm>
#include <unordered_set>

namespace Drawer {

std::wstring AppScanner::ToLower(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

bool AppScanner::IsSystemOrIgnore(const std::wstring& name) {
    std::wstring lower = ToLower(name);
    if (lower == L"desktop.ini") return true;
    if (lower == L"回收站" || lower == L"recycle bin") return true;
    if (lower == L"此电脑" || lower == L"my computer") return true;
    return false;
}

bool AppScanner::IsDocumentOrArchive(const std::wstring& path) {
    std::wstring lower = ToLower(path);
    const wchar_t* badExts[] = {
        L".xlsx", L".xls", L".docx", L".doc", L".pptx", L".ppt",
        L".pdf", L".txt", L".csv", L".zip", L".rar", L".7z",
        L".ini", L".log", L".md", L".json", L".xml",
        L".png", L".jpg", L".jpeg", L".gif", L".webp", L".bmp",
        L".mp4", L".mkv", L".avi", L".mp3", L".flac", L".wav"
    };

    for (const auto* ext : badExts) {
        size_t extLen = wcslen(ext);
        if (lower.length() >= extLen && lower.rfind(ext) == lower.length() - extLen) {
            return true;
        }
    }
    return false;
}

std::wstring AppScanner::ResolveShortcut(const std::wstring& lnkPath) {
    IShellLinkW* psl = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl);
    if (FAILED(hr) || !psl) return L"";

    IPersistFile* ppf = nullptr;
    hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
    if (FAILED(hr) || !ppf) {
        psl->Release();
        return L"";
    }

    hr = ppf->Load(lnkPath.c_str(), STGM_READ);
    std::wstring target;
    if (SUCCEEDED(hr)) {
        wchar_t szPath[MAX_PATH] = {};
        WIN32_FIND_DATAW wfd = {};
        hr = psl->GetPath(szPath, MAX_PATH, &wfd, SLGP_UNCPRIORITY);
        if (SUCCEEDED(hr) && wcslen(szPath) > 0) {
            target = szPath;
        }
    }

    ppf->Release();
    psl->Release();
    return target;
}

void AppScanner::ScanDesktopDirectory(const std::wstring& directory, std::vector<std::unique_ptr<AppItem>>& outList) {
    std::wstring searchPattern = directory + L"\\*";
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &ffd);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) {
            continue;
        }

        // 1. Strictly ignore folders / directories
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        std::wstring fileName = ffd.cFileName;
        if (IsSystemOrIgnore(fileName)) continue;

        // 2. Filter out documents, spreadsheets (e.g. .xlsx), archives, images
        if (IsDocumentOrArchive(fileName)) continue;

        std::wstring fullPath = directory + L"\\" + fileName;
        std::wstring lowerFileName = ToLower(fileName);

        // 3. Process .lnk shortcuts
        if (lowerFileName.length() > 4 && lowerFileName.substr(lowerFileName.length() - 4) == L".lnk") {
            std::wstring appName = fileName.substr(0, fileName.length() - 4);
            if (IsSystemOrIgnore(appName)) continue;

            // Strip redundant trailing .exe from shortcut name (e.g. BandicamPortable.exe.lnk -> BandicamPortable)
            std::wstring lowerAppName = ToLower(appName);
            if (lowerAppName.length() > 4 && lowerAppName.substr(lowerAppName.length() - 4) == L".exe") {
                appName = appName.substr(0, appName.length() - 4);
            }

            // Check resolved target
            std::wstring target = ResolveShortcut(fullPath);
            if (!target.empty()) {
                // If shortcut targets a folder, skip it
                DWORD attr = GetFileAttributesW(target.c_str());
                if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
                    continue;
                }
                // If shortcut targets a document or archive, skip it
                if (IsDocumentOrArchive(target)) {
                    continue;
                }
                // If shortcut targets an uninstaller, skip it
                std::wstring lowerTarget = ToLower(target);
                if (lowerTarget.find(L"unins000.exe") != std::wstring::npos ||
                    lowerTarget.find(L"uninstall.exe") != std::wstring::npos) {
                    continue;
                }
            }

            auto item = std::make_unique<AppItem>();
            item->name = appName;
            item->targetPath = fullPath; // Launching .lnk keeps full shell arguments & working dir
            item->iconPath = !target.empty() ? target : fullPath; // Clean .exe icon without shortcut overlay
            item->searchKey = ToLower(appName);
            outList.push_back(std::move(item));
        }
        // 4. Process direct executables on desktop (.exe, .bat, .cmd)
        else if (lowerFileName.length() > 4 &&
                 (lowerFileName.substr(lowerFileName.length() - 4) == L".exe" ||
                  lowerFileName.substr(lowerFileName.length() - 4) == L".bat" ||
                  lowerFileName.substr(lowerFileName.length() - 4) == L".cmd")) {
            std::wstring appName = fileName.substr(0, fileName.length() - 4);
            if (IsSystemOrIgnore(appName)) continue;

            auto item = std::make_unique<AppItem>();
            item->name = appName;
            item->targetPath = fullPath;
            item->iconPath = fullPath;
            item->searchKey = ToLower(appName);
            outList.push_back(std::move(item));
        }
    } while (FindNextFileW(hFind, &ffd) != 0);

    FindClose(hFind);
}

std::vector<std::unique_ptr<AppItem>> AppScanner::ScanDesktopApps() {
    std::vector<std::unique_ptr<AppItem>> allApps;

    wchar_t path[MAX_PATH];

    // 1. Current User Desktop
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, path))) {
        ScanDesktopDirectory(path, allApps);
    }

    // 2. Common / Public Desktop
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_COMMON_DESKTOPDIRECTORY, nullptr, 0, path))) {
        ScanDesktopDirectory(path, allApps);
    }

    // 3. Deduplicate by name & sort alphabetically
    std::unordered_set<std::wstring> seenNames;
    std::vector<std::unique_ptr<AppItem>> uniqueApps;

    for (auto& item : allApps) {
        if (seenNames.find(item->searchKey) == seenNames.end()) {
            seenNames.insert(item->searchKey);
            uniqueApps.push_back(std::move(item));
        }
    }

    std::sort(uniqueApps.begin(), uniqueApps.end(), [](const std::unique_ptr<AppItem>& a, const std::unique_ptr<AppItem>& b) {
        return a->name < b->name;
    });

    return uniqueApps;
}

} // namespace Drawer
