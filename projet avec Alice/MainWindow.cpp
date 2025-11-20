#include "MainWindow.h"
#include "ImageManager.h"
#include "RendererGDIPlus.h"
#include "StegEngine.h"

#include <string>

// Données globales
static LoadedImage gImage;
static float       gZoomFactor = 1.0f;

static const float ZOOM_STEP = 1.25f;
static const float ZOOM_MIN = 0.1f;
static const float ZOOM_MAX = 10.0f;

// Panneau de saisie du message (affiché seulement quand on cache un message)
static HWND gMsgPanel = nullptr;
static HWND gMsgEdit = nullptr;
static HWND gMsgOk = nullptr;
static HWND gMsgCancel = nullptr;

// Hauteur du panneau
static const int MSG_PANEL_HEIGHT = 140;

// ---------------------------------------------------------
// Utilitaires de conversion UTF-16 <-> UTF-8
// ---------------------------------------------------------
static std::string WStringToUTF8(const std::wstring& ws)
{
    if (ws.empty()) return {};

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(),
        (int)ws.size(),
        nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) return {};

    std::string result(sizeNeeded, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(),
        &result[0], sizeNeeded, nullptr, nullptr);
    return result;
}

static std::wstring UTF8ToWString(const std::string& s)
{
    if (s.empty()) return {};

    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, s.c_str(),
        (int)s.size(), nullptr, 0);
    if (sizeNeeded <= 0) return {};

    std::wstring result(sizeNeeded, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(),
        (int)s.size(), &result[0], sizeNeeded);
    return result;
}

// ---------------------------------------------------------
// Calcule la zone où l’image doit être dessinée
// (respect du ratio + zoom + panel optionnel en bas).
// ---------------------------------------------------------
static bool GetImageDrawRect(HWND hwnd, RECT& outRect)
{
    if (gImage.width <= 0 || gImage.height <= 0)
        return false;

    RECT rcClient{};
    GetClientRect(hwnd, &rcClient);

    // Zone utilisée par l'image (si le panneau est visible, on le laisse en bas).
    RECT rcImgArea = rcClient;

    if (gMsgPanel && IsWindowVisible(gMsgPanel))
        rcImgArea.bottom -= MSG_PANEL_HEIGHT;

    int areaW = rcImgArea.right - rcImgArea.left;
    int areaH = rcImgArea.bottom - rcImgArea.top;

    if (areaW <= 0 || areaH <= 0)
        return false;

    float scaleX = (float)areaW / gImage.width;
    float scaleY = (float)areaH / gImage.height;
    float baseScale = (scaleX < scaleY) ? scaleX : scaleY;

    float finalScale = baseScale * gZoomFactor;

    int drawW = (int)(gImage.width * finalScale);
    int drawH = (int)(gImage.height * finalScale);

    outRect.left = rcImgArea.left + (areaW - drawW) / 2;
    outRect.top = rcImgArea.top + (areaH - drawH) / 2;
    outRect.right = outRect.left + drawW;
    outRect.bottom = outRect.top + drawH;

    return true;
}

// ---------------------------------------------------------
// Zoom
// ---------------------------------------------------------
static void ZoomIn(HWND hwnd)
{
    gZoomFactor *= ZOOM_STEP;
    if (gZoomFactor > ZOOM_MAX) gZoomFactor = ZOOM_MAX;
    InvalidateRect(hwnd, nullptr, TRUE);
}

static void ZoomOut(HWND hwnd)
{
    gZoomFactor /= ZOOM_STEP;
    if (gZoomFactor < ZOOM_MIN) gZoomFactor = ZOOM_MIN;
    InvalidateRect(hwnd, nullptr, TRUE);
}

static void ZoomReset(HWND hwnd)
{
    gZoomFactor = 1.0f;
    InvalidateRect(hwnd, nullptr, TRUE);
}

