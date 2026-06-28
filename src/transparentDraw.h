#pragma once
#include "raylib.h"
#include <vector>
#include "decal.h"
#include "weapon.h"
#include "portal.h"
#include "spawn_manager.h"

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
    BillboardType type = Billboard_FacingCamera;

    Vector3 position = Vector3{0.0f, 0.0f, 0.0f};
    Texture2D texture = Texture2D{0U, 0, 0, 0, 0};
    Rectangle sourceRect = Rectangle{0.0f, 0.0f, 0.0f, 0.0f};
    Vector2 size = Vector2{0.0f, 0.0f};

    Color tint = WHITE;

    float distanceToCamera = 0.0f;
    float rotationY = 0.0f;

    bool flipX = false;
    bool isPortal = false;
    bool isOpen = false;

    PortalPalette pallet = { {0.0f, 0.25f, 1.0f}, {0.5f, 0.2f, 1.0f} };
    
    float openAmount = 0.0f;
};

// void GatherGrapplePoint(Camera& camera) {
//     for (GrapplePoint& g : grapplePoints){
//         //float dist = Vector3Distance(camera.position, g.position);
//         float dist = Vector3DistanceSqr(camera.position, g.position);
//         billboardRequests.push_back({
//             Billboard_FacingCamera, 
//             g.position,
//             g.tex,
//             Rectangle{0, 0, (float)g.tex.width, (float)g.tex.height},
//             Vector2{g.scale,g.scale},
//             WHITE,
//             dist,
//             1.0f,
//             false,
//             false,
//             false,
//             0.0f
//         });
//     }
// }

extern std::vector<BillboardDrawRequest> billboardRequests;

void GatherTransparentDrawRequests(Camera& camera, float deltaTime);
void DrawTransparentDrawRequests(Camera& camera);
void GatherDoors(Camera& camera);
void GatherMuzzleFlashes(Camera& camera, const std::vector<MuzzleFlash>& activeMuzzleFlashes);
void GatherEnemies(Camera& camera);
void GatherDungeonFires(Camera& camera, float deltaTime);
void GatherWebs(Camera& camera);
void GatherDecals(Camera& camera, const std::vector<Decal>& decals);
void GatherSpawnPortals(Camera& camera);
void GatherPortals(Camera& camera, const std::vector<Portal>& portals);
float GetAdjustedBillboardSize(float baseSize, float distance);
PortalPalette GetPortalPalette(int groupID);