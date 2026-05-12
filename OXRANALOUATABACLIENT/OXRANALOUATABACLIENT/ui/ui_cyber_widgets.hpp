#pragma once

// Cyber-style widget set. Wraps / replaces the PremiumToggle family.
// Each widget is a drop-in for its ImGui counterpart (same coordinates,
// same click-routing) so the existing DrawMenuImGui keeps working.

#include "../../imgui-1.92.5/imgui.h"

namespace ui {

// -- core widgets ----------------------------------------------------------
bool CyberToggle(const char* label, bool* v);
bool CyberButton(const char* label, ImVec2 size = ImVec2(0, 0));
bool CyberSlider(const char* label, float* v, float vmin, float vmax, const char* fmt = "%.2f");
bool CyberSliderInt(const char* label, int* v, int vmin, int vmax, const char* fmt = "%d");
bool CyberCombo(const char* label, int* current_item, const char* const items[], int items_count);
bool CyberColorEdit4(const char* label, ImVec4* col, ImGuiColorEditFlags flags = 0);

// -- settings / popup ------------------------------------------------------
bool CyberSettingsButton(const char* id);

// -- layout primitives (Stage 4 will flesh these out) ----------------------
bool CyberBeginPanel(const char* id, ImVec2 size = ImVec2(0, 0));
void CyberEndPanel();
void CyberSectionHeader(const char* label);
bool CyberSidebarItem(const char* id, const char* icon, const char* label, const char* sublabel, bool isActive);

// -- overlay helpers (used internally by widgets and by DrawMenuImGui) -----
void CyberDrawHoverGlow(ImVec2 min, ImVec2 max, float rounding, float alpha01);
void CyberDrawGradientBorder(ImVec2 min, ImVec2 max, float rounding);
void CyberDrawGlassFill(ImVec2 min, ImVec2 max, float rounding);
void CyberDrawScanlines(ImVec2 min, ImVec2 max);

} // namespace ui
