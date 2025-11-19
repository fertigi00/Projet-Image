#include <windows.h>
#include "MainWindow.h"
#include "ImageManager.h"

#define WIN_WIDTH  900
#define WIN_HEIGHT 700

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;

    if (!InitGDIPlus())
    {
        MessageBox(nullptr, L"Échec de l'initialisation de GDI+.", L"Erreur", MB_OK | MB_ICONERROR);
        return -1;
    }

    WNDCLASS wc{};
    wc.lpfnWndProc = MainWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"StegWindowClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc))
    {
        MessageBox(nullptr, L"Échec de l'enregistrement de la classe de fenêtre.", L"Erreur", MB_OK | MB_ICONERROR);
        ShutdownGDIPlus();
        return -1;
    }

    HWND hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        L"Projet Image",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        WIN_WIDTH,
        WIN_HEIGHT,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd)
    {
        MessageBox(nullptr, L"Échec de la création de la fenêtre.", L"Erreur", MB_OK | MB_ICONERROR);
        ShutdownGDIPlus();
        return -1;
    }

    // Menu
    HMENU hMenuBar = CreateMenu();

    HMENU hFileMenu = CreateMenu();
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_OPEN, L"&Ouvrir une image...");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_SAVE, L"&Sauvegarder l'image...");
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_EXIT, L"&Quitter");

    HMENU hStegMenu = CreateMenu();
    AppendMenu(hStegMenu, MF_STRING, ID_STEG_EMBED, L"&Cacher un message...");
    AppendMenu(hStegMenu, MF_STRING, ID_STEG_EXTRACT, L"&Extraire le message");
    AppendMenu(hStegMenu, MF_STRING, ID_STEG_COMPARE, L"&Comparer deux images...");

    HMENU hViewMenu = CreateMenu();
    AppendMenu(hViewMenu, MF_STRING, ID_VIEW_ZOOM_IN, L"Zoom &avant");
    AppendMenu(hViewMenu, MF_STRING, ID_VIEW_ZOOM_OUT, L"Zoom a&rrière");
    AppendMenu(hViewMenu, MF_STRING, ID_VIEW_ZOOM_RESET, L"&Réinitialiser le zoom");

    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"&Fichier");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hStegMenu, L"&Stéganographie");
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hViewMenu, L"&Affichage");

    SetMenu(hwnd, hMenuBar);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ShutdownGDIPlus();
    return (int)msg.wParam;
}
