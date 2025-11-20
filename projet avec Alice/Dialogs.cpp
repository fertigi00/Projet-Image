#include "Dialogs.h"
#include <string>

static DLGTEMPLATE* CreateDialogTemplate(int w, int h)
{
    DLGTEMPLATE* dlg = (DLGTEMPLATE*)LocalAlloc(LPTR, 4096);

    if (!dlg)
        return NULL;

    ZeroMemory(dlg, 4096);

    dlg->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_SETFONT;
    dlg->cx = (short)w;
    dlg->cy = (short)h;

    return dlg;
}


static INT_PTR CALLBACK AskTextProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static std::string* pOut = nullptr;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        pOut = reinterpret_cast<std::string*>(lParam);

        // Créer contrôles
        CreateWindowW(L"STATIC", L"Type your message:", WS_CHILD | WS_VISIBLE,
            15, 10, 300, 20, hDlg, NULL, NULL, NULL);

        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER |
            ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
            15, 35, 330, 120, hDlg, (HMENU)1001, NULL, NULL);

        CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE,
            70, 165, 100, 30, hDlg, (HMENU)IDOK, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE,
            190, 165, 100, 30, hDlg, (HMENU)IDCANCEL, NULL, NULL);

        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            wchar_t wbuf[2048] = {};
            GetDlgItemTextW(hDlg, 1001, wbuf, 2048);

            int len = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, NULL, 0, NULL, NULL);

            if (len > 1)
            {
                std::string utf8(len - 1, '\0');
                WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, &utf8[0], len, NULL, NULL);

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
    DLGTEMPLATE* dlg = CreateDialogTemplate(260, 200);
    if (!dlg) {
        MessageBox(parent, L"Erreur : impossible d'allouer le dialogue.", L"Erreur mémoire", MB_OK | MB_ICONERROR);
        return false;
    }

    INT_PTR result = DialogBoxIndirectParamW(
        GetModuleHandle(NULL),
        dlg,
        parent,
        AskTextProc,
        (LPARAM)&outText
    );

    LocalFree(dlg);
    return (result == 1);
}

static INT_PTR CALLBACK ShowMsgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        const char* text = reinterpret_cast<const char*>(lParam);

        int wlen = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
        if (wlen > 1)
        {
            std::wstring wstr(wlen, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, text, -1, &wstr[0], wlen);

            CreateWindowW(L"EDIT", wstr.c_str(),
                WS_CHILD | WS_VISIBLE | WS_BORDER |
                ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
                15, 15, 330, 130,
                hDlg, (HMENU)1002, NULL, NULL);
        }

        CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE,
            140, 155, 70, 28,
            hDlg, (HMENU)IDOK, NULL, NULL);

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
    DLGTEMPLATE* dlg = CreateDialogTemplate(260, 190);
    if (!dlg) {
        MessageBox(parent, L"Erreur : impossible d’afficher le message.", L"Erreur mémoire", MB_OK | MB_ICONERROR);
        return;
    }

    DialogBoxIndirectParamW(
        GetModuleHandle(NULL),
        dlg,
        parent,
        ShowMsgProc,
        (LPARAM)text.c_str()
    );

    LocalFree(dlg);
}
