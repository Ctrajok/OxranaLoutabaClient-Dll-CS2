#pragma once

#include "esp_common.hpp"
#include <vector>
#include <cstdint>

// Forward declarations
struct ImDrawList;

namespace esp {

struct DamageText
{
    uintptr_t targetPawn;
    Vec3 worldPos;
    int damage;
    unsigned long long spawnTime;
    float alpha;
    float currentYOffset;
    float targetYOffset;
};

// Get damage texts accessor
std::vector<DamageText>& GetDamageTexts();

// Add damage indicator
void AddDamageIndicator(uintptr_t targetPawn, const Vec3& worldPos, int damage);

// Render damage indicators
void RenderDamageIndicators(ImDrawList* drawList);

} // namespace esp
