// Config module: Configuration save/load management
// Implementation file

#include "config_manager.hpp"
#include "config_vars.hpp"
#include <Windows.h>
#include <string>

// HitSound/Hitmarker toggles live in ui_legacy.cpp
extern bool g_hitSoundEnabled;
extern bool g_hitmarkerEnabled;

// Helper functions (will be moved to utils later)
static bool GetDllDirA(std::string& out)
{
    char path[MAX_PATH];
    extern HMODULE g_hModule; // Defined in cs2_internal_dll.cpp
    if (!GetModuleFileNameA(g_hModule, path, MAX_PATH))
        return false;
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\/");
    if (pos == std::string::npos)
        return false;
    out = fullPath.substr(0, pos);
    return true;
}

static std::string MakePathA(const std::string& dir, const std::string& file)
{
    return dir + "\\" + file;
}

namespace config {

void SaveConfig()
{
    std::string dllDir;
    if (!GetDllDirA(dllDir)) return;
    std::string configPath = MakePathA(dllDir, "config.ini");
    char buf[64];
    
    // Binds
    wsprintfA(buf, "%d", g_aimbotKey); WritePrivateProfileStringA("Binds", "AimbotKey", buf, configPath.c_str());
    wsprintfA(buf, "%d", g_triggerKey); WritePrivateProfileStringA("Binds", "TriggerKey", buf, configPath.c_str());
    wsprintfA(buf, "%d", g_whKey); WritePrivateProfileStringA("Binds", "EspKey", buf, configPath.c_str());
    wsprintfA(buf, "%d", g_bonesKey); WritePrivateProfileStringA("Binds", "BonesKey", buf, configPath.c_str());
    wsprintfA(buf, "%d", g_bhopKey); WritePrivateProfileStringA("Binds", "BhopKey", buf, configPath.c_str());
    
    // ESP Floats
    sprintf_s(buf, sizeof(buf), "%.1f", g_boxPadding); WritePrivateProfileStringA("ESP", "BoxPadding", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.1f", g_boxThickness); WritePrivateProfileStringA("ESP", "BoxThickness", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.1f", g_hpBarWidth); WritePrivateProfileStringA("ESP", "HPBarWidth", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.1f", g_hpBarOffset); WritePrivateProfileStringA("ESP", "HPBarOffset", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.1f", g_nameOffsetY); WritePrivateProfileStringA("ESP", "NameOffsetY", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.1f", g_gunOffsetY); WritePrivateProfileStringA("ESP", "GunOffsetY", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.1f", g_gunOffsetX); WritePrivateProfileStringA("ESP", "GunOffsetX", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.1f", g_damageTextSize); WritePrivateProfileStringA("ESP", "DamageTextSize", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.1f", g_damageTextLifetime); WritePrivateProfileStringA("ESP", "DamageTextLifetime", buf, configPath.c_str());
    
    // Aimbot Floats & Settings
    sprintf_s(buf, sizeof(buf), "%.1f", g_aimbotFov); WritePrivateProfileStringA("Combat", "AimbotFov", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.1f", g_aimbotSmooth); WritePrivateProfileStringA("Combat", "AimbotSmooth", buf, configPath.c_str());
    WritePrivateProfileStringA("Combat", "AimbotTeamCheck", g_aimbotTeamCheck ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Combat", "AimbotVisCheck", g_aimbotVisCheck ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Combat", "AimbotAutoFire", g_aimbotAutoFire ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Combat", "AimbotDrawFov", g_aimbotDrawFov ? "1" : "0", configPath.c_str());

    // Booleans Visuals
    WritePrivateProfileStringA("Visuals", "NameEsp", g_nameEspEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "GunEsp", g_gunEspEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "HpBar", g_hpBarEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "DynamicBoxColor", g_dynamicBoxColor ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "DamageIndicators", g_damageIndicatorsEnabled ? "1" : "0", configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%d", g_hitmarkerStyle); WritePrivateProfileStringA("Visuals", "HitmarkerStyle", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.2f,%.2f,%.2f,%.2f", g_hitmarkerColor.x, g_hitmarkerColor.y, g_hitmarkerColor.z, g_hitmarkerColor.w);
    WritePrivateProfileStringA("Visuals", "HitmarkerColor", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.1f", g_hitmarkerSize); WritePrivateProfileStringA("Visuals", "HitmarkerSize", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.1f", g_hitmarkerThickness); WritePrivateProfileStringA("Visuals", "HitmarkerThickness", buf, configPath.c_str());
    WritePrivateProfileStringA("Visuals", "WhEnabled", g_whEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "BonesEnabled", g_bonesEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "ChamsEnabled", g_chamsEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "SpectatorList", g_spectatorListEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "BombEsp", g_bombEspEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "ScopeSight", g_scopeSightEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "HitSound", g_hitSoundEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "Hitmarker", g_hitmarkerEnabled ? "1" : "0", configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%d", g_hitSoundVolume);
    WritePrivateProfileStringA("Visuals", "HitSoundVolume", buf, configPath.c_str());

    // Grenade Predictor
    // (removed)
    
    // Дополнительные функции Visuals
    WritePrivateProfileStringA("Visuals", "Snaplines", g_snaplinesEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "Distance", g_distanceEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "RadarHack", g_radarHackEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "OofArrows", g_oofArrowsEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Visuals", "KeybindsList", g_keybindsListEnabled ? "1" : "0", configPath.c_str());
    
    // Misc
    WritePrivateProfileStringA("Misc", "SkinWarningShown", g_skinWarningShown ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Misc", "AutoStrafe", g_autoStrafeEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Movement", "JumpDuck", g_jumpDuckEnabled ? "1" : "0", configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%d", g_jumpDuckKey);
    WritePrivateProfileStringA("Movement", "JumpDuckKey", buf, configPath.c_str());
    WritePrivateProfileStringA("Movement", "JumpScout", g_jumpScoutEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Misc", "AntiCapture", g_antiCaptureEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Combat", "AimbotEnabled", g_aimbotEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Combat", "TriggerEnabled", g_triggerEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Combat", "RcsEnabled", g_rcsEnabled ? "1" : "0", configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%d", g_rcsStrength);
    WritePrivateProfileStringA("Combat", "RcsStrength", buf, configPath.c_str());
    WritePrivateProfileStringA("Combat", "AutoStop", g_autoStopEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Combat", "AutoKnife", g_autoKnifeEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Misc", "BhopEnabled", g_bhopEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Misc", "AntiFlash", g_antiFlashEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("Misc", "NoSmoke", g_noSmokeEnabled ? "1" : "0", configPath.c_str());
    
    // Сохранение значений слайдеров
    sprintf_s(buf, sizeof(buf), "%.0f", g_oofArrowsRadius); WritePrivateProfileStringA("Visuals", "OofRadius", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.1f", g_oofArrowsSize); WritePrivateProfileStringA("Visuals", "OofSize", buf, configPath.c_str());
    
    // (Menu.Color legacy ключ удалён — используется g_uiAccentColor)

    // ---- Cyber UI theme ----
    sprintf_s(buf, sizeof(buf), "%.3f,%.3f,%.3f,%.3f", g_uiAccentColor.x, g_uiAccentColor.y, g_uiAccentColor.z, g_uiAccentColor.w);
    WritePrivateProfileStringA("UI", "AccentColor", buf, configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.3f,%.3f,%.3f,%.3f", g_uiAccentSecondary.x, g_uiAccentSecondary.y, g_uiAccentSecondary.z, g_uiAccentSecondary.w);
    WritePrivateProfileStringA("UI", "AccentSecondary", buf, configPath.c_str());
    WritePrivateProfileStringA("UI", "Scanline", g_uiScanlineEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("UI", "HoverGlow", g_uiHoverGlowEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("UI", "Animations", g_uiAnimationsEnabled ? "1" : "0", configPath.c_str());
    WritePrivateProfileStringA("UI", "Particles", g_uiParticlesEnabled ? "1" : "0", configPath.c_str());
    sprintf_s(buf, sizeof(buf), "%.2f", g_uiScanlineIntensity); WritePrivateProfileStringA("UI", "ScanlineIntensity", buf, configPath.c_str());
}

void LoadConfig()
{
    std::string dllDir;
    if (!GetDllDirA(dllDir)) return;
    std::string configPath = MakePathA(dllDir, "config.ini");
    char buf[64];
    
    // Binds
    g_aimbotKey = GetPrivateProfileIntA("Binds", "AimbotKey", g_aimbotKey, configPath.c_str());
    g_triggerKey = GetPrivateProfileIntA("Binds", "TriggerKey", g_triggerKey, configPath.c_str());
    g_whKey = GetPrivateProfileIntA("Binds", "EspKey", g_whKey, configPath.c_str());
    g_bonesKey = GetPrivateProfileIntA("Binds", "BonesKey", g_bonesKey, configPath.c_str());
    g_bhopKey = GetPrivateProfileIntA("Binds", "BhopKey", g_bhopKey, configPath.c_str());
    
    // ESP Floats
    GetPrivateProfileStringA("ESP", "BoxPadding", "1.5", buf, sizeof(buf), configPath.c_str()); g_boxPadding = (float)atof(buf);
    GetPrivateProfileStringA("ESP", "BoxThickness", "2.0", buf, sizeof(buf), configPath.c_str()); g_boxThickness = (float)atof(buf);
    GetPrivateProfileStringA("ESP", "HPBarWidth", "4.0", buf, sizeof(buf), configPath.c_str()); g_hpBarWidth = (float)atof(buf);
    GetPrivateProfileStringA("ESP", "HPBarOffset", "3.0", buf, sizeof(buf), configPath.c_str()); g_hpBarOffset = (float)atof(buf);
    GetPrivateProfileStringA("ESP", "NameOffsetY", "14.0", buf, sizeof(buf), configPath.c_str()); g_nameOffsetY = (float)atof(buf);
    GetPrivateProfileStringA("ESP", "GunOffsetY", "2.0", buf, sizeof(buf), configPath.c_str()); g_gunOffsetY = (float)atof(buf);
    GetPrivateProfileStringA("ESP", "GunOffsetX", "0.0", buf, sizeof(buf), configPath.c_str()); g_gunOffsetX = (float)atof(buf);
    GetPrivateProfileStringA("ESP", "DamageTextSize", "20.0", buf, sizeof(buf), configPath.c_str()); g_damageTextSize = (float)atof(buf);
    GetPrivateProfileStringA("ESP", "DamageTextLifetime", "4.0", buf, sizeof(buf), configPath.c_str()); g_damageTextLifetime = (float)atof(buf);

    // Aimbot Floats & Settings
    GetPrivateProfileStringA("Combat", "AimbotFov", "5.0", buf, sizeof(buf), configPath.c_str()); g_aimbotFov = (float)atof(buf);
    GetPrivateProfileStringA("Combat", "AimbotSmooth", "3.0", buf, sizeof(buf), configPath.c_str()); g_aimbotSmooth = (float)atof(buf);
    g_aimbotTeamCheck = GetPrivateProfileIntA("Combat", "AimbotTeamCheck", g_aimbotTeamCheck ? 1 : 0, configPath.c_str()) != 0;
    g_aimbotVisCheck = GetPrivateProfileIntA("Combat", "AimbotVisCheck", g_aimbotVisCheck ? 1 : 0, configPath.c_str()) != 0;
    g_aimbotAutoFire = GetPrivateProfileIntA("Combat", "AimbotAutoFire", g_aimbotAutoFire ? 1 : 0, configPath.c_str()) != 0;
    g_aimbotDrawFov = GetPrivateProfileIntA("Combat", "AimbotDrawFov", g_aimbotDrawFov ? 1 : 0, configPath.c_str()) != 0;

    // Booleans Visuals
    g_nameEspEnabled = GetPrivateProfileIntA("Visuals", "NameEsp", g_nameEspEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_gunEspEnabled = GetPrivateProfileIntA("Visuals", "GunEsp", g_gunEspEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_hpBarEnabled = GetPrivateProfileIntA("Visuals", "HpBar", g_hpBarEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_dynamicBoxColor = GetPrivateProfileIntA("Visuals", "DynamicBoxColor", g_dynamicBoxColor ? 1 : 0, configPath.c_str()) != 0;
    g_damageIndicatorsEnabled = GetPrivateProfileIntA("Visuals", "DamageIndicators", g_damageIndicatorsEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_hitmarkerStyle = GetPrivateProfileIntA("Visuals", "HitmarkerStyle", g_hitmarkerStyle, configPath.c_str());
    GetPrivateProfileStringA("Visuals", "HitmarkerColor", "", buf, sizeof(buf), configPath.c_str());
    if (buf[0] != '\0') {
        float r, g, b, a;
        if (sscanf_s(buf, "%f,%f,%f,%f", &r, &g, &b, &a) == 4) {
            g_hitmarkerColor = ImVec4(r, g, b, a);
        }
    }
    GetPrivateProfileStringA("Visuals", "HitmarkerSize", "8.0", buf, sizeof(buf), configPath.c_str()); g_hitmarkerSize = (float)atof(buf);
    GetPrivateProfileStringA("Visuals", "HitmarkerThickness", "2.0", buf, sizeof(buf), configPath.c_str()); g_hitmarkerThickness = (float)atof(buf);
    g_whEnabled = GetPrivateProfileIntA("Visuals", "WhEnabled", g_whEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_bonesEnabled = GetPrivateProfileIntA("Visuals", "BonesEnabled", g_bonesEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_chamsEnabled = GetPrivateProfileIntA("Visuals", "ChamsEnabled", g_chamsEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_spectatorListEnabled = GetPrivateProfileIntA("Visuals", "SpectatorList", g_spectatorListEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_bombEspEnabled = GetPrivateProfileIntA("Visuals", "BombEsp", g_bombEspEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_scopeSightEnabled = GetPrivateProfileIntA("Visuals", "ScopeSight", g_scopeSightEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_hitSoundEnabled = GetPrivateProfileIntA("Visuals", "HitSound", g_hitSoundEnabled ? 1 : 0, configPath.c_str()) != 0;
    {
        int v = GetPrivateProfileIntA("Visuals", "HitSoundVolume", g_hitSoundVolume, configPath.c_str());
        if (v < 0) v = 0; if (v > 100) v = 100;
        g_hitSoundVolume = v;
    }

    // Grenade Predictor
    auto clampRange = [](float v, float lo, float hi) { if (v < lo) return lo; if (v > hi) return hi; return v; };
    auto clampVec4  = [](ImVec4& v) { auto c=[](float x){ if(x<0.f) return 0.f; if(x>1.f) return 1.f; return x;}; v.x=c(v.x); v.y=c(v.y); v.z=c(v.z); v.w=c(v.w); };
    (void)clampVec4;
    (void)clampRange;

    // Migration: if Hitmarker key is missing, fall back to HitSound value (legacy "Hit Sound & Hitmarker" combined toggle).
    {
        int sentinel = -1;
        int stored = GetPrivateProfileIntA("Visuals", "Hitmarker", sentinel, configPath.c_str());
        g_hitmarkerEnabled = (stored == sentinel) ? g_hitSoundEnabled : (stored != 0);
    }
    
    // Дополнительные функции Visuals
    g_snaplinesEnabled = GetPrivateProfileIntA("Visuals", "Snaplines", g_snaplinesEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_distanceEnabled = GetPrivateProfileIntA("Visuals", "Distance", g_distanceEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_radarHackEnabled = GetPrivateProfileIntA("Visuals", "RadarHack", g_radarHackEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_oofArrowsEnabled = GetPrivateProfileIntA("Visuals", "OofArrows", g_oofArrowsEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_keybindsListEnabled = GetPrivateProfileIntA("Visuals", "KeybindsList", g_keybindsListEnabled ? 1 : 0, configPath.c_str()) != 0;
    
    // Misc
    g_skinWarningShown = GetPrivateProfileIntA("Misc", "SkinWarningShown", g_skinWarningShown ? 1 : 0, configPath.c_str()) != 0;
    g_autoStrafeEnabled = GetPrivateProfileIntA("Misc", "AutoStrafe", g_autoStrafeEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_jumpDuckEnabled = GetPrivateProfileIntA("Movement", "JumpDuck", g_jumpDuckEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_jumpDuckKey = GetPrivateProfileIntA("Movement", "JumpDuckKey", g_jumpDuckKey, configPath.c_str());
    g_jumpScoutEnabled = GetPrivateProfileIntA("Movement", "JumpScout", g_jumpScoutEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_antiCaptureEnabled = GetPrivateProfileIntA("Misc", "AntiCapture", g_antiCaptureEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_aimbotEnabled = GetPrivateProfileIntA("Combat", "AimbotEnabled", g_aimbotEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_triggerEnabled = GetPrivateProfileIntA("Combat", "TriggerEnabled", g_triggerEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_rcsEnabled = GetPrivateProfileIntA("Combat", "RcsEnabled", g_rcsEnabled ? 1 : 0, configPath.c_str()) != 0;
    {
        int v = GetPrivateProfileIntA("Combat", "RcsStrength", g_rcsStrength, configPath.c_str());
        if (v < 0) v = 0; if (v > 100) v = 100; g_rcsStrength = v;
    }
    g_autoStopEnabled = GetPrivateProfileIntA("Combat", "AutoStop", g_autoStopEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_autoKnifeEnabled = GetPrivateProfileIntA("Combat", "AutoKnife", g_autoKnifeEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_bhopEnabled = GetPrivateProfileIntA("Misc", "BhopEnabled", g_bhopEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_antiFlashEnabled = GetPrivateProfileIntA("Misc", "AntiFlash", g_antiFlashEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_noSmokeEnabled = GetPrivateProfileIntA("Misc", "NoSmoke", g_noSmokeEnabled ? 1 : 0, configPath.c_str()) != 0;
    
    // Загрузка значений слайдеров
    GetPrivateProfileStringA("Visuals", "OofRadius", "150.0", buf, sizeof(buf), configPath.c_str()); g_oofArrowsRadius = (float)atof(buf);
    GetPrivateProfileStringA("Visuals", "OofSize", "15.0", buf, sizeof(buf), configPath.c_str()); g_oofArrowsSize = (float)atof(buf);
    
    // (Menu.Color legacy ключ удалён — используется g_uiAccentColor)

    // ---- Cyber UI theme ----
    auto clamp01 = [](float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); };
    auto loadVec4 = [&](const char* key, ImVec4& out) {
        GetPrivateProfileStringA("UI", key, "", buf, sizeof(buf), configPath.c_str());
        if (buf[0] == '\0') return;
        float r, g, b, a;
        if (sscanf_s(buf, "%f,%f,%f,%f", &r, &g, &b, &a) == 4) {
            out = ImVec4(clamp01(r), clamp01(g), clamp01(b), clamp01(a));
        }
    };
    loadVec4("AccentColor", g_uiAccentColor);
    loadVec4("AccentSecondary", g_uiAccentSecondary);
    g_uiScanlineEnabled  = GetPrivateProfileIntA("UI", "Scanline",  g_uiScanlineEnabled  ? 1 : 0, configPath.c_str()) != 0;
    g_uiHoverGlowEnabled = GetPrivateProfileIntA("UI", "HoverGlow", g_uiHoverGlowEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_uiAnimationsEnabled = GetPrivateProfileIntA("UI", "Animations", g_uiAnimationsEnabled ? 1 : 0, configPath.c_str()) != 0;
    g_uiParticlesEnabled  = GetPrivateProfileIntA("UI", "Particles",  g_uiParticlesEnabled  ? 1 : 0, configPath.c_str()) != 0;
    GetPrivateProfileStringA("UI", "ScanlineIntensity", "0.5", buf, sizeof(buf), configPath.c_str());
    g_uiScanlineIntensity = clamp01((float)atof(buf));
}

} // namespace config
