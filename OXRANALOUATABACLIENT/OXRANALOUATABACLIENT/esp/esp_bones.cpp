#include "esp_bones.hpp"
#include "esp_common.hpp"
#include "../config/config_vars.hpp"
#include "../memory/memory_utils.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_offsets.hpp"

#include "../../imgui-1.92.5/imgui.h"
#include "../../../output/client_dll.hpp"

#include <algorithm>
#include <cmath>

// =====================================================================
// Bones ESP — подход "скелет через MST".
//
// Проблема с CS2 Animgraph 2 (апрель 2026): индексы костей в
// m_modelState + 0x80 больше не стабильны между моделями. Хардкод вроде
// "индекс 13 = правое плечо" сломался — на агентах 13 может быть ногой,
// получаются "мосты" между руками и ногами.
//
// Решение БЕЗ reverse-engineering: не гадать какая кость за что отвечает.
// Вместо этого строим анатомически-корректный скелет по геометрии:
//
//   1. Считываем ВСЕ 32 кости игрока. Фильтруем (не zero/NaN, близко к origin).
//   2. Строим Minimum Spanning Tree (MST) — дерево с минимальной суммой
//      расстояний между костями. Это автоматически даёт "анатомически
//      соседние" кости, потому что в реальном скелете connected bones
//      физически рядом, а несвязанные (типа левая рука + правая нога) —
//      далеко друг от друга.
//   3. Обрезаем рёбра длиннее 40 units (анатомический максимум для
//      сегмента humanoid-скелета). Это убирает "мосты" и дает чистый
//      скелет независимо от того как Valve переставил индексы.
//
// Устойчиво к перестановке индексов. Работает на всех моделях: T, CT, агенты.
// =====================================================================

namespace esp {

namespace {

constexpr int   kMaxBones            = 32;
constexpr float kMaxBoneDistFromOrigin = 120.0f;  // кость дальше = stale slot
constexpr float kMaxSegmentDist      = 40.0f;     // сегмент длиннее = "мост"

// Disjoint-Set Union для Kruskal MST.
struct DSU {
    int parent[kMaxBones];

    void Init(int n) {
        for (int i = 0; i < n; ++i) parent[i] = i;
    }
    int Find(int x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]]; // path compression
            x = parent[x];
        }
        return x;
    }
    bool Union(int a, int b) {
        int ra = Find(a), rb = Find(b);
        if (ra == rb) return false;
        parent[ra] = rb;
        return true;
    }
};

struct Edge {
    float  distSq;
    uint8_t a, b;
};

// Строим MST + обрезаем длинные рёбра. Результат — BoneSkeleton с edges[].
static BoneSkeleton BuildSkeletonMST(const BoneCacheEntryWorld& bones)
{
    BoneSkeleton skel{};
    skel.pawn = bones.pawn;
    skel.edgeCount = 0;

    // 1. Собираем индексы валидных костей.
    int validIdx[kMaxBones];
    int validCount = 0;
    for (int i = 0; i < kMaxBones; ++i) {
        if (bones.b[i].valid) {
            validIdx[validCount++] = i;
        }
    }
    if (validCount < 2) return skel;

    // 2. Все рёбра между парами валидных костей (O(n^2), n <= 32 → ≤496 рёбер).
    Edge edges[kMaxBones * kMaxBones / 2];
    int edgeCount = 0;
    const float maxSegSq = kMaxSegmentDist * kMaxSegmentDist;

    for (int i = 0; i < validCount; ++i) {
        for (int j = i + 1; j < validCount; ++j) {
            const Vec3& a = bones.b[validIdx[i]].world;
            const Vec3& b = bones.b[validIdx[j]].world;
            float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
            float d2 = dx * dx + dy * dy + dz * dz;
            // Только рёбра короче max anatomical distance — это уже обрезка.
            if (d2 > maxSegSq) continue;
            edges[edgeCount].distSq = d2;
            edges[edgeCount].a = (uint8_t)validIdx[i];
            edges[edgeCount].b = (uint8_t)validIdx[j];
            ++edgeCount;
        }
    }

    // 3. Сортируем рёбра по длине (ascending).
    std::sort(edges, edges + edgeCount, [](const Edge& x, const Edge& y) {
        return x.distSq < y.distSq;
    });

    // 4. Kruskal MST: берём рёбра от коротких к длинным, пропускаем циклы.
    //    Ограничиваем степень вершины — кость не может иметь больше 4 соседей
    //    в humanoid скелете (голова-1, шея-3 спина+плечи, спина-3, таз-3).
    DSU dsu;
    dsu.Init(kMaxBones);
    uint8_t degree[kMaxBones] = {0};
    constexpr uint8_t kMaxDegree = 4;

    for (int i = 0; i < edgeCount && skel.edgeCount < 32; ++i) {
        uint8_t a = edges[i].a, b = edges[i].b;
        if (degree[a] >= kMaxDegree || degree[b] >= kMaxDegree) continue;
        if (!dsu.Union(a, b)) continue; // ребро создаст цикл

        skel.edges[skel.edgeCount][0] = a;
        skel.edges[skel.edgeCount][1] = b;
        ++skel.edgeCount;
        ++degree[a];
        ++degree[b];
    }

    return skel;
}

} // namespace

