#include "miniMap.h"
#include "raylib.h"
#include <cstring>
#include "world.h"
#include "dungeonGeneration.h"
#include "dungeonColors.h"
#include "pathfinding.h"
#include "iostream"


using namespace dungeon;


static constexpr float REVEAL_FADE_DURATION = 0.2f; // ~180 ms feels snappy

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

    //slow reveal time
    revealTime.assign(dungeonWidth * dungeonHeight, -1.0f); 
    timeSeconds = 0.0f;

    // Make on-screen minimap match texture size to avoid scaling seams
    drawSize = (float)texWidth;

    initialized = true;
}


void MiniMap::Unload()
{
    if (wallMask.id != 0)
        UnloadTexture(wallMask);

    wallMask = { 0 };
    explored.clear();
    visible.clear();
    revealTime.clear();
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
    const Color floorColor = { 255, 255, 255, 128 }; // dimmer + more transparent
    const Color lavaColor  = { 200,  50,  50, 160 }; // glowing-ish red, semi-transparent

    for (int y = 0; y < dungeonHeight; ++y)
    {
        for (int x = 0; x < dungeonWidth; ++x)
        {
            Color c = dungeonPixels[y * dungeonWidth + x];

            // Skip void/transparent tiles
            if (c.a == 0) continue;

            bool isLava = (c.r == 200 && c.g == 0 && c.b == 0);

            // Decide what to draw for this tile
            Color outColor = { 0, 0, 0, 0 };

            if (IsWallColor(c))
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

void MiniMap::RevealDoorsFromPlayer(Vector3 playerPos, const std::vector<Door>& doors)
{
    if (!initialized) return;

    int playerTileX = GetDungeonImageX(playerPos.x, tileSize, dungeonWidth);
    int playerTileY = GetDungeonImageY(playerPos.z, tileSize, dungeonHeight);

    if (playerTileX < 0 || playerTileX >= dungeonWidth ||
        playerTileY < 0 || playerTileY >= dungeonHeight)
        return;

    // Use a similar radius to normal reveal
    const int radius   = 8;
    const int radiusSq = radius * radius;

    for (const Door& door : doors)
    {
        int tileX = door.tileX;
        int tileY = door.tileY;

        if (tileX < 0 || tileX >= dungeonWidth ||
            tileY < 0 || tileY >= dungeonHeight)
            continue;

        int dx = tileX - playerTileX;
        int dy = tileY - playerTileY;
        if (dx*dx + dy*dy > radiusSq)
            continue;

        int idx = tileY * dungeonWidth + tileX;

        // Already revealed this door tile before
        if (explored[idx] != 0)
            continue;

        // If we have line-of-sight *to* the door tile (allowing the last tile to be blocking),
        // mark it explored so it will always show up on the minimap from now on.
        if (CanSeeDoorTile(playerTileX, playerTileY, tileX, tileY))
        {
            explored[idx]   = 1;
            revealTime[idx] = timeSeconds; // nice little fade when it first appears
        }
    }
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
                visible[idx] = 1;

                if (explored[idx] == 0)
                {
                    explored[idx]   = 1;
                    revealTime[idx] = timeSeconds; // first time we’ve ever seen this tile
                }
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

    timeSeconds += dt;
    RevealAroundPlayer(playerPos);
}

void MiniMap::Draw(int screenX, int screenY) const
{
    if (!initialized || !isDungeon) return;

    // Background plate (you can tweak alpha if you want a frame)
    Color bg = { 0, 0, 0, 0 };
    DrawRectangle(screenX, screenY, (int)drawSize, (int)drawSize, bg);

    float tileW = drawSize / (float)dungeonWidth;
    float tileH = drawSize / (float)dungeonHeight;

    Color memoryFog = { 0, 0, 0, 80 }; // explored but not currently visible

    for (int y = 0; y < dungeonHeight; ++y)
    {
        for (int x = 0; x < dungeonWidth; ++x)
        {
            int idx = y * dungeonWidth + x;

            if (explored[idx] == 0)
            {
                // Unexplored: draw NOTHING from the map.
                continue;
            }

            // --- 1) Draw this tile's part of the minimap texture ---

            Rectangle src = {
                (float)(x * scale),
                (float)(y * scale),
                (float)scale,
                (float)scale
            };

            Rectangle dst = {
                screenX + x * tileW,
                screenY + y * tileH,
                tileW,
                tileH
            };

            Vector2 origin = { 0, 0 };
            DrawTexturePro(wallMask, src, dst, origin, 0.0f, WHITE);

            // --- 2) Reveal fade-in overlay for newly discovered tiles ---

            float rt = revealTime[idx];
            if (rt >= 0.0f)
            {
                float elapsed = timeSeconds - rt;

                if (elapsed < REVEAL_FADE_DURATION)
                {
                    // 0 -> black, 1 -> fully revealed
                    float t = (elapsed <= 0.0f) ? 0.0f
                             : (elapsed / REVEAL_FADE_DURATION);
                    if (t < 1.0f)
                    {
                        unsigned char a = (unsigned char)((1.0f - t) * 220.0f);
                        Color fade = { 0, 0, 0, a };
                        DrawRectangleRec(dst, fade);
                    }
                }
            }

            // --- 3) Memory fog: explored but not currently in LOS ---

            if (visible[idx] == 0)
            {
                DrawRectangleRec(dst, memoryFog);
            }
        }
    }

    // 4) Player marker (on top)
    float px = screenX + playerU * drawSize;
    float py = screenY + playerV * drawSize;
    DrawCircleV(Vector2{ px, py }, 3.0f, GREEN);
}


void MiniMap::DrawEnemies(const std::vector<Character*>& enemies,
                          int screenX, int screenY) 
{
    if (!initialized || !isDungeon) return;

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
        Vector3 eye = pos; eye.y += 40.0f;  // or player eye, depending how you want it
        if (!HasWorldLineOfSight(player.position, pos)) continue;

        // Convert tile coord → minimap pixel center
        float u = (tileX + 0.5f) / (float)dungeonWidth;
        float v = (tileY + 0.5f) / (float)dungeonHeight;

        float ex = screenX + u * drawSize;
        float ey = screenY + v * drawSize;

        DrawCircleV(Vector2{ ex, ey }, 2.5f, RED);
    }
}

void MiniMap::DrawDoors(const std::vector<Door>& doors,
                        int screenX, int screenY) const
{
    if (!initialized) return;

    float tileW = drawSize / (float)dungeonWidth;
    float tileH = drawSize / (float)dungeonHeight;

    //could use dungeonColors.h here. 
    //Draw door markers as different colors depending on door type. 
    Color normalDoorColor = { 255,  255, 255, 128 }; // lighter gray
    Color lockedDoorColor = { 200,  30,  30, 128 }; // red/pink toned
    Color exitDoorColor   = {   0, 200, 200, 128 }; // teal (go back level) never used but maybe in the future. 
    Color nextDoorColor   = { 255, 180,   0, 128 }; // orange (progress)
    Color portalColor     = { 220,   0, 220, 128 }; // magenta portals
    Color eventLockedColor= {   0, 255, 128, 128 }; // spring green/event-locked

    for (const Door& door : doors)
    {
        int tileX = door.tileX;
        int tileY = door.tileY;

        if (tileX < 0 || tileX >= dungeonWidth ||
            tileY < 0 || tileY >= dungeonHeight)
            continue;

        int idx = tileY * dungeonWidth + tileX;

        // Only draw door markers after the player has *seen* the door tile once
        if (explored.empty() || explored[idx] == 0)
            continue;

        // Pick color based on door type
        Color c = normalDoorColor;

        if (door.isLocked)
            c = lockedDoorColor;
        else if (door.doorType == DoorType::ExitToPrevious)
            c = exitDoorColor;
        else if (door.isPortal)
            c = portalColor;
        else if (door.eventLocked)
            c = eventLockedColor;
        else if (door.doorType == DoorType::GoToNext) // if you have that flag
            c = nextDoorColor;

        // Center of door tile
        float u = (tileX + 0.5f) / (float)dungeonWidth;
        float v = (tileY + 0.5f) / (float)dungeonHeight;

        float dx = screenX + u * drawSize;
        float dy = screenY + v * drawSize;

        int size = 8; // chunky door icon
        DrawRectangle((int)(dx - size * 0.5f),
                      (int)(dy - size * 0.5f),
                      size, size,
                      c);
    }
}
