#include <windows.h>
#include "MainWindow.h"
#include "ImageManager.h"//le message cacher dans les pixel an rgb en bineair 

// Taille de la fenêtre
#define WIN_WIDTH 800
#define WIN_HEIGHT 600

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Initialisation GDI+
    if (!InitGDIPlus())
    {
        MessageBox(nullptr, L"Failed to initialize GDI+", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Classe de fenêtre
    WNDCLASS wc = {};
    wc.lpfnWndProc = MainWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"StegWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc))
    {
        MessageBox(nullptr, L"Failed to register window class", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Création de la fenêtre principale
    HWND hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        L"Steganography App",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WIN_WIDTH, WIN_HEIGHT,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd)
    {
        MessageBox(nullptr, L"Failed to create window", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Menu
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    AppendMenu(hFileMenu, MF_STRING, 1, L"Open image");
    AppendMenu(hFileMenu, MF_STRING, 2, L"Save image");
    AppendMenu(hFileMenu, MF_STRING, 99, L"Quit");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");

    HMENU hStegMenu = CreateMenu();
    AppendMenu(hStegMenu, MF_STRING, 10, L"Embed message");
    AppendMenu(hStegMenu, MF_STRING, 11, L"Extract message");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hStegMenu, L"Steg");

    SetMenu(hwnd, hMenu);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Boucle de messages
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Libération GDI+
    ShutdownGDIPlus();

    return (int)msg.wParam;
}