// ---------------------------------------------------------------------
// Чтение всех 32 костей + построение skeleton MST
// ---------------------------------------------------------------------
void UpdateBonesCache(int width, int height)
{
    (void)width; (void)height;
    uintptr_t client = memory::GetClientBase();
    if (!client) return;

    using namespace cs2_dumper::schemas::client_dll;

    // Временные буферы используются ВНЕ __try (vector нельзя трогать внутри SEH).
    BoneCacheEntryWorld tempBones[64] = {};
    BoneSkeleton        tempSkels[64] = {};
    int count = 0;

    __try
    {
        auto& espTargets = GetEspTargetsWorld();

        for (const EspTargetWorld& e : espTargets)
        {
            if (count >= 64) break;
            uintptr_t pawn = e.pawn;
            if (!pawn) continue;

            uintptr_t gameScene = 0;
            if (!memory::TryRead<uintptr_t>(pawn + C_BaseEntity::m_pGameSceneNode, gameScene) || !gameScene) continue;

            uintptr_t boneMatrix = 0;
            if (!memory::TryRead<uintptr_t>(gameScene + CSkeletonInstance::m_modelState + 0x80, boneMatrix) || !boneMatrix) continue;

            BoneCacheEntryWorld entry{};
            entry.pawn = pawn;
            for (auto& cb : entry.b) cb.valid = false;

            const Vec3& origin = e.origin;
            const float maxDistSq = kMaxBoneDistFromOrigin * kMaxBoneDistFromOrigin;

            // Читаем ВСЕ 32 кости. Каждая кость в matrix = 32 байта (3х4 matrix + quat).
            for (int boneId = 0; boneId < kMaxBones; ++boneId)
            {
                Vec3 p3{};
                if (!memory::TryRead<float>(boneMatrix + boneId * 0x20 + 0x0, p3.x)) continue;
                if (!memory::TryRead<float>(boneMatrix + boneId * 0x20 + 0x4, p3.y)) continue;
                if (!memory::TryRead<float>(boneMatrix + boneId * 0x20 + 0x8, p3.z)) continue;

                // Отсев невалидных значений.
                if (p3.x == 0.0f && p3.y == 0.0f && p3.z == 0.0f) continue;
                if (!std::isfinite(p3.x) || !std::isfinite(p3.y) || !std::isfinite(p3.z)) continue;

                float dx = p3.x - origin.x, dy = p3.y - origin.y, dz = p3.z - origin.z;
                if ((dx * dx + dy * dy + dz * dz) > maxDistSq) continue;

                entry.b[boneId].world = p3;
                entry.b[boneId].valid = true;
            }

            tempBones[count] = entry;
            tempSkels[count] = BuildSkeletonMST(entry);
            ++count;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}

    // Перенос в глобальные std::vector — вне SEH.
    auto& bonesOut = GetBoneCache();
    auto& skelsOut = GetBoneSkeletons();
    bonesOut.clear();
    skelsOut.clear();
    for (int i = 0; i < count; ++i) {
        bonesOut.push_back(tempBones[i]);
        skelsOut.push_back(tempSkels[i]);
    }
}

// ---------------------------------------------------------------------
// Рендер по скелету из MST
// ---------------------------------------------------------------------
void RenderBones(const float view[16], int width, int height, ImDrawList* drawList)
{
    if (!config::g_bonesEnabled || !drawList)
        return;

    const ImU32 col = ImGui::ColorConvertFloat4ToU32(config::g_boneColor);

    const float margin = 256.0f;
    const float minX = -margin, minY = -margin;
    const float maxX = (float)width + margin, maxY = (float)height + margin;

    auto& bones = GetBoneCache();
    auto& skels = GetBoneSkeletons();

    // Итерируем параллельно — skels[i] строился из bones[i].
    const size_t n = (bones.size() < skels.size()) ? bones.size() : skels.size();
    for (size_t i = 0; i < n; ++i)
    {
        const BoneCacheEntryWorld& be = bones[i];
        const BoneSkeleton&        sk = skels[i];

        for (uint8_t e = 0; e < sk.edgeCount; ++e)
        {
            uint8_t ai = sk.edges[e][0];
            uint8_t bi = sk.edges[e][1];
            if (ai >= kMaxBones || bi >= kMaxBones) continue;
            if (!be.b[ai].valid || !be.b[bi].valid) continue;

            const Vec3& wa = be.b[ai].world;
            const Vec3& wb = be.b[bi].world;

            Vec2 pa{}, pb{};
            if (!WorldToScreen(view, wa, width, height, pa)) continue;
            if (!WorldToScreen(view, wb, width, height, pb)) continue;

            if (pa.x < minX || pa.x > maxX || pa.y < minY || pa.y > maxY) continue;
            if (pb.x < minX || pb.x > maxX || pb.y < minY || pb.y > maxY) continue;

            drawList->AddLine(ImVec2(pa.x, pa.y), ImVec2(pb.x, pb.y), col, 1.5f);
        }
    }
}

} // namespace esp
