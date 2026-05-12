#pragma once

namespace ui {

// Premium toggle widget with animation
bool PremiumToggle(const char* label, bool* v);

// Keybind widget for capturing key presses
bool KeybindWidget(const char* id, int& vk);

// Convert virtual key code to string
const char* VkToStringA(int vk);

} // namespace ui