// ---------------------------------------------------------
// Comparaison de deux images + création d'image de différence
// ---------------------------------------------------------
static void CompareImagesAndDiff(const LoadedImage& imgA, const LoadedImage& imgB, HWND hwnd)
{
    if (imgA.width != imgB.width || imgA.height != imgB.height)
    {
        MessageBox(hwnd, L"Les images n'ont pas la même taille.", L"Comparaison", MB_OK | MB_ICONERROR);
        return;
    }

    if (imgA.pixels.empty() || imgB.pixels.empty())
    {
        MessageBox(hwnd, L"Une des images est vide.", L"Comparaison", MB_OK | MB_ICONERROR);
        return;
    }

    size_t totalPixels = (size_t)imgA.width * imgA.height;
    size_t pixelsDiff = 0;
    size_t channelsDiff = 0;

    for (size_t i = 0; i < totalPixels; ++i)
    {
        const uint8_t* pA = &imgA.pixels[i * 4];
        const uint8_t* pB = &imgB.pixels[i * 4];

        bool pixelChanged = false;
        for (int c = 0; c < 3; ++c)
        {
            if (pA[c] != pB[c])
            {
                pixelChanged = true;
                ++channelsDiff;
            }
        }
        if (pixelChanged) ++pixelsDiff;
    }

    wchar_t buffer[256];
    swprintf_s(buffer, L"Pixels différents : %zu\nCanaux différents : %zu",
        pixelsDiff, channelsDiff);
    MessageBox(hwnd, buffer, L"Comparaison d'images", MB_OK | MB_ICONINFORMATION);

    // Image de différence rouge/noir
    LoadedImage diff;
    diff.width = imgA.width;
    diff.height = imgA.height;
    diff.pixels.resize(diff.width * diff.height * 4);

    for (int y = 0; y < diff.height; ++y)
    {
        for (int x = 0; x < diff.width; ++x)
        {
            int idx = (y * diff.width + x) * 4;

            const uint8_t* pA = &imgA.pixels[idx];
            const uint8_t* pB = &imgB.pixels[idx];
            uint8_t* pD = &diff.pixels[idx];

            bool same = (pA[0] == pB[0] && pA[1] == pB[1] && pA[2] == pB[2]);

            if (same)
            {
                pD[0] = pD[1] = pD[2] = 0;
            }
            else
            {
                pD[0] = 0;
                pD[1] = 0;
                pD[2] = 255;
            }
            pD[3] = 255;
        }
    }

    // Sauvegarde de l'image de différence
    OPENFILENAME ofn{};
    wchar_t fileName[MAX_PATH] = {};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Image BMP\0*.bmp\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn))
    {
        if (SaveImageAny(ofn.lpstrFile, diff))
            MessageBox(hwnd, L"Image de différence sauvegardée.", L"OK", MB_OK | MB_ICONINFORMATION);
        else
            MessageBox(hwnd, L"Erreur lors de la sauvegarde de l'image de différence.", L"Erreur", MB_OK | MB_ICONERROR);
    }
}

// ---------------------------------------------------------
// Affichage / masquage du panneau de message
// ---------------------------------------------------------
static void ShowMessagePanel(HWND hwnd, bool show)
{
    if (!gMsgPanel)
        return;

    ShowWindow(gMsgPanel, show ? SW_SHOWNA : SW_HIDE);
    ShowWindow(gMsgEdit, show ? SW_SHOWNA : SW_HIDE);
    ShowWindow(gMsgOk, show ? SW_SHOWNA : SW_HIDE);
    ShowWindow(gMsgCancel, show ? SW_SHOWNA : SW_HIDE);

    if (show)
        SetFocus(gMsgEdit);

    InvalidateRect(hwnd, nullptr, TRUE);
}

