#include "skins_config.hpp"

namespace skins {

// Global skin configuration instance
static SkinConfig g_skinConfig;

SkinConfig& GetSkinConfig()
{
	return g_skinConfig;
}

void InitializeSkinConfig()
{
	auto& config = GetSkinConfig();
	
	// Initialize all weapons with Default (0) paint kit
	// User selects skins in menu, they are saved in config
	config.weaponSkins[1] = 0;   // Desert Eagle - Default
	config.weaponSkins[4] = 0;   // Glock-18 - Default
	config.weaponSkins[7] = 0;   // AK-47 - Default
	config.weaponSkins[9] = 0;   // AWP - Default
	config.weaponSkins[10] = 0;  // FAMAS - Default
	config.weaponSkins[13] = 0;  // Galil-AR - Default
	config.weaponSkins[16] = 0;  // M4A4 - Default
	config.weaponSkins[30] = 0;  // TEC-9 - Default
	config.weaponSkins[40] = 0;  // SSG-08 - Default
	config.weaponSkins[60] = 0;  // M4A1-S - Default
	config.weaponSkins[61] = 0;  // USP-S - Default
	
	// Initialize other fields
	config.lastActiveWeapon = 0;
	config.lastSkinUpdateTick = 0;
	config.skinIdCounter = 0;
	config.skinUpdateCounter = 0;
	config.skinRevision = 1;
	config.lastSkinUpdateCounterSeen = 0;
}

int GetPaintKitForWeapon(int defIndex)
{
	auto& config = GetSkinConfig();
	auto it = config.weaponSkins.find(defIndex);
	if (it != config.weaponSkins.end())
		return it->second;
	return 0; // No skin
}

void SetPaintKitForWeapon(int defIndex, int paintKit)
{
	auto& config = GetSkinConfig();
	config.weaponSkins[defIndex] = paintKit;
	config.skinUpdateCounter++;
}

} // namespace skins
