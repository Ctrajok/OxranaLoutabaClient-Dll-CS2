#ifndef CONFIG_VARS_HPP
#define CONFIG_VARS_HPP

#include "../../../imgui-1.92.5/imgui.h"
#include <vector>
#include <map>
#include <string>
#include <cstdint>

// Config module: Global configuration variables
// This file contains extern declarations for all configuration variables

namespace config {

// ==================== ESP Settings ====================
extern bool g_whEnabled;
extern int g_whKey;
extern ImVec4 g_boxColor;
extern float g_boxPadding;
extern float g_boxThickness;
extern bool g_dynamicBoxColor;

extern bool g_nameEspEnabled;
extern ImVec4 g_nameColor;
extern float g_nameOffsetY;

extern bool g_gunEspEnabled;
extern ImVec4 g_gunColor;
extern float g_gunOffsetY;
extern float g_gunOffsetX;
extern ImVec4 g_weaponIconColor;

extern bool g_hpBarEnabled;
extern ImVec4 g_hpBarColor;
extern float g_hpBarWidth;
extern float g_hpBarOffset;

extern bool g_bonesEnabled;
extern int g_bonesKey;
extern ImVec4 g_boneColor;

extern bool g_snaplinesEnabled;
extern bool g_distanceEnabled;

extern bool g_oofArrowsEnabled;
extern float g_oofArrowsRadius;
extern float g_oofArrowsSize;
extern ImVec4 g_oofArrowsColor;

extern bool g_bombEspEnabled;

extern bool g_damageIndicatorsEnabled;
extern ImVec4 g_damageColor;
extern float g_damageTextSize;
extern float g_damageTextLifetime;

// Hitmarker visuals
extern int g_hitmarkerStyle;
extern ImVec4 g_hitmarkerColor;
extern float g_hitmarkerSize;
extern float g_hitmarkerThickness;

// Hit sound volume
extern int g_hitSoundVolume;  // 0..100

extern bool g_spectatorListEnabled;

// ==================== Combat Settings ====================
extern bool g_aimbotEnabled;
extern int g_aimbotKey;
extern bool g_aimbotTeamCheck;
extern int g_aimbotBone;
extern float g_aimbotFov;
extern float g_aimbotSmooth;
extern bool g_aimbotDrawFov;
extern ImVec4 g_aimbotFovColor;
extern bool g_aimbotVisCheck;
extern bool g_aimbotAutoFire;

extern bool g_triggerEnabled;
extern int g_triggerKey;
extern bool g_triggerTeamCheck;
extern bool g_triggerScopeOnly;
extern bool g_triggerHeadOnly;
extern bool g_triggerVisCheck;
extern float g_triggerAccuracyThreshold;
extern int g_triggerToggleKey;

extern bool g_noRecoilEnabled;
extern bool g_noSpreadEnabled;

// RCS (Recoil Control System) — постепенная компенсация отдачи
extern bool  g_rcsEnabled;
extern int   g_rcsStrength;  // 0..100 процент компенсации

// Auto-stop: тормозит перед выстрелом
extern bool g_autoStopEnabled;

// Auto-knife: бьёт ножом когда враг в радиусе
extern bool g_autoKnifeEnabled;

// ==================== Movement Settings ====================
extern bool g_bhopEnabled;
extern int g_bhopKey;
extern bool g_autoStrafeEnabled;

// Jump-Duck (jump + duck по одной кнопке)
extern bool g_jumpDuckEnabled;
extern int  g_jumpDuckKey;

// Jump Scout (автовыстрел при приземлении с SSG/AWP)
extern bool g_jumpScoutEnabled;

// ==================== Player Settings ====================
extern bool g_antiFlashEnabled;
extern bool g_fovEnabled;
extern float g_fovValue;
extern bool g_chamsEnabled;
extern ImVec4 g_chamsColorT;
extern ImVec4 g_chamsColorCT;
extern bool g_noSmokeEnabled;

// ==================== UI Settings ====================
extern bool g_menuOpen;
extern bool g_skinMenuOpen;
extern bool g_skinWarningShown;
extern bool g_scopeSightEnabled;
extern bool g_keybindsListEnabled;
extern bool g_antiCaptureEnabled;

// ---- Cyber theme (new) ----
extern ImVec4 g_uiAccentColor;
extern ImVec4 g_uiAccentSecondary;
extern bool   g_uiScanlineEnabled;
extern bool   g_uiHoverGlowEnabled;
extern bool   g_uiAnimationsEnabled;
extern bool   g_uiParticlesEnabled;
extern float  g_uiScanlineIntensity;

// ==================== Radar Settings ====================
extern bool g_radarHackEnabled;
extern float g_radarScale;
extern float g_radarSize;

// ==================== Flash Settings ====================
extern ImVec4 g_flashTextColorOn;
extern ImVec4 g_flashTextColorOff;
extern float g_flashTextScale;

} // namespace config

#endif // CONFIG_VARS_HPP
