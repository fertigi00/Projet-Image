#pragma once
#include <windows.h>
#include <string>

bool Dialog_AskText(HWND parent, std::string& outText);

void Dialog_ShowMessage(HWND parent, const std::string& text);
