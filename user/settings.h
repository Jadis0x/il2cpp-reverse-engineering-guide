#pragma once

#include "keybinds.h"

class Settings {
public:
 KeyBinds::Config KeyBinds = {
 VK_TAB // toggle menu
 };

 bool ImGuiInitialized = false;
 bool bShowMenu = false;
 bool bEnableUnityLogs = true;
};

extern Settings settings;