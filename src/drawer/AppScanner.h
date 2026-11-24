#pragma once
#include <vector>
#include <memory>
#include "AppItem.h"

namespace Drawer {

class AppScanner {
public:
    static std::vector<std::unique_ptr<AppItem>> ScanDesktopApps();

private:
    static void ScanDesktopDirectory(const std::wstring& directory, std::vector<std::unique_ptr<AppItem>>& outList);
    static std::wstring ResolveShortcut(const std::wstring& lnkPath);
    static bool IsDocumentOrArchive(const std::wstring& path);
    static bool IsSystemOrIgnore(const std::wstring& name);
    static std::wstring ToLower(const std::wstring& str);
};

} // namespace Drawer
