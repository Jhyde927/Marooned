#include "portal.h"
#include <iostream>
#include "dungeonColors.h"
#include "dungeonGeneration.h"
#include "resourceManager.h"
#include "world.h"
#include "sound_manager.h"

using namespace dungeon;

static std::vector<Portal> portalsGroup0; // default group (no ID pixel)
static std::vector<Portal> portalsGroup1; // has PortalID pixel to the right
static std::vector<Portal> portalsGroup2; // has portalID pixel to the left
static std::vector<Portal> portalsGroup3; // has portalID pixel to the left and right
static std::vector<Portal> portalsGroup4; // has portalID pixel to the top
static std::vector<Portal> portalsGroup5; // has portalID pixel to the bottom
static std::vector<Portal> portalsGroup6; // has portalID pixel to the top and bottom


// Simple anti-bounce
static float gTeleportCooldown = 0.0f;

// Fade/teleport timing
static bool  gTeleportPending = false;
static float gTeleportDelayLeft = 0.0f;     // counts down from 0.1f
static Vector3 gPendingDestPos = { 0, 0, 0 };

static int gPendingGroupIndex = -1;

// Optional: remember which portal we teleported FROM (extra safety)
static int gLastPortalIndex = -1;

void PortalSystem::Clear()
{
    gTeleportCooldown = 0.0f;
    gLastPortalIndex = -1;
    portals.clear();
    portalsGroup0.clear();
    portalsGroup1.clear();
    portalsGroup2.clear();
    portalsGroup3.clear();
    portalsGroup4.clear();
    portalsGroup5.clear();
    portalsGroup6.clear();

}

void PortalSystem::GenerateFromDungeon(
    const Image& dungeonImage,
    int dungeonWidth,
    int dungeonHeight,
    float tileSize,
    float baseY
)
{
    portals.clear();
    portalsGroup0.clear();
    portalsGroup1.clear();
    portalsGroup2.clear();
    portalsGroup3.clear();
    portalsGroup4.clear();
    portalsGroup5.clear();
    portalsGroup6.clear();

    Color* pixels = LoadImageColors(dungeonImage);

    for (int y = 0; y < dungeonHeight; y++)
    {
        for (int x = 0; x < dungeonWidth; x++)
        {
            const int index = y * dungeonWidth + x;
            const Color c = pixels[index];

            if (!EqualsRGB(c, ColorOf(Code::PortalTile)))
                continue;

            Vector3 spawnPos = GetDungeonWorldPos(x, y, tileSize, baseY);

            Portal p;
            p.position = spawnPos;
            p.box = {
                { spawnPos.x - tileSize * 0.4f, baseY - 50.0f, spawnPos.z - tileSize * 0.4f },
                { spawnPos.x + tileSize * 0.4f, baseY + 50.0f, spawnPos.z + tileSize * 0.4f }
            };

            // Detect PortalID neighbors
            bool hasPortalIdRight = false;
            if (x + 1 < dungeonWidth)
            {
                const Color right = pixels[y * dungeonWidth + (x + 1)];
                hasPortalIdRight = EqualsRGB(right, ColorOf(Code::PortalID));
            }

            bool hasPortalIdLeft = false;
            if (x - 1 >= 0)
            {
                const Color left = pixels[y * dungeonWidth + (x - 1)];
                hasPortalIdLeft = EqualsRGB(left, ColorOf(Code::PortalID));
            }

            bool hasPortalIdTop = false;
            if (y + 1 < dungeonHeight)
            {
                const Color top = pixels[(y + 1) * dungeonWidth + x];
                hasPortalIdTop = EqualsRGB(top, ColorOf(Code::PortalID));
            }

            bool hasPortalIdBottom = false;
            if (y - 1 >= 0)
            {
                const Color bottom = pixels[(y - 1) * dungeonWidth + x];
                hasPortalIdBottom = EqualsRGB(bottom, ColorOf(Code::PortalID));
            }

            // Group priority:
            // both sides => group3
            // left-only  => group2
            // right-only => group1
            // top-only =>   group4
            // bottom-only => group5
            // top and bottom => group6
            // none       => group0
            if (hasPortalIdLeft && hasPortalIdRight)
            {
                p.tint = RED;
                p.groupID = 3;
                portalsGroup3.push_back(p);
            }
            else if (hasPortalIdLeft)
            {
                p.tint = BLUE;
                p.groupID = 2;
                portalsGroup2.push_back(p);
            }
            else if (hasPortalIdRight)
            {
                p.tint = GREEN;
                p.groupID = 1;
                portalsGroup1.push_back(p);
            }
            else if (hasPortalIdBottom && hasPortalIdTop)
            {
                p.tint = SKYBLUE;
                p.groupID = 6;
                portalsGroup6.push_back(p);
            }
            else if (hasPortalIdTop)
            {
                p.tint = ORANGE;
                p.groupID = 4;
                portalsGroup4.push_back(p);
            }
            else if (hasPortalIdBottom)
            {
                p.tint = PINK;
                p.groupID = 5;
                portalsGroup5.push_back(p);
            }
            else
            {
                p.tint = PURPLE;
                p.groupID = 0;
                portalsGroup0.push_back(p);
            }

            portals.push_back(p);
        }
    }

    UnloadImageColors(pixels);
}



