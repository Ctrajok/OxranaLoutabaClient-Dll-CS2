// Memory module: Runtime offsets management
// Implementation file

#include "memory_offsets.hpp"
#include "../config/config_parser.hpp"
#include "../../output/offsets.hpp"
#include "../../output/client_dll.hpp"
#include "../../output/buttons.hpp"

namespace memory {

// Singleton instance
static RuntimeOffsets g_offsetsRuntime{};

RuntimeOffsets& GetOffsets()
{
    return g_offsetsRuntime;
}

void InitializeOffsets()
{
    // Initialize with default values from cs2_dumper
    using namespace cs2_dumper;
    
    g_offsetsRuntime.dwEntityList = offsets::client_dll::dwEntityList;
    g_offsetsRuntime.dwViewMatrix = offsets::client_dll::dwViewMatrix;
    g_offsetsRuntime.dwViewAngles = offsets::client_dll::dwViewAngles;
    g_offsetsRuntime.dwLocalPlayerController = offsets::client_dll::dwLocalPlayerController;
    g_offsetsRuntime.dwLocalPlayerPawn = offsets::client_dll::dwLocalPlayerPawn;
    g_offsetsRuntime.dwGlobalVars = offsets::client_dll::dwGlobalVars;
    g_offsetsRuntime.dwCSGOInput = offsets::client_dll::dwCSGOInput;
    g_offsetsRuntime.dwPlantedC4 = offsets::client_dll::dwPlantedC4;
    g_offsetsRuntime.dwForceAttack = buttons::attack;
    g_offsetsRuntime.dwForceJump = buttons::jump;
    g_offsetsRuntime.dwForceLeft = buttons::left;
    g_offsetsRuntime.dwForceRight = buttons::right;

    // Schema offsets
    using namespace schemas::client_dll;
    
    g_offsetsRuntime.m_iHealth = C_BaseEntity::m_iHealth;
    g_offsetsRuntime.m_iTeamNum = C_BaseEntity::m_iTeamNum;
    g_offsetsRuntime.m_vecAbsOrigin = CGameSceneNode::m_vecAbsOrigin;
    g_offsetsRuntime.m_pGameSceneNode = C_BaseEntity::m_pGameSceneNode;
    g_offsetsRuntime.m_vecVelocity = C_BaseEntity::m_vecVelocity;
    g_offsetsRuntime.m_fFlags = C_BaseEntity::m_fFlags;
    g_offsetsRuntime.m_pCollision = C_BaseEntity::m_pCollision;
    
    g_offsetsRuntime.m_pObserverServices = C_BasePlayerPawn::m_pObserverServices;
    g_offsetsRuntime.m_pWeaponServices = C_BasePlayerPawn::m_pWeaponServices;
    g_offsetsRuntime.m_pCameraServices = C_BasePlayerPawn::m_pCameraServices;
    g_offsetsRuntime.m_vOldOrigin = C_BasePlayerPawn::m_vOldOrigin;
    
    g_offsetsRuntime.m_iShotsFired = C_CSPlayerPawn::m_iShotsFired;
    g_offsetsRuntime.m_pAimPunchServices = C_CSPlayerPawn::m_pAimPunchServices;
    g_offsetsRuntime.m_aimPunchAngle = CCSPlayer_AimPunchServices::m_predictableBaseAngle;
    g_offsetsRuntime.m_aimPunchAngleVel = CCSPlayer_AimPunchServices::m_predictableBaseAngleVel;
    g_offsetsRuntime.m_bIsScoped = C_CSPlayerPawn::m_bIsScoped;
    g_offsetsRuntime.m_entitySpottedState = C_CSPlayerPawn::m_entitySpottedState;
    g_offsetsRuntime.m_pBulletServices = C_CSPlayerPawn::m_pBulletServices;

    // CCSPlayer_BulletServices
    g_offsetsRuntime.m_totalHitsOnServer = CCSPlayer_BulletServices::m_totalHitsOnServer;
    
    g_offsetsRuntime.m_flFlashDuration = C_CSPlayerPawnBase::m_flFlashDuration;
    g_offsetsRuntime.m_flFlashBangTime = C_CSPlayerPawnBase::m_flFlashBangTime;
    
    g_offsetsRuntime.m_vecViewOffset = C_BaseModelEntity::m_vecViewOffset;
    g_offsetsRuntime.m_Glow = C_BaseModelEntity::m_Glow;
    
    g_offsetsRuntime.m_hObserverTarget = CPlayer_ObserverServices::m_hObserverTarget;
    
    g_offsetsRuntime.m_hObserverPawn = CCSPlayerController::m_hObserverPawn;
    g_offsetsRuntime.m_hPlayerPawn = CCSPlayerController::m_hPlayerPawn;
    g_offsetsRuntime.m_iszPlayerName = CBasePlayerController::m_iszPlayerName;
    
    g_offsetsRuntime.m_hActiveWeapon = CPlayer_WeaponServices::m_hActiveWeapon;
    
    g_offsetsRuntime.m_iItemDefinitionIndex = C_EconItemView::m_iItemDefinitionIndex;
    
    g_offsetsRuntime.m_vecMins = CCollisionProperty::m_vecMins;
    g_offsetsRuntime.m_vecMaxs = CCollisionProperty::m_vecMaxs;
    
    g_offsetsRuntime.m_iFOV = CCSPlayerBase_CameraServices::m_iFOV;
    g_offsetsRuntime.m_iFOVStart = CCSPlayerBase_CameraServices::m_iFOVStart;
    g_offsetsRuntime.m_flFOVTime = CCSPlayerBase_CameraServices::m_flFOVTime;
    g_offsetsRuntime.m_flFOVRate = CCSPlayerBase_CameraServices::m_flFOVRate;
    
    g_offsetsRuntime.m_modelState = CSkeletonInstance::m_modelState;
    
    g_offsetsRuntime.m_bBombTicking = C_PlantedC4::m_bBombTicking;
    g_offsetsRuntime.m_bBombDefused = C_PlantedC4::m_bBombDefused;
    g_offsetsRuntime.m_flC4Blow = C_PlantedC4::m_flC4Blow;
    g_offsetsRuntime.m_bBeingDefused = C_PlantedC4::m_bBeingDefused;
    
    g_offsetsRuntime.m_hHudModelArms = C_CSPlayerPawn::m_hHudModelArms;
}

// LoadFromJson implementation will be added later when we refactor SimpleJsonParser
bool RuntimeOffsets::LoadFromJson(const std::string& offsetsJsonPath, const std::string& schemaJsonPath)
{
    std::map<std::string, uint32_t> offsets;
    if (SimpleJsonParser::LoadOffsets(offsetsJsonPath, offsets)) {
        if (offsets.count("dwEntityList")) dwEntityList = offsets["dwEntityList"];
        if (offsets.count("dwViewMatrix")) dwViewMatrix = offsets["dwViewMatrix"];
        if (offsets.count("dwViewAngles")) dwViewAngles = offsets["dwViewAngles"];
        if (offsets.count("dwLocalPlayerController")) dwLocalPlayerController = offsets["dwLocalPlayerController"];
        if (offsets.count("dwLocalPlayerPawn")) dwLocalPlayerPawn = offsets["dwLocalPlayerPawn"];
        if (offsets.count("dwGlobalVars")) dwGlobalVars = offsets["dwGlobalVars"];
        if (offsets.count("dwCSGOInput")) dwCSGOInput = offsets["dwCSGOInput"];
        if (offsets.count("dwPlantedC4")) dwPlantedC4 = offsets["dwPlantedC4"];
    }

    // C_BaseEntity
    std::map<std::string, uint32_t> schema;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "C_BaseEntity", schema)) {
        if (schema.count("m_iHealth")) m_iHealth = schema["m_iHealth"];
        if (schema.count("m_iTeamNum")) m_iTeamNum = schema["m_iTeamNum"];
        if (schema.count("m_pGameSceneNode")) m_pGameSceneNode = schema["m_pGameSceneNode"];
        if (schema.count("m_vecVelocity")) m_vecVelocity = schema["m_vecVelocity"];
        if (schema.count("m_fFlags")) m_fFlags = schema["m_fFlags"];
        if (schema.count("m_pCollision")) m_pCollision = schema["m_pCollision"];
    }
    // CGameSceneNode
    std::map<std::string, uint32_t> schemaScene;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "CGameSceneNode", schemaScene)) {
        if (schemaScene.count("m_vecAbsOrigin")) m_vecAbsOrigin = schemaScene["m_vecAbsOrigin"];
    }
    // C_BasePlayerPawn
    std::map<std::string, uint32_t> schemaPawn;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "C_BasePlayerPawn", schemaPawn)) {
        if (schemaPawn.count("m_pObserverServices")) m_pObserverServices = schemaPawn["m_pObserverServices"];
        if (schemaPawn.count("m_pWeaponServices")) m_pWeaponServices = schemaPawn["m_pWeaponServices"];
        if (schemaPawn.count("m_pCameraServices")) m_pCameraServices = schemaPawn["m_pCameraServices"];
        if (schemaPawn.count("m_vOldOrigin")) m_vOldOrigin = schemaPawn["m_vOldOrigin"];
    }
    // Buttons
    std::string buttonsJsonPath = offsetsJsonPath;
    size_t lastSlash = buttonsJsonPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        buttonsJsonPath = buttonsJsonPath.substr(0, lastSlash + 1) + "buttons.json";
    } else {
        buttonsJsonPath = "buttons.json";
    }
    std::map<std::string, uint32_t> buttons;
    if (SimpleJsonParser::LoadButtons(buttonsJsonPath, buttons)) {
        if (buttons.count("attack")) dwForceAttack = buttons["attack"];
        if (buttons.count("jump")) dwForceJump = buttons["jump"];
        if (buttons.count("left")) dwForceLeft = buttons["left"];
        if (buttons.count("right")) dwForceRight = buttons["right"];
    }

    // C_CSPlayerPawn
    std::map<std::string, uint32_t> schemaCSPawn;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "C_CSPlayerPawn", schemaCSPawn)) {
        if (schemaCSPawn.count("m_iShotsFired")) m_iShotsFired = schemaCSPawn["m_iShotsFired"];
        if (schemaCSPawn.count("m_pAimPunchServices")) m_pAimPunchServices = schemaCSPawn["m_pAimPunchServices"];
        if (schemaCSPawn.count("m_bIsScoped")) m_bIsScoped = schemaCSPawn["m_bIsScoped"];
        if (schemaCSPawn.count("m_entitySpottedState")) m_entitySpottedState = schemaCSPawn["m_entitySpottedState"];
        if (schemaCSPawn.count("m_hHudModelArms")) m_hHudModelArms = schemaCSPawn["m_hHudModelArms"];
        if (schemaCSPawn.count("m_pBulletServices")) m_pBulletServices = schemaCSPawn["m_pBulletServices"];
    }
    // CCSPlayer_BulletServices
    std::map<std::string, uint32_t> schemaBullet;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "CCSPlayer_BulletServices", schemaBullet)) {
        if (schemaBullet.count("m_totalHitsOnServer")) m_totalHitsOnServer = schemaBullet["m_totalHitsOnServer"];
    }
    // CCSPlayer_AimPunchServices
    std::map<std::string, uint32_t> schemaAimPunch;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "CCSPlayer_AimPunchServices", schemaAimPunch)) {
        if (schemaAimPunch.count("m_predictableBaseAngle")) m_aimPunchAngle = schemaAimPunch["m_predictableBaseAngle"];
        if (schemaAimPunch.count("m_predictableBaseAngleVel")) m_aimPunchAngleVel = schemaAimPunch["m_predictableBaseAngleVel"];
    }
    // C_CSPlayerPawnBase
    std::map<std::string, uint32_t> schemaCSBase;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "C_CSPlayerPawnBase", schemaCSBase)) {
        if (schemaCSBase.count("m_flFlashDuration")) m_flFlashDuration = schemaCSBase["m_flFlashDuration"];
        if (schemaCSBase.count("m_flFlashBangTime")) m_flFlashBangTime = schemaCSBase["m_flFlashBangTime"];
    }
    // C_BaseModelEntity
    std::map<std::string, uint32_t> schemaModel;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "C_BaseModelEntity", schemaModel)) {
        if (schemaModel.count("m_vecViewOffset")) m_vecViewOffset = schemaModel["m_vecViewOffset"];
        if (schemaModel.count("m_Glow")) m_Glow = schemaModel["m_Glow"];
    }
    // CPlayer_ObserverServices
    std::map<std::string, uint32_t> schemaObs;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "CPlayer_ObserverServices", schemaObs)) {
        if (schemaObs.count("m_hObserverTarget")) m_hObserverTarget = schemaObs["m_hObserverTarget"];
    }
    // CBasePlayerController
    std::map<std::string, uint32_t> schemaCtrl;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "CBasePlayerController", schemaCtrl)) {
        if (schemaCtrl.count("m_iszPlayerName")) m_iszPlayerName = schemaCtrl["m_iszPlayerName"];
    }
    // CCSPlayerController
    std::map<std::string, uint32_t> schemaCCtrl;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "CCSPlayerController", schemaCCtrl)) {
        if (schemaCCtrl.count("m_hPlayerPawn")) m_hPlayerPawn = schemaCCtrl["m_hPlayerPawn"];
        if (schemaCCtrl.count("m_hObserverPawn")) m_hObserverPawn = schemaCCtrl["m_hObserverPawn"];
    }
    // CPlayer_WeaponServices
    std::map<std::string, uint32_t> schemaWpn;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "CPlayer_WeaponServices", schemaWpn)) {
        if (schemaWpn.count("m_hActiveWeapon")) m_hActiveWeapon = schemaWpn["m_hActiveWeapon"];
    }
    // C_EconItemView
    std::map<std::string, uint32_t> schemaEcon;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "C_EconItemView", schemaEcon)) {
        if (schemaEcon.count("m_iItemDefinitionIndex")) m_iItemDefinitionIndex = schemaEcon["m_iItemDefinitionIndex"];
    }
    // CCollisionProperty
    std::map<std::string, uint32_t> schemaCol;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "CCollisionProperty", schemaCol)) {
        if (schemaCol.count("m_vecMins")) m_vecMins = schemaCol["m_vecMins"];
        if (schemaCol.count("m_vecMaxs")) m_vecMaxs = schemaCol["m_vecMaxs"];
    }
    // CCSPlayerBase_CameraServices
    std::map<std::string, uint32_t> schemaCam;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "CCSPlayerBase_CameraServices", schemaCam)) {
        if (schemaCam.count("m_iFOV")) m_iFOV = schemaCam["m_iFOV"];
        if (schemaCam.count("m_iFOVStart")) m_iFOVStart = schemaCam["m_iFOVStart"];
        if (schemaCam.count("m_flFOVTime")) m_flFOVTime = schemaCam["m_flFOVTime"];
        if (schemaCam.count("m_flFOVRate")) m_flFOVRate = schemaCam["m_flFOVRate"];
    }
    // CSkeletonInstance
    std::map<std::string, uint32_t> schemaSkel;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "CSkeletonInstance", schemaSkel)) {
        if (schemaSkel.count("m_modelState")) m_modelState = schemaSkel["m_modelState"];
    }
    // C_PlantedC4
    std::map<std::string, uint32_t> schemaC4;
    if (SimpleJsonParser::LoadSchemaOffsets(schemaJsonPath, "C_PlantedC4", schemaC4)) {
        if (schemaC4.count("m_bBombTicking")) m_bBombTicking = schemaC4["m_bBombTicking"];
        if (schemaC4.count("m_bBombDefused")) m_bBombDefused = schemaC4["m_bBombDefused"];
        if (schemaC4.count("m_flC4Blow")) m_flC4Blow = schemaC4["m_flC4Blow"];
        if (schemaC4.count("m_bBeingDefused")) m_bBeingDefused = schemaC4["m_bBeingDefused"];
    }
    return true;
}

} // namespace memory
