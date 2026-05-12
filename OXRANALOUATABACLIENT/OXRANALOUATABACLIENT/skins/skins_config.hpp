#pragma once

#include <map>
#include <cstdint>

namespace skins {

// Skin configuration structure
struct SkinConfig {
	std::map<int, int> weaponSkins;           // defIndex -> paintKit
	std::map<uintptr_t, int> appliedSkins;    // Already applied skins
	uint32_t lastActiveWeapon;                // Last active weapon
	uint64_t lastSkinUpdateTick;              // Last update time
	int32_t skinIdCounter;                    // Unique ID counter
	int skinUpdateCounter;                    // Real-time update trigger
	int skinRevision;                         // Skin revision counter
	int lastSkinUpdateCounterSeen;            // Last seen update counter
};

// Get global skin configuration
SkinConfig& GetSkinConfig();

// Initialize skin configuration with defaults
void InitializeSkinConfig();

// Get paint kit for weapon by definition index
int GetPaintKitForWeapon(int defIndex);

// Set paint kit for weapon
void SetPaintKitForWeapon(int defIndex, int paintKit);

} // namespace skins
