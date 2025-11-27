#include "miniMap.h"
#include "raylib.h"
#include <cstring>
#include "world.h"
#include "dungeonGeneration.h"
#include "dungeonColors.h"
#include "pathfinding.h"

// External dungeon info
// extern int dungeonWidth;
// extern int dungeonHeight;
// extern Color* dungeonPixels;
using namespace dungeon;
// Your existing helpers (forward declare)


// World->image helpers (you already use these for barrels)
extern float tileSize;
int GetDungeonImageX(float worldX, float tileSize, int dungeonWidth);
int GetDungeonImageY(float worldZ, float tileSize, int dungeonHeight);

MiniMap::MiniMap() {}
MiniMap::~MiniMap()
{
    Unload();
}

void MiniMap::Initialize(int scaleValue)
{
    scale = scaleValue;

    texWidth  = dungeonWidth  * scale;
    texHeight = dungeonHeight * scale;

    wallMask = GenerateWallMaskTexture();

    // Init explored grid: 0 = not explored, 1 = explored
    explored.assign(dungeonWidth * dungeonHeight, 0);

    initialized = true;
}

void MiniMap::Unload()
{
    if (wallMask.id != 0)
        UnloadTexture(wallMask);

    wallMask = { 0 };
    explored.clear();
    initialized = false;
}

bool MiniMap::IsExplored(int x, int y) const
{
    if (x < 0 || x >= dungeonWidth ||
        y < 0 || y >= dungeonHeight)
        return false;

    int idx = y * dungeonWidth + x;
    return explored[idx] != 0;
}

Texture2D MiniMap::GenerateWallMaskTexture()
{
    const int mapW = texWidth;
    const int mapH = texHeight;
    const int s    = scale;

    // Start fully transparent
    std::vector<Color> pixels(mapW * mapH, { 0, 0, 0, 0 });

    // Visual colors on the minimap
    const Color wallColor  = { 230, 230, 230, 255 }; // bright walls
    const Color floorColor = { 200, 200, 200, 120 }; // dimmer + more transparent
    const Color lavaColor  = { 200,  50,  50, 160 }; // glowing-ish red, semi-transparent

    for (int y = 0; y < dungeonHeight; ++y)
    {
        for (int x = 0; x < dungeonWidth; ++x)
        {
            Color c = dungeonPixels[y * dungeonWidth + x];

            // Skip void/transparent tiles
            if (c.a == 0) continue;

            // Barrel = pure blue (you can tweak if needed)
            bool isBarrel = (c.r == 0 && c.g == 0 && c.b == 255);

            // Wall = pure black
            bool isWall = (c.r == 0 && c.g == 0 && c.b == 0);

            // Lava = (example) pure red; replace with your actual lava color
            //bool isLava = (c.r == 200 && c.g == 0 && c.b == 0);

            // Decide what to draw for this tile
            Color outColor = { 0, 0, 0, 0 };

            if (isWall)
            {
                outColor = wallColor;
            }
            // else if (isLava)
            // {
            //     outColor = lavaColor;
            // }
            else
            {
                // Everything else that is not void/wall/lava becomes "floor"
                // Including tiles with barrels, doors, etc. – they’ll sit on floor.
                outColor = floorColor;
            }

            if (outColor.a == 0) continue; // nothing to draw

            int baseX = x * s;
            int baseY = y * s;

            for (int oy = 0; oy < s; ++oy)
            {
                int py = baseY + oy;
                for (int ox = 0; ox < s; ++ox)
                {
                    int px = baseX + ox;
                    pixels[py * mapW + px] = outColor;
                }
            }
        }
    }

    Image img = {
        .data = pixels.data(),
        .width = mapW,
        .height = mapH,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    Texture2D tex = LoadTextureFromImage(img);
    // Do NOT UnloadImage(img) here; pixels is owned by std::vector

    return tex;
}

void MiniMap::RevealAroundPlayer(Vector3 playerPos) //with LOS
{
    // Convert player world pos to dungeon tile
    int playerTileX = GetDungeonImageX(playerPos.x, tileSize, dungeonWidth);
    int playerTileY = GetDungeonImageY(playerPos.z, tileSize, dungeonHeight);

    if (playerTileX < 0 || playerTileX >= dungeonWidth ||
        playerTileY < 0 || playerTileY >= dungeonHeight)
        return;

    // Vision radius in tiles (tweak this)
    const int radius = 8;
    const int radiusSq = radius * radius;

    // Eye height so we don't raycast from feet (tweak to match your game)
    // Vector3 eyePos = playerPos;
    // eyePos.y += 100.0f; 

    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            // Optional: keep it roughly circular
            if (dx*dx + dy*dy > radiusSq) continue;

            int tx = playerTileX + dx;
            int ty = playerTileY + dy;

            if (tx < 0 || tx >= dungeonWidth ||
                ty < 0 || ty >= dungeonHeight)
                continue;

            int idx = ty * dungeonWidth + tx;

            // Optional: skip void tiles
            Color c = dungeonPixels[idx];
            if (c.a == 0) continue;

            Vector2 from = { playerTileX + 0.5f, playerTileY + 0.5f };
            Vector2 to   = { tx + 0.5f,          ty + 0.5f };
            // Only reveal if LOS is clear
            if (TileLineOfSight(from, to, dungeonImg)) //use tile LOS not world. WorldLOS was leaky. 
            {
                explored[idx] = 1;
            }
        }
    }

    // Store player position in minimap UV (for the red dot)
    playerU = (playerTileX + 0.5f) / (float)dungeonWidth;
    playerV = (playerTileY + 0.5f) / (float)dungeonHeight;
}


