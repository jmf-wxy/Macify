#pragma once
#include <windows.h>
#include <string>

namespace Core {

bool ShowInputBox(HWND parent, const wchar_t* title, const wchar_t* prompt, std::wstring& textOut, const wchar_t* defaultText = L"");

} // namespace Core
