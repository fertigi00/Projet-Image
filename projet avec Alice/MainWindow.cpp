#include "MainWindow.h"
#include "RendererGDI.h"
#include "ImageManager.h"
#include "StegEngine.h"  // pour EmbedLSB / ExtractLSB
#include <windows.h>
#include <string>

// Image globale
LoadedImage gImage = {}; // pixels vide, width/height = 0
std::string gExtractedMessage; // message extrait à afficher sur l'image

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case 1: // Open image
        {
            OPENFILENAME ofn = {};
            wchar_t szFile[MAX_PATH] = {};

            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"BMP Files\0*.bmp\0All Files\0*.*\0";
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

            if (GetOpenFileName(&ofn))
            {
                if (LoadBMPGDIPlus(ofn.lpstrFile, gImage))
                    InvalidateRect(hwnd, nullptr, TRUE);
                else
                    MessageBox(hwnd, L"Failed to load image.", L"Error", MB_OK | MB_ICONERROR);
            }
        }
        break;

        case 2: // Save image
        {
            OPENFILENAME ofn = {};
            wchar_t szFile[MAX_PATH] = {};

            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"BMP Files\0*.bmp\0All Files\0*.*\0";
            ofn.Flags = OFN_OVERWRITEPROMPT;

            if (GetSaveFileName(&ofn))
            {
                // L'image contient déjà le message via EmbedLSB
                if (!SaveBMPGDIPlus(ofn.lpstrFile, gImage))
                    MessageBox(hwnd, L"Failed to save image.", L"Error", MB_OK | MB_ICONERROR);
                else
                    MessageBox(hwnd, L"Image sauvegardée avec le message caché !", L"Info", MB_OK);
            }
        }
        break;

        case 10: // Embed message
        {
            // Création d'un buffer
            wchar_t buffer[1024] = {};

            // Boîte de dialogue Edit simple
            HWND hEdit = CreateWindowEx(
                0, L"EDIT", L"",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                100, 100, 400, 100, hwnd, nullptr, nullptr, nullptr
            );

            if (hEdit)
            {
                MessageBox(hwnd, L"Tapez votre message dans la petite fenêtre puis cliquez OK.", L"Info", MB_OK);

                // Récupérer le texte
                GetWindowText(hEdit, buffer, 1024);
                DestroyWindow(hEdit);

                // Conversion wstring -> string
                std::wstring ws(buffer);
                std::string msg(ws.begin(), ws.end());

                // Embed le message
                if (Steg::EmbedLSB(gImage, msg))
                {
                    InvalidateRect(hwnd, nullptr, TRUE); // repaint
                    MessageBox(hwnd, L"Message intégré dans l'image !", L"Info", MB_OK);
                }
                else
                {
                    MessageBox(hwnd, L"Message trop long pour cette image!", L"Erreur", MB_OK | MB_ICONERROR);
                }
            }
        }
        break;



        case 11: // Extract message
        {
            gExtractedMessage.clear();
            if (Steg::ExtractLSB(gImage, gExtractedMessage))
            {
                // Afficher le message dans une boîte de dialogue
                std::wstring wmsg(gExtractedMessage.begin(), gExtractedMessage.end());
                MessageBox(hwnd, wmsg.c_str(), L"Message caché", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                MessageBox(hwnd, L"Aucun message caché trouvé.", L"Info", MB_OK);
            }
        }
        break;

        case 99: // Quit
            PostQuitMessage(0);
            break;
        }
    }
    break;

    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        if (!gImage.pixels.empty())
        {
            int imgW = gImage.width;
            int imgH = gImage.height;
            int winW = rc.right - rc.left;
            int winH = rc.bottom - rc.top;

            float scaleX = (float)winW / imgW;
            float scaleY = (float)winH / imgH;
            float scale = (scaleX < scaleY) ? scaleX : scaleY;

            int drawW = (int)(imgW * scale);
            int drawH = (int)(imgH * scale);

            RECT drawRect;
            drawRect.left = (winW - drawW) / 2;
            drawRect.top = (winH - drawH) / 2;
            drawRect.right = drawRect.left + drawW;
            drawRect.bottom = drawRect.top + drawH;

            RenderImageGDI(hdc, drawRect, &gImage);

            // Affichage du message extrait sur l'image
            if (!gExtractedMessage.empty())
            {
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(255, 0, 0)); // texte rouge
                DrawTextA(hdc, gExtractedMessage.c_str(), -1, &drawRect, DT_CENTER | DT_TOP | DT_WORDBREAK);
            }
        }

        EndPaint(hwnd, &ps);
    }
    break;

    case WM_DESTROY:
        gImage.pixels.clear();
        gImage.width = 0;
        gImage.height = 0;
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
