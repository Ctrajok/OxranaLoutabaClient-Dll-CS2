#pragma once
#include <cstdint>

// Forward declarations - actual definitions in cs2_internal_dll.cpp
struct Vec3;
struct QAngle;

namespace combat {

// Calculate aim angles from source to destination position
QAngle CalcAngle(Vec3 src, Vec3 dst);

// Calculate field of view between view angles and aim angles
float GetFov(QAngle viewAngles, QAngle aimAngles);

// Get bone position from pawn entity
Vec3 GetBonePos(uintptr_t pawn, int boneIndex);

// Clamp angles to valid ranges
void ClampAngles(QAngle& angles);

} // namespace combat
