#include "skins_apply.hpp"
#include <Windows.h>
#include "../memory/memory_offsets.hpp"
#include "../memory/memory_client.hpp"
#include "../memory/memory_utils.hpp"
#include "../config/config_vars.hpp"
#include "../../../output/client_dll.hpp"
#include <map>

#include "skins_config.hpp"

namespace skins {

// Переменные для Skin Changer
static std::map<uintptr_t, int> g_appliedSkins;
static uint32_t g_lastActiveWeapon = 0;
static uint64_t g_lastSkinUpdateTick = 0;
static int32_t g_skinIdCounter = 0;

// Триггер для реал-тайм обновления скинов
int g_skinUpdateCounter = 0;
static int g_skinRevision = 1;
static int g_lastSkinUpdateCounterSeen = 0;

void InitSkinConfig()
{
    // Function moved to skins_config.cpp
}

static std::map<uintptr_t, uint32_t> g_generatedIDs;
static std::map<uintptr_t, int> g_appliedKits;

static bool TryReadEconAttributesVec(uintptr_t itemView, bool dynamicList, uintptr_t& outPtr, uint64_t& outSize)
{
    using namespace cs2_dumper::schemas::client_dll;
    uintptr_t baseList = itemView + (dynamicList ? C_EconItemView::m_NetworkedDynamicAttributes : C_EconItemView::m_AttributeList);
    uintptr_t attributesVec = baseList + CAttributeList::m_Attributes;

    uint8_t blob[0x40] = {};
    if (memory::TryRead<uint8_t>(attributesVec, blob[0]) == false)
        return false;
    for (size_t i = 1; i < sizeof(blob); ++i)
        (void)memory::TryRead<uint8_t>(attributesVec + i, blob[i]);

    auto tryCandidate = [&](size_t sizeOff, bool size32, size_t ptrOff, const char* tag) -> bool {
        uint64_t size = 0;
        uintptr_t ptr = 0;
        if (size32) {
            uint32_t s32 = *(uint32_t*)(blob + sizeOff);
            size = (uint64_t)s32;
        } else {
            size = *(uint64_t*)(blob + sizeOff);
        }
        ptr = *(uintptr_t*)(blob + ptrOff);

        if (size == 0 || size > 256) return false;
        if (ptr < 0x10000) return false;

        uint16_t def = 0;
        if (!memory::TryRead<uint16_t>(ptr + CEconItemAttribute::m_iAttributeDefinitionIndex, def))
            return false;

        outPtr = ptr;
        outSize = size;
        return true;
    };

    if (tryCandidate(0x0, false, 0x8, "A0")) return true;
    if (tryCandidate(0x0, true, 0x8, "B0")) return true;
    if (tryCandidate(0x8, false, 0x0, "A1")) return true;
    if (tryCandidate(0x8, true, 0x0, "B1")) return true;
    if (tryCandidate(0x10, true, 0x0, "B2")) return true;
    if (tryCandidate(0x8, true, 0x0, "V0")) return true;
    if (tryCandidate(0x10, true, 0x0, "V1")) return true;

    return false;
}

static bool TrySetEconAttributeFloatInList(uintptr_t itemView, bool dynamicList, uint16_t attributeDefIndex, float value, bool& outFoundDef)
{
    using namespace cs2_dumper::schemas::client_dll;
    uintptr_t ptr = 0;
    uint64_t size = 0;
    if (!TryReadEconAttributesVec(itemView, dynamicList, ptr, size))
        return false;

    if (size == 0 || !ptr) return false;
    if (size > 64) return false;

    for (uint64_t i = 0; i < size; ++i)
    {
        uintptr_t attr = ptr + (i * 0x48);
        uint16_t def = 0;
        if (!memory::TryRead<uint16_t>(attr + CEconItemAttribute::m_iAttributeDefinitionIndex, def))
            continue;
        if (def != attributeDefIndex)
            continue;

        outFoundDef = true;
        (void)memory::TryWrite<float>(attr + CEconItemAttribute::m_flValue, value);
        (void)memory::TryWrite<float>(attr + CEconItemAttribute::m_flInitialValue, value);
        return true;
    }

    return false;
}

static bool TrySetEconAttributeFloat(uintptr_t itemView, uint16_t attributeDefIndex, float value)
{
    bool foundDefA = false;
    bool foundDefB = false;
    bool okA = TrySetEconAttributeFloatInList(itemView, false, attributeDefIndex, value, foundDefA);
    bool okB = TrySetEconAttributeFloatInList(itemView, true, attributeDefIndex, value, foundDefB);
    return okA || okB;
}

void UpdateSkinChangerHooked()
{
    uintptr_t client = memory::GetClientBase();
    if (!client) return;

    using namespace cs2_dumper::schemas::client_dll;

    uintptr_t localPawn = 0;
    if (!memory::TryRead<uintptr_t>(client + memory::GetOffsets().dwLocalPlayerPawn, localPawn) || !localPawn) return;

    uintptr_t weaponServices = 0;
    if (!memory::TryRead<uintptr_t>(localPawn + C_BasePlayerPawn::m_pWeaponServices, weaponServices) || !weaponServices) return;

    uintptr_t entityList = 0;
    if (!memory::TryRead<uintptr_t>(client + memory::GetOffsets().dwEntityList, entityList) || !entityList) return;

    uintptr_t hMyWeapons = weaponServices + CPlayer_WeaponServices::m_hMyWeapons;
    int weaponCount = 0;
    memory::TryRead<int>(hMyWeapons, weaponCount);
    uintptr_t pElements = 0;
    memory::TryRead<uintptr_t>(hMyWeapons + 0x8, pElements);
    
    if (weaponCount <= 0 || weaponCount > 64 || !pElements) return;
    
    uint32_t hActiveWeapon = 0;
    memory::TryRead<uint32_t>(weaponServices + CPlayer_WeaponServices::m_hActiveWeapon, hActiveWeapon);

    static int lastRevision = 0;
    bool menuChanged = (g_skinRevision != lastRevision);
    if (menuChanged) lastRevision = g_skinRevision;

    for (int i = 0; i < weaponCount; ++i) {
        uint32_t hWeapon = 0;
        if (!memory::TryRead<uint32_t>(pElements + (i * sizeof(uint32_t)), hWeapon)) continue;
        if (!hWeapon || hWeapon == 0xFFFFFFFF) continue;

        int wIdx = hWeapon & 0x1FF;
        int wEntry = (hWeapon & 0x7FFF) >> 9;
        uintptr_t wListEntry = 0;
        if (!memory::TryRead<uintptr_t>(entityList + 0x8 * wEntry + 0x10, wListEntry) || !wListEntry) continue;
        
        uintptr_t weaponEnt = 0;
        if (!memory::TryRead<uintptr_t>(wListEntry + 0x70 * wIdx, weaponEnt) || !weaponEnt) continue;

        uintptr_t itemView = weaponEnt + C_EconEntity::m_AttributeManager + C_AttributeContainer::m_Item;
        
        uint16_t defIndex = 0;
        if (!memory::TryRead<uint16_t>(itemView + C_EconItemView::m_iItemDefinitionIndex, defIndex)) continue;

        int targetPaintKit = GetPaintKitForWeapon(defIndex);
        if (targetPaintKit <= 0) continue;

        bool needUpdate = (g_appliedKits[weaponEnt] != targetPaintKit) || menuChanged || (g_generatedIDs[weaponEnt] == 0);

        if (needUpdate) {
            uint32_t newHighID = 1;
            
            g_generatedIDs[weaponEnt] = newHighID;
            g_appliedKits[weaponEnt] = targetPaintKit;

            (void)memory::TryWrite<uint32_t>(itemView + C_EconItemView::m_iAccountID, 0);
            (void)memory::TryWrite<uint32_t>(weaponEnt + C_EconEntity::m_OriginalOwnerXuidLow, 0);
            (void)memory::TryWrite<uint32_t>(weaponEnt + C_EconEntity::m_OriginalOwnerXuidHigh, 0);

            (void)memory::TryWrite<uint32_t>(itemView + C_EconItemView::m_iItemIDHigh, newHighID);
            (void)memory::TryWrite<uint32_t>(itemView + C_EconItemView::m_iItemIDLow, 0);
            
            (void)memory::TryWrite<int32_t>(weaponEnt + C_EconEntity::m_nFallbackPaintKit, targetPaintKit);
            (void)memory::TryWrite<int32_t>(weaponEnt + C_EconEntity::m_nFallbackSeed, 1);
            (void)memory::TryWrite<float>(weaponEnt + C_EconEntity::m_flFallbackWear, 0.001f);
            
            (void)memory::TryWrite<int32_t>(weaponEnt + C_EconEntity::m_nFallbackStatTrak, -1);
            
            (void)memory::TryWrite<int32_t>(itemView + C_EconItemView::m_iEntityQuality, 4);
            
            (void)TrySetEconAttributeFloat(itemView, 6, (float)targetPaintKit);
            (void)TrySetEconAttributeFloat(itemView, 7, 1.0f);
            (void)TrySetEconAttributeFloat(itemView, 8, 0.001f);
            
            (void)memory::TryWrite<char>(itemView + C_EconItemView::m_szCustomName, 0);
        }
    }
}

void __fastcall hkFrameStageNotify(void* rcx, int curStage) {
    if (g_skinUpdateCounter != g_lastSkinUpdateCounterSeen) {
        g_lastSkinUpdateCounterSeen = g_skinUpdateCounter;
        ++g_skinRevision;
        if (g_skinRevision <= 0) g_skinRevision = 1;
    }
    
    // config::g_skinChangerEnabled isn't easily accessible, so we assume enabled for now
    // Actually we can add it to config_vars.hpp
    if ((curStage == 6)) 
    {
        UpdateSkinChangerHooked();
    }
    
    // oFrameStageNotify should be called
    typedef void(__fastcall* FrameStageNotify_t)(void* rcx, int curStage);
    extern FrameStageNotify_t oFrameStageNotify;
    if (oFrameStageNotify)
        oFrameStageNotify(rcx, curStage);
}

} // namespace skins
