// Config module: Global configuration variables
// Implementation file with variable definitions

#include "config_vars.hpp"
#include <Windows.h>

namespace config {

// ==================== ESP Settings ====================
bool g_whEnabled = true;
int g_whKey = VK_F7;
ImVec4 g_boxColor = ImVec4(1.0f, 0.90f, 0.43f, 1.0f);
float g_boxPadding = 1.5f;
float g_boxThickness = 2.0f;
bool g_dynamicBoxColor = false;

bool g_nameEspEnabled = true;
ImVec4 g_nameColor = ImVec4(0.90f, 0.90f, 0.95f, 1.0f);
float g_nameOffsetY = 14.0f;

bool g_gunEspEnabled = true;
ImVec4 g_gunColor = ImVec4(0.75f, 0.80f, 1.0f, 1.0f);
float g_gunOffsetY = 2.0f;
float g_gunOffsetX = 0.0f;
ImVec4 g_weaponIconColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

bool g_hpBarEnabled = true;
ImVec4 g_hpBarColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
float g_hpBarWidth = 4.0f;
float g_hpBarOffset = 3.0f;

bool g_bonesEnabled = false;
int g_bonesKey = 0;
ImVec4 g_boneColor = ImVec4(0.60f, 0.88f, 1.00f, 1.0f);

bool g_snaplinesEnabled = false;
bool g_distanceEnabled = true;

bool g_oofArrowsEnabled = false;
float g_oofArrowsRadius = 150.0f;
float g_oofArrowsSize = 15.0f;
ImVec4 g_oofArrowsColor = ImVec4(1.0f, 0.2f, 0.2f, 0.8f);

bool g_bombEspEnabled = true;

bool g_damageIndicatorsEnabled = false;
ImVec4 g_damageColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
float g_damageTextSize = 20.0f;
float g_damageTextLifetime = 4.0f;

// Hitmarker visuals (toggle lives in ui_legacy.cpp as g_hitmarkerEnabled)
int g_hitmarkerStyle = 0;                                  // 0=Cross, 1=Plus, 2=T, 3=Dot
ImVec4 g_hitmarkerColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
float g_hitmarkerSize = 8.0f;
float g_hitmarkerThickness = 2.0f;

// Hit sound volume 0..100
int g_hitSoundVolume = 80;

bool g_spectatorListEnabled = true;

// ==================== Combat Settings ====================
bool g_aimbotEnabled = false;
int g_aimbotKey = VK_XBUTTON2;
bool g_aimbotTeamCheck = true;
int g_aimbotBone = 6;
float g_aimbotFov = 5.0f;
float g_aimbotSmooth = 3.0f;
bool g_aimbotDrawFov = true;
ImVec4 g_aimbotFovColor = ImVec4(1.0f, 1.0f, 0.0f, 0.4f);
bool g_aimbotVisCheck = true;
bool g_aimbotAutoFire = false;

bool g_triggerEnabled = true;
int g_triggerKey = 'X';
bool g_triggerTeamCheck = true;
bool g_triggerScopeOnly = false;
bool g_triggerHeadOnly = false;
bool g_triggerVisCheck = true;
float g_triggerAccuracyThreshold = 0.015f;
int g_triggerToggleKey = 0;

bool g_noRecoilEnabled = false;
bool g_noSpreadEnabled = false;

// RCS (настоящая компенсация отдачи во время стрельбы)
bool g_rcsEnabled  = false;
int  g_rcsStrength = 75;  // 75% по умолчанию — не palится как 100%

// Auto-stop
bool g_autoStopEnabled = false;

// Auto-knife
bool g_autoKnifeEnabled = false;

// ==================== Movement Settings ====================
bool g_bhopEnabled = true;
int g_bhopKey = VK_SPACE;
bool g_autoStrafeEnabled = false;

// Jump-Duck
bool g_jumpDuckEnabled = false;
int  g_jumpDuckKey = 0;

// Jump Scout
bool g_jumpScoutEnabled = false;

// ==================== Player Settings ====================
bool g_antiFlashEnabled = false;
bool g_fovEnabled = false;
float g_fovValue = 90.0f;
bool g_chamsEnabled = false;
ImVec4 g_chamsColorT = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
ImVec4 g_chamsColorCT = ImVec4(0.0f, 0.65f, 1.0f, 1.0f);
bool g_noSmokeEnabled = false;

// ==================== UI Settings ====================
bool g_menuOpen = false;
bool g_skinMenuOpen = false;
bool g_skinWarningShown = false;
bool g_scopeSightEnabled = true;
bool g_keybindsListEnabled = false;
bool g_antiCaptureEnabled = false;

// ---- Cyber theme (new) ----
ImVec4 g_uiAccentColor      = ImVec4(0.00f, 0.90f, 1.00f, 1.00f); // #00E5FF cyan
ImVec4 g_uiAccentSecondary  = ImVec4(1.00f, 0.00f, 0.50f, 1.00f); // #FF0080 magenta
bool   g_uiScanlineEnabled  = true;
bool   g_uiHoverGlowEnabled = true;
bool   g_uiAnimationsEnabled = true;
bool   g_uiParticlesEnabled  = false;
float  g_uiScanlineIntensity = 0.5f;

// ==================== Radar Settings ====================
bool g_radarHackEnabled = false;
float g_radarScale = 4.0f;
float g_radarSize = 160.0f;

// ==================== Flash Settings ====================
ImVec4 g_flashTextColorOn = ImVec4(1.0f, 0.9f, 0.1f, 1.0f);
ImVec4 g_flashTextColorOff = ImVec4(0.5f, 0.9f, 0.5f, 1.0f);
float g_flashTextScale = 1.0f;

} // namespace config
