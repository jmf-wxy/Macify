#include "InputBox.h"
#include <dwmapi.h>
#include <commctrl.h>

namespace Core {

struct InputBoxContext {
    HWND hEdit = nullptr;
    HWND hBtnOk = nullptr;
    HWND hBtnCancel = nullptr;
    HFONT hFont = nullptr;
    HBRUSH hBgBrush = nullptr;
    HBRUSH hEditBrush = nullptr;
    std::wstring resultText;
    bool confirmed = false;
    WNDPROC oldEditProc = nullptr;
};

static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* ctx = reinterpret_cast<InputBoxContext*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (ctx) {
        if (msg == WM_KEYDOWN) {
            if (wParam == VK_RETURN) {
                SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), 0);
                return 0;
            } else if (wParam == VK_ESCAPE) {
                SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), 0);
                return 0;
            }
        }
        return CallWindowProcW(ctx->oldEditProc, hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK InputBoxWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* ctx = reinterpret_cast<InputBoxContext*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            ctx = reinterpret_cast<InputBoxContext*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
            return 0;
        }

        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetBkColor(hdc, RGB(28, 30, 36));
            SetTextColor(hdc, RGB(240, 242, 248));
            return reinterpret_cast<INT_PTR>(ctx ? ctx->hBgBrush : GetStockObject(BLACK_BRUSH));
        }

        case WM_CTLCOLOREDIT: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetBkColor(hdc, RGB(40, 44, 52));
            SetTextColor(hdc, RGB(255, 255, 255));
            return reinterpret_cast<INT_PTR>(ctx ? ctx->hEditBrush : GetStockObject(BLACK_BRUSH));
        }

        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            if (id == IDOK && ctx) {
                wchar_t buf[512] = {};
                GetWindowTextW(ctx->hEdit, buf, 511);
                ctx->resultText = buf;
                ctx->confirmed = true;
                DestroyWindow(hwnd);
                return 0;
            } else if (id == IDCANCEL && ctx) {
                ctx->confirmed = false;
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }

        case WM_CLOSE:
            if (ctx) ctx->confirmed = false;
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool ShowInputBox(HWND parent, const wchar_t* title, const wchar_t* prompt, std::wstring& textOut, const wchar_t* defaultText) {
    const wchar_t CLASS_NAME[] = L"MacifyInputBoxDialog";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = InputBoxWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(28, 30, 36));
    RegisterClassExW(&wc);

    InputBoxContext ctx;
    ctx.hBgBrush = wc.hbrBackground;
    ctx.hEditBrush = CreateSolidBrush(RGB(40, 44, 52));

    int dlgW = 400;
    int dlgH = 175;

    RECT rcParent = {};
    if (parent && IsWindow(parent)) {
        GetWindowRect(parent, &rcParent);
    } else {
        rcParent.left = 0; rcParent.top = 0;
        rcParent.right = GetSystemMetrics(SM_CXSCREEN);
        rcParent.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    int posX = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int posY = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_DLGMODALFRAME,
        CLASS_NAME,
        title ? title : L"输入",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        posX, posY, dlgW, dlgH,
        parent, nullptr, GetModuleHandle(nullptr), &ctx
    );

    if (!hDlg) {
        if (ctx.hEditBrush) DeleteObject(ctx.hEditBrush);
        if (ctx.hBgBrush) DeleteObject(ctx.hBgBrush);
        return false;
    }

    // Windows 11 / 10 Dark Mode titlebar
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hDlg, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &darkMode, sizeof(darkMode));

    // Font
    ctx.hFont = CreateFontW(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );

    // Prompt label
    HWND hStatic = CreateWindowExW(
        0, L"STATIC", prompt ? prompt : L"请输入：",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 16, dlgW - 40, 22,
        hDlg, nullptr, GetModuleHandle(nullptr), nullptr
    );
    SendMessageW(hStatic, WM_SETFONT, reinterpret_cast<WPARAM>(ctx.hFont), TRUE);

    // Edit control
    ctx.hEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", defaultText ? defaultText : L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        20, 44, dlgW - 40, 28,
        hDlg, reinterpret_cast<HMENU>(1001), GetModuleHandle(nullptr), nullptr
    );
    SendMessageW(ctx.hEdit, WM_SETFONT, reinterpret_cast<WPARAM>(ctx.hFont), TRUE);

    // Subclass Edit control
    SetWindowLongPtrW(ctx.hEdit, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&ctx));
    ctx.oldEditProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(ctx.hEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditSubclassProc)));

    // Buttons
    ctx.hBtnOk = CreateWindowExW(
        0, L"BUTTON", L"确定",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        dlgW - 190, 88, 80, 30,
        hDlg, reinterpret_cast<HMENU>(IDOK), GetModuleHandle(nullptr), nullptr
    );
    SendMessageW(ctx.hBtnOk, WM_SETFONT, reinterpret_cast<WPARAM>(ctx.hFont), TRUE);

    ctx.hBtnCancel = CreateWindowExW(
        0, L"BUTTON", L"取消",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        dlgW - 100, 88, 80, 30,
        hDlg, reinterpret_cast<HMENU>(IDCANCEL), GetModuleHandle(nullptr), nullptr
    );
    SendMessageW(ctx.hBtnCancel, WM_SETFONT, reinterpret_cast<WPARAM>(ctx.hFont), TRUE);

    // Focus edit and select all
    SetFocus(ctx.hEdit);
    SendMessageW(ctx.hEdit, EM_SETSEL, 0, -1);

    // Disable parent for modality
    if (parent) EnableWindow(parent, FALSE);

    // Modal message loop
    MSG msg = {};
    while (IsWindow(hDlg) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (parent) {
        EnableWindow(parent, TRUE);
        SetForegroundWindow(parent);
    }

    if (ctx.hFont) DeleteObject(ctx.hFont);
    if (ctx.hEditBrush) DeleteObject(ctx.hEditBrush);
    if (ctx.hBgBrush) DeleteObject(ctx.hBgBrush);

    if (ctx.confirmed) {
        textOut = ctx.resultText;
        return true;
    }
    return false;
}

} // namespace Core
