#include "ui_style.hpp"
#include "../config/config_vars.hpp"
#include "../../imgui-1.92.5/imgui.h"

// NOTE: The cyber theme uses ApplyClientStyle() in ui_legacy.cpp as the real entry point.
// These stubs remain for header/API compatibility — not actively called by anything today.

namespace ui {

void ApplyStyle()
{
    // no-op; cyber theme is applied by ApplyClientStyle() in ui_legacy.cpp
}

void SetThemeColor(float r, float g, float b, float a)
{
    config::g_uiAccentColor = ImVec4(r, g, b, a);
}

} // namespace ui