void MiniMap::Update(float dt, Vector3 playerPos)
{
    if (!initialized) return;
    RevealAroundPlayer(playerPos);
}

void MiniMap::Draw(int screenX, int screenY) const
{
    if (!initialized) return;

    // 1) Draw the base wall mask
    Rectangle src = { 0, 0, (float)texWidth, (float)texHeight };
    Rectangle dst = { (float)screenX, (float)screenY, drawSize, drawSize };
    Vector2 origin = { 0, 0 };

    Color mapTint = { 200, 240, 255, 200 }; // light teal/cool tone
    DrawTexturePro(wallMask, src, dst, origin, 0.0f, mapTint);

    // 2) Draw fog overlay on unexplored tiles
    float tileW = drawSize / (float)dungeonWidth;
    float tileH = drawSize / (float)dungeonHeight;

    Color fogColor = { 0, 0, 0, 255 }; // semi-opaque black

    for (int y = 0; y < dungeonHeight; ++y)
    {
        for (int x = 0; x < dungeonWidth; ++x)
        {
            int idx = y * dungeonWidth + x;
            if (explored[idx] != 0) continue; // already explored → no fog

            float fx = screenX + x * tileW;
            float fy = screenY + y * tileH;

            DrawRectangle((int)fx, (int)fy,
                          (int)ceilf(tileW), (int)ceilf(tileH),
                          fogColor);
        }
    }

    // 3) Draw player marker
    // (Small red dot at UV position)
    float px = screenX + playerU * drawSize;
    float py = screenY + playerV * drawSize;

    DrawCircleV(Vector2{ px, py }, 3.0f, GREEN);
}

void MiniMap::DrawEnemies(const std::vector<Character*>& enemies,
                          int screenX, int screenY) 
{
    if (!initialized) return;

    float tileW = drawSize / (float)dungeonWidth;
    float tileH = drawSize / (float)dungeonHeight;

    for (Character* enemy : enemies)
    {
        if (!enemy) continue;
        if (enemy->isDead) continue; // or whatever flag you use

        Vector3 pos = enemy->position;

        // Convert enemy world position to dungeon tile
        int tileX = GetDungeonImageX(pos.x, tileSize, dungeonWidth);
        int tileY = GetDungeonImageY(pos.z, tileSize, dungeonHeight);

        if (tileX < 0 || tileX >= dungeonWidth ||
            tileY < 0 || tileY >= dungeonHeight)
            continue;

        int idx = tileY * dungeonWidth + tileX;

        // Only show enemies in explored tiles
        if (explored.empty() || explored[idx] == 0)
            continue;

        // OPTIONAL: only show if in LOS as well
        // Vector3 eye = pos; eye.y += 40.0f;  // or player eye, depending how you want it
        // if (!GetWorldLineOfSight(playerEyePos, pos)) continue;

        // Convert tile coord → minimap pixel center
        float u = (tileX + 0.5f) / (float)dungeonWidth;
        float v = (tileY + 0.5f) / (float)dungeonHeight;

        float ex = screenX + u * drawSize;
        float ey = screenY + v * drawSize;

        DrawCircleV(Vector2{ ex, ey }, 2.5f, RED);
    }
}

