#pragma once

#include "esp_common.hpp"

// Forward declarations
struct ImDrawList;

namespace esp {

// Bomb data structure
struct BombData {
    Vec3 pos;
    float blowTime;
    bool isDefusing;
    bool found;
};

// Get bomb data accessor
BombData& GetBombData();

// Render bomb ESP (C4 indicator)
void RenderBombEsp(ImDrawList* drawList);

} // namespace esp