static bool TryTeleportInGroup(
    std::vector<Portal>& group,
    Vector3& playerPos,
    float playerRadius,
    float& teleportCooldown,
    int groupIndex
)
{
    if ((int)group.size() < 2)
    {
        return false;
    }

    int hitIndex = -1;
    for (int i = 0; i < (int)group.size(); i++)
    {
        if (CheckCollisionBoxSphere(group[i].box, playerPos, playerRadius))
        {
            hitIndex = i;
            break;
        }
    }

    if (hitIndex == -1)
    {
        return false;
    }

    int destIndex = -1;
    if ((int)group.size() == 2)
    {
        destIndex = (hitIndex == 0) ? 1 : 0; //destination = the other portal
    }
    else
    {
        destIndex = (hitIndex + 1) % (int)group.size(); //cycle through portals
    }

    // Arm the pending teleport (do NOT move player yet)
    gPendingDestPos = group[destIndex].position;
    gTeleportPending = true;
    gTeleportDelayLeft = 0.10f; // fade out time before teleport
    gPendingGroupIndex = groupIndex;

    StartFadeOutFromTeleport();

    return true;
}


void PortalSystem::Update(Vector3& playerPos, float playerRadius, float dt)
{
    gTeleportCooldown = std::max(0.0f, gTeleportCooldown - dt);

    // Step 1: If we are waiting to teleport, count down and execute
    if (gTeleportPending)
    {
        gTeleportDelayLeft = std::max(0.0f, gTeleportDelayLeft - dt);

        if (gTeleportDelayLeft <= 0.0f)
        {
            // Move player AFTER fade out
            playerPos = Vector3Add(gPendingDestPos, Vector3{ 0.0f, 20.0f, 0.0f });
            
            std::string pSound = (GetRandomValue(0, 1) > 0) ? "portal" : "portal2";
            SoundManager::GetInstance().Play(pSound);

            // Start fade in elsewhere

            // Start cooldown now (prevents instant retrigger)
            gTeleportCooldown = 3.0f;

            // Clear pending
            gTeleportPending = false;
            gPendingGroupIndex = -1;
        }

        return;
    }

    // Step 2: If we're cooling down, do nothing
    if (gTeleportCooldown > 0.0f)
    {
        return;
    }

    // Step 3: Try to start a teleport (this will arm pending + start fade out)
    if (TryTeleportInGroup(portalsGroup0, playerPos, playerRadius, gTeleportCooldown, 0))
    {
        return;
    }

    if (TryTeleportInGroup(portalsGroup1, playerPos, playerRadius, gTeleportCooldown, 1))
    {
        return;
    }

    if (TryTeleportInGroup(portalsGroup2, playerPos, playerRadius, gTeleportCooldown, 2))
    {
        return;
    }

    if (TryTeleportInGroup(portalsGroup3, playerPos, playerRadius, gTeleportCooldown, 3))
    {
        return;
    }

    if (TryTeleportInGroup(portalsGroup4, playerPos, playerRadius, gTeleportCooldown, 4))
    {
        return;
    }

    if (TryTeleportInGroup(portalsGroup5, playerPos, playerRadius, gTeleportCooldown, 5))
    {
        return;
    }

    if (TryTeleportInGroup(portalsGroup6, playerPos, playerRadius, gTeleportCooldown, 6))
    {
        return;
    }
}



void PortalSystem::DrawDebug(Camera& camera)
{
    for (const Portal& p : portals)
    {
        DrawCube(p.position, 40.0f, 80.0f, 40.0f, RED);
        DrawCubeWires(p.position, 40.0f, 80.0f, 40.0f, MAROON);

        // DrawBillboardRec(camera, gradientTex, srcRect, portalPos, Vector2{100.0f, 200.0f}, WHITE);
    }
}
