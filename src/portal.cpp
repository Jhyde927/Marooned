#include "portal.h"
#include <iostream>
#include "dungeonColors.h"
#include "dungeonGeneration.h"

using namespace dungeon;
static std::vector<Portal> portals;
static std::vector<Portal> portalsGroup0; // default group (no ID pixel)
static std::vector<Portal> portalsGroup1; // has PortalID pixel to the right


// Simple anti-bounce
static float gTeleportCooldown = 0.0f;
// Optional: remember which portal we teleported FROM (extra safety)
static int gLastPortalIndex = -1;

void PortalSystem::Clear()
{
    gTeleportCooldown = 0.0f;
    gLastPortalIndex = -1;
    portals.clear();
}

void PortalSystem::GenerateFromDungeon(
    const Image& dungeonImage,
    int dungeonWidth,
    int dungeonHeight,
    float tileSize,
    float baseY
)
{
    portalsGroup0.clear();
    portalsGroup1.clear();

    Color* pixels = LoadImageColors(dungeonImage);

    for (int y = 0; y < dungeonHeight; y++)
    {
        for (int x = 0; x < dungeonWidth; x++)
        {
            const int index = y * dungeonWidth + x;
            const Color c = pixels[index];

            if (!EqualsRGB(c, ColorOf(Code::PortalTile)))
                continue;

            // World pos uses your existing dungeon convention (flips etc.)
            Vector3 spawnPos = GetDungeonWorldPos(x, y, tileSize, baseY);

            Portal p;
            p.position = spawnPos;
            p.box = {
                { spawnPos.x - tileSize * 0.4f, baseY - 50.0f, spawnPos.z - tileSize * 0.4f },
                { spawnPos.x + tileSize * 0.4f, baseY + 50.0f, spawnPos.z + tileSize * 0.4f }
            };

            // ---- Group detection: check pixel to the RIGHT (x+1, y)
            bool hasPortalIdRight = false;
            if (x + 1 < dungeonWidth)
            {
                const Color right = pixels[y * dungeonWidth + (x + 1)];
                if (EqualsRGB(right, ColorOf(Code::PortalID)))
                    hasPortalIdRight = true;
            }

            if (hasPortalIdRight){
                portalsGroup1.push_back(p);
                portals.push_back(p);

            }

            else{
                portalsGroup0.push_back(p);
                portals.push_back(p);
            }

        }
    }

    UnloadImageColors(pixels);

    std::cout << "Portals group0: " << portalsGroup0.size()
              << " | group1: " << portalsGroup1.size() << std::endl;
}

static bool TryTeleportInGroup(
    std::vector<Portal>& group,
    Vector3& playerPos,
    float playerRadius,
    float& teleportCooldown
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
        destIndex = (hitIndex == 0) ? 1 : 0;
    }
    else
    {
        destIndex = (hitIndex + 1) % (int)group.size();
    }

    Vector3 destPos = group[destIndex].position;
    playerPos = Vector3Add(destPos, Vector3{ 0.0f, 20.0f, 0.0f });

    teleportCooldown = 3.0f;

    std::cout << "Teleported within group (size " << group.size()
              << ") from " << hitIndex << " to " << destIndex << "\n";

    return true;
}

void PortalSystem::Update(Vector3& playerPos, float playerRadius, float dt)
{
    gTeleportCooldown = std::max(0.0f, gTeleportCooldown - dt);

    if (gTeleportCooldown > 0.0f)
    {
        return;
    }

    if (TryTeleportInGroup(portalsGroup0, playerPos, playerRadius, gTeleportCooldown))
    {
        return;
    }

    if (TryTeleportInGroup(portalsGroup1, playerPos, playerRadius, gTeleportCooldown))
    {
        return;
    }
}


void PortalSystem::DrawDebug()
{
    for (const Portal& p : portals)
    {
        DrawCube(p.position, 40.0f, 80.0f, 40.0f, RED);
        DrawCubeWires(p.position, 40.0f, 80.0f, 40.0f, MAROON);
    }
}