// ---------------------------------------------------------
// Procédure de la fenêtre principale
// ---------------------------------------------------------
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        RECT rc{};
        GetClientRect(hwnd, &rc);

        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        // Création du panneau de message (invisible au départ)
        gMsgPanel = CreateWindowEx(
            0,
            L"STATIC",
            nullptr,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0,
            height - MSG_PANEL_HEIGHT,
            width,
            MSG_PANEL_HEIGHT,
            hwnd,
            (HMENU)ID_MSG_PANEL,
            GetModuleHandle(nullptr),
            nullptr
        );

        // Couleur grise de fond
        SendMessage(gMsgPanel, WM_SETTEXT, 0, (LPARAM)L"");

        gMsgEdit = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            nullptr,
            WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
            10,
            10,
            width - 20,
            MSG_PANEL_HEIGHT - 50,
            hwnd,
            (HMENU)ID_MSG_EDIT,
            GetModuleHandle(nullptr),
            nullptr
        );

        gMsgOk = CreateWindow(
            L"BUTTON",
            L"Cacher le message",
            WS_CHILD,
            10,
            MSG_PANEL_HEIGHT - 35,
            140,
            25,
            hwnd,
            (HMENU)ID_MSG_OK,
            GetModuleHandle(nullptr),
            nullptr
        );

        gMsgCancel = CreateWindow(
            L"BUTTON",
            L"Annuler",
            WS_CHILD,
            160,
            MSG_PANEL_HEIGHT - 35,
            80,
            25,
            hwnd,
            (HMENU)ID_MSG_CANCEL,
            GetModuleHandle(nullptr),
            nullptr
        );

        // Masquer au départ
        ShowMessagePanel(hwnd, false);
    }
    return 0;

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        if (gMsgPanel)
        {
            MoveWindow(gMsgPanel, 0, height - MSG_PANEL_HEIGHT, width, MSG_PANEL_HEIGHT, TRUE);
        }
        if (gMsgEdit)
        {
            MoveWindow(gMsgEdit, 10, height - MSG_PANEL_HEIGHT + 10, width - 20, MSG_PANEL_HEIGHT - 50, TRUE);
        }
        if (gMsgOk)
        {
            MoveWindow(gMsgOk, 10, height - 35, 140, 25, TRUE);
        }
        if (gMsgCancel)
        {
            MoveWindow(gMsgCancel, 160, height - 35, 80, 25, TRUE);
        }

        InvalidateRect(hwnd, nullptr, TRUE);
    }
    return 0;

    case WM_COMMAND:
    {
        const int id = LOWORD(wParam);

        // Boutons du panneau
        if (id == ID_MSG_OK)
        {
            if (gImage.pixels.empty())
            {
                MessageBox(hwnd, L"Aucune image chargée.", L"Stéganographie", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            int len = GetWindowTextLengthW(gMsgEdit);
            if (len <= 0)
            {
                MessageBox(hwnd, L"Veuillez entrer un message.", L"Stéganographie", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            std::wstring ws;
            ws.resize(len + 1);
            GetWindowTextW(gMsgEdit, &ws[0], len + 1);
            ws.resize(len); // enlever le '\0'

            std::string msg = WStringToUTF8(ws);

            if (Steg::EmbedLSB(gImage, msg))
            {
                MessageBox(hwnd, L"Message caché dans l'image.", L"Stéganographie", MB_OK | MB_ICONINFORMATION);
                ShowMessagePanel(hwnd, false);
            }
            else
            {
                MessageBox(hwnd, L"Message trop long pour cette image.", L"Stéganographie", MB_OK | MB_ICONERROR);
            }
            return 0;
        }
        else if (id == ID_MSG_CANCEL)
        {
            ShowMessagePanel(hwnd, false);
            return 0;
        }

        // Menus
        switch (id)
        {
        case ID_FILE_OPEN:
        {
            OPENFILENAME ofn{};
            wchar_t fileName[MAX_PATH] = {};

            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = fileName;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"Images (*.bmp; *.png; *.jpg; *.jpeg)\0*.bmp;*.png;*.jpg;*.jpeg\0Tous les fichiers (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

            if (GetOpenFileName(&ofn))
            {
                LoadedImage img;
                if (LoadImageAny(ofn.lpstrFile, img))
                {
                    gImage = std::move(img);
                    ZoomReset(hwnd);
                }
                else
                {
                    MessageBox(hwnd, L"Échec du chargement de l'image.", L"Erreur", MB_OK | MB_ICONERROR);
                }
            }
        }
        return 0;

        case ID_FILE_SAVE:
        {
            if (gImage.pixels.empty())
            {
                MessageBox(hwnd, L"Aucune image à sauvegarder.", L"Sauvegarde", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            OPENFILENAME ofn{};
            wchar_t fileName[MAX_PATH] = {};

            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = fileName;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter =
                L"PNG (*.png)\0*.png\0"
                L"JPEG (*.jpg; *.jpeg)\0*.jpg;*.jpeg\0"
                L"BMP (*.bmp)\0*.bmp\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_OVERWRITEPROMPT;

            if (GetSaveFileName(&ofn))
            {
                if (SaveImageAny(ofn.lpstrFile, gImage))
                    MessageBox(hwnd, L"Image sauvegardée.", L"Sauvegarde", MB_OK | MB_ICONINFORMATION);
                else
                    MessageBox(hwnd, L"Erreur lors de la sauvegarde.", L"Erreur", MB_OK | MB_ICONERROR);
            }
        }
        return 0;

        case ID_FILE_EXIT:
            PostQuitMessage(0);
            return 0;

        case ID_STEG_EMBED:
            if (gImage.pixels.empty())
            {
                MessageBox(hwnd, L"Aucune image chargée.", L"Stéganographie", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                // Afficher le panneau de saisie
                SetWindowTextW(gMsgEdit, L"");
                ShowMessagePanel(hwnd, true);
            }
            return 0;

        case ID_STEG_EXTRACT:
        {
            if (gImage.pixels.empty())
            {
                MessageBox(hwnd, L"Aucune image chargée.", L"Stéganographie", MB_OK | MB_ICONINFORMATION);
                return 0;
            }

            std::string msg;
            if (Steg::ExtractLSB(gImage, msg))
            {
                std::wstring wmsg = UTF8ToWString(msg);
                MessageBox(hwnd, wmsg.c_str(), L"Message caché trouvé", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                MessageBox(hwnd, L"Aucun message caché trouvé.", L"Stéganographie", MB_OK | MB_ICONINFORMATION);
            }
        }
        return 0;

        case ID_STEG_COMPARE:
        {
            // --- Image A ---
            OPENFILENAME ofnA{};
            wchar_t fileA[MAX_PATH] = {};
            ofnA.lStructSize = sizeof(ofnA);
            ofnA.hwndOwner = hwnd;
            ofnA.lpstrFile = fileA;
            ofnA.nMaxFile = MAX_PATH;
            ofnA.lpstrFilter =
                L"Images (*.bmp; *.png; *.jpg; *.jpeg)\0*.bmp;*.png;*.jpg;*.jpeg\0"
                L"Tous les fichiers (*.*)\0*.*\0";
            ofnA.nFilterIndex = 1;
            ofnA.Flags = OFN_FILEMUSTEXIST;

            if (!GetOpenFileName(&ofnA))
                return 0;

            // --- Image B ---
            OPENFILENAME ofnB{};
            wchar_t fileB[MAX_PATH] = {};
            ofnB.lStructSize = sizeof(ofnB);
            ofnB.hwndOwner = hwnd;
            ofnB.lpstrFile = fileB;
            ofnB.nMaxFile = MAX_PATH;
            ofnB.lpstrFilter =
                L"Images (*.bmp; *.png; *.jpg; *.jpeg)\0*.bmp;*.png;*.jpg;*.jpeg\0"
                L"Tous les fichiers (*.*)\0*.*\0";
            ofnB.nFilterIndex = 1;
            ofnB.Flags = OFN_FILEMUSTEXIST;

            if (!GetOpenFileName(&ofnB))
                return 0;

            // --- Charger ---
            LoadedImage imgA, imgB;
            if (!LoadImageAny(ofnA.lpstrFile, imgA) ||
                !LoadImageAny(ofnB.lpstrFile, imgB))
            {
                MessageBox(hwnd, L"Impossible de charger les images.", L"Comparaison", MB_OK | MB_ICONERROR);
                return 0;
            }

            CompareImagesAndDiff(imgA, imgB, hwnd);
        }
        return 0;


        case ID_VIEW_ZOOM_IN:
            ZoomIn(hwnd);
            return 0;

        case ID_VIEW_ZOOM_OUT:
            ZoomOut(hwnd);
            return 0;

        case ID_VIEW_ZOOM_RESET:
            ZoomReset(hwnd);
            return 0;
        }
    }
    return 0;

    case WM_LBUTTONDOWN:
        ZoomIn(hwnd);
        return 0;

    case WM_RBUTTONDOWN:
        ZoomOut(hwnd);
        return 0;

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (delta > 0) ZoomIn(hwnd);
        else if (delta < 0) ZoomOut(hwnd);
    }
    return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (!gImage.pixels.empty())
        {
            RECT rcDraw{};
            if (GetImageDrawRect(hwnd, rcDraw))
            {
                RenderImageGDIPlus(hdc, rcDraw, gImage);
            }
        }

        EndPaint(hwnd, &ps);
    }
    return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
