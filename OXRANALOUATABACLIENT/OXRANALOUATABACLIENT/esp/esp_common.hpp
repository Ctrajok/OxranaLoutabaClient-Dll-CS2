#pragma once

#include <vector>
#include <string>
#include <cstdint>

// Vec2 and Vec3 definitions (needed for ESP structures)
struct Vec2 {
    float x, y;
};

struct Vec3 {
    float x, y, z;
};

namespace esp {

// Box2D structure (2D screen space)
struct Box2D {
    float left, top, right, bottom;
};

// ESP Box structure (2D screen space)
struct EspBox
{
    Box2D box;
    int hp;
    uintptr_t pawn;
    char name[64];
    char weapon[64];
    int weaponIconIndex;
    bool flashed;
    float flashDur;
    float flashEndTime;
    float distance;
};

// ESP Target structure (3D world space)
struct EspTargetWorld
{
    uintptr_t pawn;
    int hp;
    Vec3 origin;
    Vec3 mins;
    Vec3 maxs;
    char name[64];
    char weapon[64];
    int weaponIconIndex;
    bool flashed;
    float flashDur;
    float flashEndTime;
    float distance;
};

// Cached bone data for skeleton rendering
struct CachedBoneWorld
{
    Vec3 world;
    bool valid;
};

struct BoneCacheEntryWorld
{
    uintptr_t pawn;
    CachedBoneWorld b[32];
};

// MST-граф скелета: пары индексов b[i], b[j] которые надо соединить.
// Строится UpdateBonesCache через Kruskal MST по всем валидным костям —
// устойчиво к перестановке индексов костей между моделями CS2.
struct BoneSkeleton
{
    uintptr_t pawn;
    uint8_t   edges[32][2]; // пары (a, b), 0xFF = конец массива
    uint8_t   edgeCount;
};

// Accessor functions for ESP data
std::vector<EspBox>& GetEspBoxes();
std::vector<EspTargetWorld>& GetEspTargetsWorld();
std::vector<BoneCacheEntryWorld>& GetBoneCache();
std::vector<BoneSkeleton>& GetBoneSkeletons();
std::vector<std::string>& GetSpectators();

// Collect ESP data from game memory
void CollectEspData(int width, int height);

// Project 3D world targets to 2D screen boxes
void ProjectEspBoxes(const float view[16], int width, int height);

// Convert world position to screen position
bool WorldToScreen(const float m[16], const Vec3& pos, int width, int height, Vec2& out);

// Helper function to get estimated game time
float GetEstimatedGameTimeSeconds();

} // namespace esp
