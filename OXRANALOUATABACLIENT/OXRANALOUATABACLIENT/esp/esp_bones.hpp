#pragma once

// Forward declarations
struct ImDrawList;

namespace esp {

// Update bones cache from game memory
void UpdateBonesCache(int width, int height);

// Render skeleton bones
void RenderBones(const float view[16], int width, int height, ImDrawList* drawList);

} // namespace esp
