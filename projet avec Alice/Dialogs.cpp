#include <windows.h>
#include <string>

// ===============================================================
// Dialog: ask text to embed
// ===============================================================
static INT_PTR CALLBACK AskTextProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static std::string* pOut;

    switch (msg)
    {
    case WM_INITDIALOG:
        pOut = (std::string*)lParam;
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            wchar_t wbuf[2048];
            GetDlgItemTextW(hDlg, 1001, wbuf, 2048);

            // convert to UTF-8
            int len = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, nullptr, 0, nullptr, nullptr);
            if (len > 0)
            {
                std::string utf8(len - 1, '\0'); // -1 car WideCharToMultiByte inclut le '\0'
                WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, &utf8[0], len, nullptr, nullptr);
                *pOut = utf8;
            }

            EndDialog(hDlg, 1);
            return TRUE;
        }

        case IDCANCEL:
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

bool Dialog_AskText(HWND parent, std::string& outText)
{
    HWND dlg = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        L"#32770",
        L"Enter message",
        WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU,
        0, 0, 400, 250,
        parent, NULL, NULL, NULL
    );

    // Label
    CreateWindowW(L"STATIC", L"Type your message:", WS_CHILD | WS_VISIBLE, 20, 20, 300, 20, dlg, NULL, NULL, NULL);

    // Edit control (multiline)
    CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
        20, 50, 350, 120, dlg, (HMENU)1001, NULL, NULL);

    // OK button
    CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE, 80, 180, 100, 30, dlg, (HMENU)IDOK, NULL, NULL);

    // Cancel button
    CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE, 200, 180, 100, 30, dlg, (HMENU)IDCANCEL, NULL, NULL);

    // Center the window
    RECT rc;
    GetWindowRect(dlg, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    SetWindowPos(dlg, NULL, x, y, 0, 0, SWP_NOSIZE);

    // Modal dialog
    INT_PTR result = DialogBoxParam(NULL, NULL, dlg, AskTextProc, (LPARAM)&outText);
    return (result == 1);
}

// ===============================================================
// Dialog: show extracted message
// ===============================================================
static INT_PTR CALLBACK ShowMsgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        const char* text = (const char*)lParam;
        if (text)
        {
            int lenW = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
            if (lenW > 0)
            {
                wchar_t* buf = new wchar_t[lenW];
                MultiByteToWideChar(CP_UTF8, 0, text, -1, buf, lenW);
                SetDlgItemTextW(hDlg, 1002, buf);
                delete[] buf;
            }
        }
        return TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

void Dialog_ShowMessage(HWND parent, const std::string& text)
{
    HWND dlg = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        L"#32770",
        L"Hidden message",
        WS_POPUP | WS_VISIBLE | WS_CAPTION,
        0, 0, 400, 250,
        parent, NULL, NULL, NULL
    );

    // Read-only edit
    CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
        20, 20, 350, 150, dlg, (HMENU)1002, NULL, NULL);

    // OK button
    CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE, 150, 180, 80, 30, dlg, (HMENU)IDOK, NULL, NULL);

    // Center window
    RECT rc;
    GetWindowRect(dlg, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    SetWindowPos(dlg, NULL, x, y, 0, 0, SWP_NOSIZE);

    DialogBoxParam(NULL, NULL, dlg, ShowMsgProc, (LPARAM)text.c_str());
}
