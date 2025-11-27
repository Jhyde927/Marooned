#pragma once

#include "raylib.h"
#include <vector>
#include "character.h"


// Forward declarations
// extern int dungeonWidth;
// extern int dungeonHeight;
// extern Color* dungeonPixels;

class MiniMap
{
public:
    MiniMap();
    ~MiniMap();

    void Initialize(int scale);
    void Unload();

    // playerPos = world-space player position (x,z used)
    void Update(float dt, Vector3 playerPos);
    void DrawEnemies(const std::vector<Character*>& enemies,
                     int screenX, int screenY);

    // screenX/screenY = top-left position on screen where minimap is drawn
    void Draw(int screenX, int screenY) const;

    // New: draw enemy dots on the minimap



    void SetDrawSize(float size) { drawSize = size; }
    float GetDrawSize() const { return drawSize; }

    // Optional: other systems can query exploration
    bool IsExplored(int x, int y) const;

private:
    int scale      = 4;     // pixels per dungeon tile (in minimap texture)
    int texWidth   = 0;
    int texHeight  = 0;

    float drawSize = 160.0f;  // on-screen minimap size (square)

    Texture2D wallMask = { 0 };
    bool initialized   = false;

    // Tile-based exploration: size = dungeonWidth * dungeonHeight (0 = not explored, 1 = explored)
    std::vector<unsigned char> explored;

    // Last known player minimap UV (0..1 in each axis); used for drawing player dot
    float playerU = 0.5f;
    float playerV = 0.5f;

    Texture2D GenerateWallMaskTexture();
    void RevealAroundPlayer(Vector3 playerPos);

};


