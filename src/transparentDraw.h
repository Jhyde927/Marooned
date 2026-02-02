#pragma once
#include "raylib.h"
#include <vector>
#include "decal.h"
#include "weapon.h"
#include "portal.h"

struct PortalPalette
{
    Vector3 colorA;
    Vector3 colorB;
};

enum BillboardType {
    Billboard_FacingCamera,
    Billboard_FixedFlat,
    Billboard_Decal,
    Billboard_Door

};

struct BillboardDrawRequest {
    BillboardType type;
    Vector3 position;
    Texture2D texture;
    Rectangle sourceRect;
    float size;
    Color tint;
    float distanceToCamera;
    float rotationY;
    bool flipX = false; 
    bool isPortal;
    bool isOpen;
    PortalPalette pallet;
};

extern std::vector<BillboardDrawRequest> billboardRequests;

void GatherTransparentDrawRequests(Camera& camera, float deltaTime);
void DrawTransparentDrawRequests(Camera& camera);
void GatherDoors(Camera& camera);
void GatherMuzzleFlashes(Camera& camera, const std::vector<MuzzleFlash>& activeMuzzleFlashes);
void GatherEnemies(Camera& camera);
void GatherDungeonFires(Camera& camera, float deltaTime);
void GatherWebs(Camera& camera);
void GatherDecals(Camera& camera, const std::vector<Decal>& decals);
void GatherPortals(Camera& camera, const std::vector<Portal>& portals);
float GetAdjustedBillboardSize(float baseSize, float distance);