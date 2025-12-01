#pragma once
#include "raylib.h"
#include <vector>
#include "decal.h"
#include "weapon.h"

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
    bool isPortal;
    bool isOpen;
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

float GetAdjustedBillboardSize(float baseSize, float distance);