#pragma once

#include <windows.h>

// Menu
enum MenuIds
{
    ID_FILE_OPEN = 1001,
    ID_FILE_SAVE = 1002,
    ID_FILE_EXIT = 1003,

    ID_STEG_EMBED = 1101,
    ID_STEG_EXTRACT = 1102,
    ID_STEG_COMPARE = 1103,

    ID_VIEW_ZOOM_IN = 1201,
    ID_VIEW_ZOOM_OUT = 1202,
    ID_VIEW_ZOOM_RESET = 1203
};

// Contrôles internes pour le panneau de message
enum ControlIds
{
    ID_MSG_PANEL = 2001,
    ID_MSG_EDIT = 2002,
    ID_MSG_OK = 2003,
    ID_MSG_CANCEL = 2004
};

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
