#pragma once
#include <windows.h>
#include <string>

// Show dialog asking user to input a text message.
// Returns true if OK pressed, false if Cancel.
// Result stored in outText.
bool Dialog_AskText(HWND parent, std::string& outText);

// Show dialog displaying extracted message.
void Dialog_ShowMessage(HWND parent, const std::string& text);
