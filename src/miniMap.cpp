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

    
    explored.assign(dungeonWidth * dungeonHeight, 0);
    visible.assign(dungeonWidth * dungeonHeight, 0);

    initialized = true;
}

void MiniMap::Unload()
{
    if (wallMask.id != 0)
        UnloadTexture(wallMask);

    wallMask = { 0 };
    explored.clear();
    visible.clear();
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
    const Color wallColor  = { 255, 255, 255, 255 }; // bright walls
    const Color floorColor = { 200, 200, 200, 255 }; // dimmer + more transparent
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
            bool isLava = (c.r == 200 && c.g == 0 && c.b == 0);

            // Decide what to draw for this tile
            Color outColor = { 0, 0, 0, 0 };

            if (isWall)
            {
                outColor = wallColor;
            }
            else if (isLava)
            {
                outColor = lavaColor;
            }
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

void MiniMap::RevealAroundPlayer(Vector3 playerPos)
{
    int playerTileX = GetDungeonImageX(playerPos.x, tileSize, dungeonWidth);
    int playerTileY = GetDungeonImageY(playerPos.z, tileSize, dungeonHeight);
    if (playerTileX < 0 || playerTileX >= dungeonWidth ||
        playerTileY < 0 || playerTileY >= dungeonHeight)
        return;

    // Clear per-frame visibility
    std::fill(visible.begin(), visible.end(), 0);

    const int radius   = 8;
    const int radiusSq = radius * radius;

    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            if (dx*dx + dy*dy > radiusSq) continue;

            int tx = playerTileX + dx;
            int ty = playerTileY + dy;
            if (tx < 0 || tx >= dungeonWidth ||
                ty < 0 || ty >= dungeonHeight)
                continue;

            int idx = ty * dungeonWidth + tx;

            Color c = dungeonPixels[idx];
            if (c.a == 0) continue; // void

            // tile-space LOS (your TileLineOfSight using Vector2)
            Vector2 from = { playerTileX + 0.5f, playerTileY + 0.5f };
            Vector2 to   = { tx + 0.5f,          ty + 0.5f };

            if (TileLineOfSight(from, to, dungeonImg))
            {
                visible[idx]  = 1;  // currently in vision
                explored[idx] = 1;  // permanently revealed
            }
        }
    }

    playerU = (playerTileX + 0.5f) / (float)dungeonWidth;
    playerV = (playerTileY + 0.5f) / (float)dungeonHeight;
}




void MiniMap::Update(float dt, Vector3 playerPos)
{
    if (!initialized) return;
    if (!isDungeon) return;
    RevealAroundPlayer(playerPos);
}

void MiniMap::Draw(int screenX, int screenY) const
{
    if (!initialized) return;
    if (!isDungeon) return;
    // 1) Base minimap (walls/floors/lava with your tint)
    Rectangle src = { 0, 0, (float)texWidth, (float)texHeight };
    Rectangle dst = { (float)screenX, (float)screenY, drawSize, drawSize };
    Vector2 origin = { 0, 0 };
    Color mapTint = { 200, 240, 255, 255 }; // light teal/cool tone
    DrawTexturePro(wallMask, src, dst, origin, 0.0f, mapTint);

    float tileW = drawSize / (float)dungeonWidth;
    float tileH = drawSize / (float)dungeonHeight;

    Color fogUnexplored = { 0, 0, 0, 255 };  // solid black
    Color fogMemory     = { 0, 0, 0, 200};  // softer dark for explored-but-not-visible

    for (int y = 0; y < dungeonHeight; ++y)
    {
        for (int x = 0; x < dungeonWidth; ++x)
        {
            int idx = y * dungeonWidth + x;

            float fx = screenX + x * tileW;
            float fy = screenY + y * tileH;

            if (explored[idx] == 0)
            {
                // Never seen
                DrawRectangle((int)fx, (int)fy,
                              (int)ceilf(tileW), (int)ceilf(tileH),
                              fogUnexplored);
            }
            else if (visible[idx] == 0)
            {
                // Seen before, but *not* currently in vision
                DrawRectangle((int)fx, (int)fy,
                              (int)ceilf(tileW), (int)ceilf(tileH),
                              fogMemory);
            }
            // else: visible[idx] != 0 → no overlay, full brightness
        }
    }

    // 3) Player marker (green)
    float px = screenX + playerU * drawSize;
    float py = screenY + playerV * drawSize;
    DrawCircleV(Vector2{ px, py }, 3.0f, GREEN);
}


// void MiniMap::Draw(int screenX, int screenY) const
// {
//     if (!initialized) return;

//     // 1) Draw the base wall mask
//     Rectangle src = { 0, 0, (float)texWidth, (float)texHeight };
//     Rectangle dst = { (float)screenX, (float)screenY, drawSize, drawSize };
//     Vector2 origin = { 0, 0 };

//     Color mapTint = { 200, 240, 255, 255 }; // light teal/cool tone
//     DrawTexturePro(wallMask, src, dst, origin, 0.0f, mapTint);

//     // 2) Draw fog overlay on unexplored tiles
//     float tileW = drawSize / (float)dungeonWidth;
//     float tileH = drawSize / (float)dungeonHeight;

//     Color fogColor = { 0, 0, 0, 255 }; // semi-opaque black

//     for (int y = 0; y < dungeonHeight; ++y)
//     {
//         for (int x = 0; x < dungeonWidth; ++x)
//         {
//             int idx = y * dungeonWidth + x;
//             if (explored[idx] != 0) continue; // already explored → no fog

//             float fx = screenX + x * tileW;
//             float fy = screenY + y * tileH;

//             DrawRectangle((int)fx, (int)fy,
//                           (int)ceilf(tileW), (int)ceilf(tileH),
//                           fogColor);
//         }
//     }

//     // 3) Draw player marker
//     // (Small red dot at UV position)
//     float px = screenX + playerU * drawSize;
//     float py = screenY + playerV * drawSize;

//     DrawCircleV(Vector2{ px, py }, 3.0f, GREEN);
// }

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

