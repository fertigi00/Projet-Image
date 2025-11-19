#include "MainWindow.h"
#include "RendererGDI.h"
#include "ImageManager.h"
#include "StegEngine.h"  // pour EmbedLSB / ExtractLSB
#include <windows.h>
#include <string>

// Image globale
LoadedImage gImage = {}; // pixels vide, width/height = 0
std::string gExtractedMessage; // message extrait à afficher sur l'image
// --- Fonction utilitaire pour conversion bits -> octets ---
void BitsToBytes(const std::vector<uint8_t>& bits, std::vector<uint8_t>& bytes)
{
    bytes.clear();
    size_t n = bits.size() / 8;
    for (size_t i = 0; i < n; ++i)
    {
        uint8_t b = 0;
        for (int j = 0; j < 8; ++j)
        {
            b <<= 1;
            b |= (bits[i * 8 + j] & 1);
        }
        bytes.push_back(b);
    }
}

// --- Fonction de comparaison de deux images BMP 24 bits ---
void CompareImages(const LoadedImage& imgA, const LoadedImage& imgB, HWND hwnd)
{
    if (imgA.width != imgB.width || imgA.height != imgB.height)
    {
        MessageBox(hwnd, L"Les images n'ont pas la même taille. Comparaison impossible.", L"Erreur", MB_OK | MB_ICONERROR);
        return;
    }

    if (imgA.pixels.empty() || imgB.pixels.empty())
    {
        MessageBox(hwnd, L"Une des images est vide.", L"Erreur", MB_OK | MB_ICONERROR);
        return;
    }

    size_t totalPixels = imgA.width * imgA.height;
    size_t pixelsModified = 0;
    size_t channelsModified = 0;
    size_t lsbModified = 0;

    for (size_t i = 0; i < totalPixels; ++i)
    {
        const uint8_t* pxA = &imgA.pixels[i * 4]; // 4 bytes per pixel RGBA
        const uint8_t* pxB = &imgB.pixels[i * 4];

        bool pixelChanged = false;

        // Comparaison stricte (B, G, R)
        for (int c = 0; c < 3; ++c)
        {
            if (pxA[c] != pxB[c])
            {
                pixelChanged = true;
                channelsModified++;
            }

            // Comparaison LSB
            if ((pxA[c] & 1) != (pxB[c] & 1))
                lsbModified++;
        }

        if (pixelChanged)
            pixelsModified++;
    }

    // Affichage du verdict
    wchar_t buffer[512];
    if (pixelsModified == 0)
        swprintf_s(buffer, L"Aucune différence détectée.\nPixels modifiés: 0\nCanaux modifiés: 0\nBits LSB modifiés: %zu", lsbModified);
    else
        swprintf_s(buffer, L"Différences détectées :\nPixels modifiés: %zu\nCanaux modifiés: %zu\nBits LSB modifiés: %zu",
            pixelsModified, channelsModified, lsbModified);

    MessageBox(hwnd, buffer, L"Résultat de la comparaison", MB_OK | MB_ICONINFORMATION);
}
// Génère une image de différence visuelle entre imgA et imgB
LoadedImage CreateDifferenceImage(const LoadedImage& imgA, const LoadedImage& imgB)
{
    LoadedImage diff = imgA; // même dimensions
    size_t pixels = imgA.width * imgA.height;

    for (size_t i = 0; i < pixels; i++)
    {
        const uint8_t* pxA = &imgA.pixels[i * 4]; // B,G,R,A
        const uint8_t* pxB = &imgB.pixels[i * 4];
        uint8_t* pxD = &diff.pixels[i * 4];

        if (pxA[0] == pxB[0] && pxA[1] == pxB[1] && pxA[2] == pxB[2])
        {
            // pixel identique -> noir
            pxD[0] = 0; // B
            pxD[1] = 0; // G
            pxD[2] = 0; // R
        }
        else
        {
            // pixel différent -> rouge
            pxD[0] = 0;   // B
            pxD[1] = 0;   // G
            pxD[2] = 255; // R
        }
        pxD[3] = 255; // alpha plein
    }

    return diff;
}


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



        // --- case 11: Extract message ---
        case 11:
        {
            gExtractedMessage.clear();

            std::string tmpMsg;
            bool result = Steg::ExtractLSB(gImage, tmpMsg);

            if (result)
            {
                // Extraction OK, afficher le message
                gExtractedMessage = tmpMsg;
                std::wstring wmsg(gExtractedMessage.begin(), gExtractedMessage.end());
                MessageBox(hwnd, wmsg.c_str(), L"Message caché", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                // Extraction échouée : vérifier pourquoi
                size_t pixels = gImage.width * gImage.height;
                std::vector<uint8_t> bits;

                for (size_t i = 0; i < pixels; i++) {
                    const uint8_t* px = &gImage.pixels[i * 4];
                    bits.push_back(px[0] & 1);
                    bits.push_back(px[1] & 1);
                    bits.push_back(px[2] & 1);
                }

                if (bits.size() < 18 * 8) {
                    MessageBox(hwnd, L"Image trop petite pour contenir un message.", L"Erreur", MB_OK | MB_ICONERROR);
                    break;
                }

                std::vector<uint8_t> headerBytes;
                BitsToBytes(std::vector<uint8_t>(bits.begin(), bits.begin() + 18 * 8), headerBytes);

                // Vérifier MAGIC
                uint32_t magic = (headerBytes[0] << 24) | (headerBytes[1] << 16) | (headerBytes[2] << 8) | headerBytes[3];
                if (magic != Steg::MAGIC) {
                    MessageBox(hwnd, L"Message corrompu : magic invalide.", L"Erreur", MB_OK | MB_ICONERROR);
                    break;
                }

                // Vérifier longueur
                uint32_t length = (headerBytes[4] << 24) | (headerBytes[5] << 16) | (headerBytes[6] << 8) | headerBytes[7];
                if (length == 0 || 12 + length > (bits.size() / 8)) {
                    MessageBox(hwnd, L"Message corrompu : longueur invalide.", L"Erreur", MB_OK | MB_ICONERROR);
                    break;
                }

                // Vérifier CRC32
                uint32_t crcStored = (headerBytes[8] << 24) | (headerBytes[9] << 16) | (headerBytes[10] << 8) | headerBytes[11];
                std::vector<uint8_t> msgBytes;
                BitsToBytes(std::vector<uint8_t>(bits.begin() + 96, bits.begin() + 96 + length * 8), msgBytes);
                uint32_t crcCalc = Steg::ComputeCRC32(msgBytes.data(), msgBytes.size());

                if (crcCalc != crcStored) {
                    MessageBox(hwnd, L"Message corrompu : CRC invalide.", L"Erreur", MB_OK | MB_ICONERROR);
                }
                else {
                    MessageBox(hwnd, L"Message introuvable ou fichier modifié.", L"Info", MB_OK);
                }
            }
        }
        break;
        case 12: // Comparaison d'Images
        {
            OPENFILENAME ofnA = {};
            wchar_t szFileA[MAX_PATH] = {};
            ofnA.lStructSize = sizeof(ofnA);
            ofnA.hwndOwner = hwnd;
            ofnA.lpstrFile = szFileA;
            ofnA.nMaxFile = MAX_PATH;
            ofnA.lpstrFilter = L"BMP Files\0*.bmp\0All Files\0*.*\0";
            ofnA.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

            if (!GetOpenFileName(&ofnA))
                break; // Annulé

            OPENFILENAME ofnB = {};
            wchar_t szFileB[MAX_PATH] = {};
            ofnB.lStructSize = sizeof(ofnB);
            ofnB.hwndOwner = hwnd;
            ofnB.lpstrFile = szFileB;
            ofnB.nMaxFile = MAX_PATH;
            ofnB.lpstrFilter = L"BMP Files\0*.bmp\0All Files\0*.*\0";
            ofnB.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

            if (!GetOpenFileName(&ofnB))
                break; // Annulé

            LoadedImage imgA = {};
            LoadedImage imgB = {};

            if (!LoadBMPGDIPlus(ofnA.lpstrFile, imgA))
            {
                MessageBox(hwnd, L"Impossible de charger l'image A.", L"Erreur", MB_OK | MB_ICONERROR);
                break;
            }

            if (!LoadBMPGDIPlus(ofnB.lpstrFile, imgB))
            {
                MessageBox(hwnd, L"Impossible de charger l'image B.", L"Erreur", MB_OK | MB_ICONERROR);
                break;
            }
            // Appel de la fonction de comparaison
            CompareImages(imgA, imgB, hwnd);

            // --- BONUS : image de différence visuelle ---
            LoadedImage diffImage = CreateDifferenceImage(imgA, imgB);

            // Boîte de dialogue pour sauvegarder l'image de différence
            OPENFILENAME ofnSave = {};
            wchar_t szFileDiff[MAX_PATH] = {};
            ofnSave.lStructSize = sizeof(ofnSave);
            ofnSave.hwndOwner = hwnd;
            ofnSave.lpstrFile = szFileDiff;
            ofnSave.nMaxFile = MAX_PATH;
            ofnSave.lpstrFilter = L"BMP Files\0*.bmp\0All Files\0*.*\0";
            ofnSave.Flags = OFN_OVERWRITEPROMPT;

            if (GetSaveFileName(&ofnSave))
            {
                if (SaveBMPGDIPlus(ofnSave.lpstrFile, diffImage))
                    MessageBox(hwnd, L"Image de différence sauvegardée !", L"Info", MB_OK);
                else
                    MessageBox(hwnd, L"Impossible de sauvegarder l'image de différence.", L"Erreur", MB_OK | MB_ICONERROR);
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
