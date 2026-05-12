#ifndef MEMORY_OFFSETS_HPP
#define MEMORY_OFFSETS_HPP

#include <cstdint>
#include <string>

// Memory module: Runtime offsets management
// This file contains the RuntimeOffsets structure and loading logic

namespace memory {

struct RuntimeOffsets
{
    // Base offsets
    std::uint32_t dwEntityList;
    std::uint32_t dwViewMatrix;
    std::uint32_t dwViewAngles;
    std::uint32_t dwLocalPlayerController;
    std::uint32_t dwLocalPlayerPawn;
    std::uint32_t dwGlobalVars;
    std::uint32_t dwCSGOInput;
    std::uint32_t dwPlantedC4;
    std::uint32_t dwForceAttack;
    std::uint32_t dwForceJump;
    std::uint32_t dwForceLeft;
    std::uint32_t dwForceRight;

    // Schema offsets - C_BaseEntity
    std::uint32_t m_iHealth;
    std::uint32_t m_iTeamNum;
    std::uint32_t m_vecAbsOrigin;
    std::uint32_t m_pGameSceneNode;
    std::uint32_t m_vecVelocity;
    std::uint32_t m_fFlags;
    std::uint32_t m_pCollision;
    
    // C_BasePlayerPawn
    std::uint32_t m_pObserverServices;
    std::uint32_t m_pWeaponServices;
    std::uint32_t m_pCameraServices;
    std::uint32_t m_vOldOrigin;
    
    // C_CSPlayerPawn
    std::uint32_t m_iShotsFired;
    std::uint32_t m_pAimPunchServices;
    std::uint32_t m_aimPunchAngle;
    std::uint32_t m_aimPunchAngleVel;
    std::uint32_t m_bIsScoped;
    std::uint32_t m_entitySpottedState;
    std::uint32_t m_pBulletServices;

    // CCSPlayer_BulletServices
    std::uint32_t m_totalHitsOnServer;
    
    // C_CSPlayerPawnBase
    std::uint32_t m_flFlashDuration;
    std::uint32_t m_flFlashBangTime;
    
    // C_BaseModelEntity
    std::uint32_t m_vecViewOffset;
    std::uint32_t m_Glow;
    
    // Observer
    std::uint32_t m_hObserverTarget;
    
    // Controller
    std::uint32_t m_hObserverPawn;
    std::uint32_t m_hPlayerPawn;
    std::uint32_t m_iszPlayerName;
    
    // Weapon services
    std::uint32_t m_hActiveWeapon;
    
    // Econ
    std::uint32_t m_iItemDefinitionIndex;
    
    // Collision
    std::uint32_t m_vecMins;
    std::uint32_t m_vecMaxs;
    
    // Camera services
    std::uint32_t m_iFOV;
    std::uint32_t m_iFOVStart;
    std::uint32_t m_flFOVTime;
    std::uint32_t m_flFOVRate;
    
    // Skeleton
    std::uint32_t m_modelState;
    
    // C_PlantedC4 (bomb)
    std::uint32_t m_bBombTicking;
    std::uint32_t m_bBombDefused;
    std::uint32_t m_flC4Blow;
    std::uint32_t m_bBeingDefused;
    
    // C_CSPlayerPawn (skin changer)
    std::uint32_t m_hHudModelArms;

    // Load offsets from JSON files
    bool LoadFromJson(const std::string& offsetsJsonPath, const std::string& schemaJsonPath);
};

// Get singleton instance of RuntimeOffsets
RuntimeOffsets& GetOffsets();

// Initialize offsets from default paths
void InitializeOffsets();

} // namespace memory

#endif // MEMORY_OFFSETS_HPP
