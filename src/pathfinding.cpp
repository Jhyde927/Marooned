#include "pathfinding.h"
#include <queue>
#include <unordered_map>
#include "raymath.h"
#include <algorithm>
#include "dungeonGeneration.h"
#include "world.h"
#include "character.h"
#include "utilities.h"


std::vector<std::vector<bool>> walkable; //grid of bools that mark walkabe/unwalkable tiles. 

std::vector<Vector2> FindPath(Vector2 start, Vector2 goal) {
    const int width  = (int)walkable.size();         // X dimension (cols)
    if (width == 0) return {};
    const int height = (int)walkable[0].size();      // Y dimension (rows)

    auto inBounds = [&](int x, int y){
        return x >= 0 && y >= 0 && x < width && y < height;
    };
    auto toIndex = [&](int x, int y){
        return y * width + x; // stride by width because first index is X
    };

    const int sx = (int)start.x, sy = (int)start.y;
    const int gx = (int)goal.x,  gy = (int)goal.y;

    if (!inBounds(sx, sy) || !inBounds(gx, gy)) return {};

    if (!walkable[sx][sy]) return {};
    if (!walkable[gx][gy]) return {};

    std::queue<Vector2> frontier;
    frontier.push({(float)sx, (float)sy});

    std::unordered_map<int, Vector2> cameFrom;
    cameFrom[toIndex(sx, sy)] = {-1, -1};

    static const int dx[4] = { 1, -1,  0,  0 };
    static const int dy[4] = { 0,  0,  1, -1 };

    bool reached = false;

    while (!frontier.empty()) {
        Vector2 cur = frontier.front(); frontier.pop();
        const int cx = (int)cur.x, cy = (int)cur.y;

        if (cx == gx && cy == gy) { reached = true; break; }

        for (int i = 0; i < 4; ++i) {
            const int nx = cx + dx[i];
            const int ny = cy + dy[i];
            if (!inBounds(nx, ny)) continue;
            if (!walkable[nx][ny]) continue; // NOTE: [x][y] on purpose

            const int idx = toIndex(nx, ny);
            if (cameFrom.count(idx) == 0) {
                cameFrom[idx] = cur;
                frontier.push({ (float)nx, (float)ny });
            }
        }
    }

    //if goal never discovered, no path.
    if (!reached) return {};

    // Reconstruct
    std::vector<Vector2> path;
    Vector2 cur = { (float)gx, (float)gy };
    while (!(cur.x == -1 && cur.y == -1)) {
        path.push_back(cur);
        cur = cameFrom[toIndex((int)cur.x, (int)cur.y)];
    }
    std::reverse(path.begin(), path.end());

    // Optional: stop one tile short
    // if (path.size() > 1) path.pop_back();

    return path;
}



void ConvertImageToWalkableGrid(const Image& dungeonMap) {
    walkable.clear();
    walkable.resize(dungeonMap.width, std::vector<bool>(dungeonMap.height, false));

    for (int x = 0; x < dungeonMap.width; ++x) {
        for (int y = 0; y < dungeonMap.height; ++y) {
            Color c = GetImageColor(dungeonMap, x, y);

            if (c.a == 0) {
                walkable[x][y] = false;
                continue;
            }

            bool black   = (c.r == 0 && c.g == 0 && c.b == 0);       // walls
            bool blue    = (c.r == 0 && c.g == 0 && c.b == 255);     // barrels
            bool yellow  = (c.r == 255 && c.g == 255 && c.b == 0);   // light pedestals
            bool skyBlue = (c.r == 0 && c.g == 128 && c.b == 255);   // chests
            bool purple  = (c.r == 128 && c.g == 0 && c.b == 128);   // closed doors
            bool aqua    = (c.r == 0 && c.g == 255 && c.b == 255);   // locked doors
            bool lava     = (c.r == 200 && c.g == 0 && c.b == 0);    // lava pit

            walkable[x][y] = !(black || blue || yellow || skyBlue || purple || aqua || lava);
        }
    }
}





Vector2 WorldToImageCoords(Vector3 worldPos) {
    int x = (int)(worldPos.x / tileSize);
    int y = (int)(worldPos.z / tileSize);

    // Flip Y and X to match how image pixels map to world
    x = dungeonWidth - 1 - x;
    y = dungeonHeight - 1 - y;

    return { (float)x, (float)y };
}


bool IsSeeThroughForLOS(int x, int y)
{
    // Out of bounds = blocks LOS
    if (x < 0 || x >= (int)walkable.size())  return false;
    if (y < 0 || y >= (int)walkable[0].size()) return false;

    // Normal case: walkable = see-through
    if (walkable[x][y]) return true;

    // Special case: lava is NOT walkable but still see-through for vision
    if (IsLavaTile(x, y)) return true;

    // Everything else (walls, closed doors, barrels, etc) blocks LOS
    return false;
}

bool IsWalkable(int x, int y, const Image& dungeonMap) {
    if (x < 0 || x >= dungeonMap.width || y < 0 || y >= dungeonMap.height)
        return false;

    Color c = GetImageColor(dungeonMap, x, y);

    // Transparent = not walkable
    if (c.a == 0)
        return false;

    // Match walkability rules from ConvertImageToWalkableGrid
    bool black    = (c.r == 0 && c.g == 0 && c.b == 0);       // walls
    bool blue     = (c.r == 0 && c.g == 0 && c.b == 255);     // barrels
    bool yellow   = (c.r == 255 && c.g == 255 && c.b == 0);   // light pedestals
    bool skyBlue  = (c.r == 0 && c.g == 128 && c.b == 255);   // chests 
    bool purple   = (c.r == 128 && c.g == 0 && c.b == 128);   // closed doors
    bool aqua     = (c.r == 0 && c.g == 255 && c.b == 255);   // locked doors
    bool lava     = (c.r == 200 && c.g == 0 && c.b == 0);     // lava

    return !(black || blue || yellow || skyBlue || purple || aqua || lava);
}

extern std::vector<std::vector<bool>> walkable;



bool CanSeeDoorTile(int x0, int y0, int x1, int y1)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy;

    // Start tile should be walkable / see-through
    if (!IsSeeThroughForLOS(x0, y0)) return false;

    while (!(x0 == x1 && y0 == y1))
    {
        int e2 = err * 2;

        int prevX = x0;
        int prevY = y0;

        bool stepX = false;
        bool stepY = false;

        if (e2 > -dy) { err -= dy; x0 += sx; stepX = true; }
        if (e2 <  dx) { err += dx; y0 += sy; stepY = true; }

        // Supercover: prevent cutting corners through walls
        if (stepX && stepY)
        {
            if (!IsSeeThroughForLOS(prevX + sx, prevY))   return false;
            if (!IsSeeThroughForLOS(prevX,       prevY + sy)) return false;
        }

        // If we've reached the door tile, we allow it even if *it* is blocking.
        if (x0 == x1 && y0 == y1)
        {
            return true;
        }

        // Intervening tiles must be see-through
        if (!IsSeeThroughForLOS(x0, y0)) return false;
    }

    return true; // should hit return above, but safe default
}




bool IsTileOccupied(int x, int y, const std::vector<Character*>& skeletons, const Character* self) {
    for (const Character* s : enemyPtrs) {
        if (s == self || s->state == CharacterState::Death) continue; 

        Vector2 tile = WorldToImageCoords(s->position);
        if ((int)tile.x == x && (int)tile.y == y) {
            return true;
        }
    }
    return false;
}

Character* GetTileOccupier(int x, int y, const std::vector<Character*>& skeletons, const Character* self) {
    //skeles can't occupy the same tile while stoped. 
    for (Character* s : skeletons) {
        if (s == self || s->state == CharacterState::Death) continue;

        Vector2 tile = WorldToImageCoords(s->position);
        if ((int)tile.x == x && (int)tile.y == y) {
            return s; // first one we find
        }
    }
    return nullptr;
}

// Biases the target angle away from the player, otherwise identical to GetRetreatTile
static Vector2 GetRetreatTileAwayFrom(
    const Vector2& start,
    const Vector2& playerTile,
    const Character* self,
    float targetDistance,
    float tolerance,
    int maxAttempts)
{
    for (int i = 0; i < maxAttempts; ++i)
    {
        // Base direction = away from player
        float baseAngle = atan2f(start.y - playerTile.y, start.x - playerTile.x);
        // Add a little randomness so it isn't perfectly straight
        float angle = baseAngle + (GetRandomValue(-45, 45) * DEG2RAD);

        float dist = targetDistance + GetRandomValue((int)-tolerance, (int)tolerance);

        int rx = (int)(start.x + cosf(angle) * dist);
        int ry = (int)(start.y + sinf(angle) * dist);

        if (rx < 0 || ry < 0 || rx >= dungeonWidth || ry >= dungeonHeight) continue;
        if (!walkable[rx][ry]) continue;
        if (IsTileOccupied(rx, ry, enemyPtrs, self)) continue;

        return { (float)rx, (float)ry };
    }
    return { -1, -1 };
}

// Simple ring sampler (no LOS) 
static Vector2 GetRetreatTile(
    const Vector2& start,
    const Character* self,
    float targetDistance,
    float tolerance,
    int maxAttempts)
{
    for (int i = 0; i < maxAttempts; ++i)
    {
        float angle = GetRandomValue(0, 359) * DEG2RAD;
        float dist  = targetDistance + GetRandomValue((int)-tolerance, (int)tolerance);

        int rx = (int)(start.x + cosf(angle) * dist);
        int ry = (int)(start.y + sinf(angle) * dist);

        if (rx < 0 || ry < 0 || rx >= dungeonWidth || ry >= dungeonHeight) continue;
        if (!walkable[rx][ry]) continue;
        if (IsTileOccupied(rx, ry, enemyPtrs, self)) continue;

        return { (float)rx, (float)ry };
    }
    return { -1, -1 };
}


// Try to build a RETREAT path with a cap on path length.
// Returns false if no acceptable path is found.
bool TrySetRetreatPath(
    const Vector2& startTile,
    const Vector2& playerTile,
    Character* self,
    std::vector<Vector3>& outPath,
    float targetDistance,     // e.g. 12
    float tolerance,          // e.g. 3
    int   maxAttempts,        // e.g. 100
    int   maxPathLen,         // e.g. 30   <-- HARD CAP
    int   maxShrinkSteps      // fallback: shrink distance band a bit
)
{
    if (isLoadingLevel) return false;

    auto tryPickAndBuild = [&](float dist)->bool { //lambda
        // 1) try biased-away
        Vector2 candidate = GetRetreatTileAwayFrom(startTile, playerTile, self, dist, tolerance, maxAttempts);
        if (candidate.x < 0) {
            // 2) fallback ring
            candidate = GetRetreatTile(startTile, self, dist, tolerance, maxAttempts);
        }
        if (candidate.x < 0) return false;

        if (!IsWalkable((int)candidate.x, (int)candidate.y, dungeonImg)) return false;

        std::vector<Vector2> tilePath = FindPath(startTile, candidate);
        if (tilePath.empty()) return false;

        // ❗ Cap the corridor distance (tiles along the path)
        if ((int)tilePath.size() > maxPathLen) return false;

        // Convert to world path
        outPath.clear();
        outPath.reserve(tilePath.size());
        for (const Vector2& t : tilePath) {
            Vector3 wp = GetDungeonWorldPos((int)t.x, (int)t.y, tileSize, dungeonPlayerHeight);
            wp.y += 80.0f; // your per-type offsets
            if (self && self->type == CharacterType::Pirate) wp.y = 160.0f;
            outPath.push_back(wp);
        }
        return !outPath.empty();
    };

    // Try ideal distance first
    if (tryPickAndBuild(targetDistance)) return true;

    // Gently shrink target distance if space is tight, each time respecting maxPathLen
    float dist = targetDistance;
    for (int i = 0; i < maxShrinkSteps; ++i) {
        dist = std::max(2.0f, dist * 0.75f);
        if (tryPickAndBuild(dist)) return true;
    }

    return false;
}




Vector2 GetRandomReachableTile(const Vector2& start, const Character* self, int maxAttempts) {
    int patrolRadius = 3;
    for (int i = 0; i < maxAttempts; ++i) {
        int rx = (int)start.x + GetRandomValue(-patrolRadius, patrolRadius);
        int ry = (int)start.y + GetRandomValue(-patrolRadius, patrolRadius);

        if (rx < 0 || ry < 0 || rx >= dungeonWidth || ry >= dungeonHeight)
            continue;

        if (!walkable[rx][ry]) continue;
        if (IsTileOccupied(rx, ry, enemyPtrs, self)) continue;

        Vector2 target = {(float)rx, (float)ry};
        if (!LineOfSightRaycast(start, target, dungeonImg, 100, 0.0f)) continue;
        
        return target;
    }

    // Return invalid coords if nothing found
    return {-1, -1};
}

bool TrySetRandomPatrolPath(const Vector2& start, Character* self, std::vector<Vector3>& outPath) {
    if (isLoadingLevel) return false;
    Vector2 randomTile = GetRandomReachableTile(start, self);

    if (!IsWalkable(randomTile.x, randomTile.y, dungeonImg)) return false;

    std::vector<Vector2> tilePath = FindPath(start, randomTile);
    if (tilePath.empty()) return false;

    outPath.clear();
    for (const Vector2& tile : tilePath) {
        Vector3 worldPos = GetDungeonWorldPos(tile.x, tile.y, tileSize, dungeonPlayerHeight);
        worldPos.y += 80.0f; //skeles at 80
        if (self->type == CharacterType::Pirate) worldPos.y = 160;
        outPath.push_back(worldPos);
    }

    return !outPath.empty();
}



bool HasWorldLineOfSight(Vector3 from, Vector3 to, float epsilonFraction, LOSMode mode)
{
    Ray ray = { from, Vector3Normalize(Vector3Subtract(to, from)) };
    float maxDistance = Vector3Distance(from, to);
    float epsilon = epsilonFraction * maxDistance;

    // Walls always block
    for (const WallRun& wall : wallRunColliders) {
        RayCollision hit = GetRayCollisionBox(ray, wall.bounds);
        if (hit.hit && hit.distance + epsilon < maxDistance) return false;
    }

    if (mode == LOSMode::AI) {
        // For AI vision, a CLOSED door panel blocks. (Open doors do not.)
        for (const Door& door : doors) {
            if (!door.isOpen) {
                RayCollision hit = GetRayCollisionBox(ray, door.collider);
                if (hit.hit && hit.distance + epsilon < maxDistance) return false;
            }
        }
        //also block by door side colliders for AI 
        for (const DoorwayInstance& dw : doorways) {
            for (const BoundingBox& sc : dw.sideColliders) {
                RayCollision hit = GetRayCollisionBox(ray, sc);
                if (hit.hit && hit.distance + epsilon < maxDistance) return false;
            }
        }
    } else { // LOSMode::Lighting
        // For lighting, let light leak through the middle of the doorway but block the sides.
        for (const DoorwayInstance& dw : doorways) {
            for (const BoundingBox& sc : dw.sideColliders) {
                RayCollision hit = GetRayCollisionBox(ray, sc);
                if (hit.hit && hit.distance + epsilon < maxDistance) return false;
            }
        }
        // NOTE: We intentionally do NOT test door panels here, so closed doors don't kill the glow. Doors leak light through so we
        //can have cool light through open door effect. Looks dumb with closed doors but that is the price we pay. Because recalculating
        //lights on door open isn't doable. 
    }

    return true;
}


Vector2 TileToWorldCenter(Vector2 tile) {
    return {
        tile.x + 0.5f * tileSize,
        tile.y + 0.5f * tileSize
    };
}

bool IsLavaTile(int x, int y)
{
    if (x < 0 || x >= dungeonWidth ||
        y < 0 || y >= dungeonHeight)
        return false;

    Color c = dungeonPixels[y * dungeonWidth + x];

    // Your lava color from the PNG
    return (c.a != 0 &&
            c.r == 200 &&
            c.g == 0 &&
            c.b == 0);
}


bool LineOfSightRaycast(Vector2 start, Vector2 end, const Image& dungeonMap, int maxSteps, float epsilon) {
    //raymarch the PNG map, do we use this? 
    const int numRays = 5;
    const float spread = 0.1f; // widen fan

    Vector2 dir = Vector2Normalize(Vector2Subtract(end, start));
    float distance = Vector2Length(Vector2Subtract(end, start));
    if (distance == 0) return true; 

    Vector2 perp = { -dir.y, dir.x };

    int blockedCount = 0;
    const int maxBlocked = 3;

    for (int i = 0; i < numRays; ++i) {
        
        float offset = ((float)i - numRays / 2) * (spread / (float)numRays);
        Vector2 offsetStart = Vector2Add(start, Vector2Scale(perp, offset));

        
        if (SingleRayBlocked(offsetStart, end, dungeonMap, maxSteps, epsilon)) {
            blockedCount++;
            if (blockedCount > maxBlocked) return false;
        }
    }

    return true;
}

bool SingleRayBlocked(Vector2 start, Vector2 end, const Image& dungeonMap, int maxSteps, float epsilon) {
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float distance = sqrtf(dx*dx + dy*dy);
    if (distance == 0) return false;

    float stepX = dx / distance;
    float stepY = dy / distance;

    float x = start.x;
    float y = start.y;



    for (int i = 0; i < distance && i < maxSteps; ++i) {
        int tileX = (int)x;
        int tileY = (int)y;

        if (tileX < 0 || tileX >= dungeonMap.width || tileY < 0 || tileY >= dungeonMap.height)
            return true;

        // Early out: if close enough to the end
        if (Vector2Distance((Vector2){x, y}, end) < epsilon) {
            return false;
        }

        Color c = GetImageColor(dungeonMap, tileX, tileY);

        if (c.r == 0 && c.g == 0 && c.b == 0) return true; // wall
        if (c.r == 255 && c.g == 255 && c.b == 0) return true; //light pedestal
        if (c.r == 128 && c.g == 0 && c.b == 128 && !IsDoorOpenAt(tileX, tileY)) return true; //closed door
        if (c.r == 0 && c.g == 255 && c.b == 255 && !IsDoorOpenAt(tileX, tileY)) return true; //locked closed door
        
        

        x += stepX;
        y += stepY;
    }

    return false;
}


// Supercover Bresenham LOS.
// Returns true ONLY if the straight line from start->end stays in walkable space.
// Uses runtime walkable grid via IsSeeThroughForLOS
bool TileLineOfSight(Vector2 start, Vector2 end, const Image& dungeonMap)
{
    int x0 = (int)start.x;
    int y0 = (int)start.y;
    int x1 = (int)end.x;
    int y1 = (int)end.y;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy;

    // First tile should normally be see-through (player stands on walkable tile).
    // If you ever cast from a non-walkable tile, you can relax this.
    if (!IsSeeThroughForLOS(x0, y0)) return false;

    while (!(x0 == x1 && y0 == y1))
    {
        int e2 = err * 2;

        int prevX = x0;
        int prevY = y0;

        bool stepX = false;
        bool stepY = false;

        if (e2 > -dy) { err -= dy; x0 += sx; stepX = true; }
        if (e2 <  dx) { err += dx; y0 += sy; stepY = true; }

        // Supercover corner handling: when we move diagonally, also check
        // the two orthogonal neighbors to avoid cutting corners through walls.
        if (stepX && stepY)
        {
            if (!IsSeeThroughForLOS(prevX + sx, prevY)) return false;
            if (!IsSeeThroughForLOS(prevX,       prevY + sy)) return false;
        }

        if (!IsSeeThroughForLOS(x0, y0)) return false;
    }

    return true;
}



// Tunables for tile-space smoothing
static constexpr int   kMaxLookaheadTiles = 1;   // how many nodes ahead to consider 
//--canceled out for now and see if it fixes stuck skeletons.
static constexpr int   kMaxLosSteps       = 80;  // ray steps for LOS (>= dungeon max dimension * 4 is fine)
static constexpr float kLosEpsilon        = 0.01f; // small epsilon for your raycast

std::vector<Vector2> SmoothTilePath(const std::vector<Vector2>& tilePath, const Image& dungeonMap)
{
    std::vector<Vector2> out;
    if (tilePath.empty()) return out;

    const size_t N = tilePath.size();
    size_t i = 0;

    while (i < N)
    {
        // Always keep current tile
        out.push_back(tilePath[i]);

        size_t furthest = i;
        const size_t maxJ = std::min(N - 1, i + (size_t)kMaxLookaheadTiles);

        // Search backwards from farthest candidate to nearest
        for (size_t j = maxJ; j > i; --j)
        {
            // If LOS says yes, we can skip straight there
            if (TileLineOfSight(tilePath[i], tilePath[j],
                                   dungeonMap))
            {
                furthest = j;
                break; // take the longest valid skip
            }
        }

        // Advance
        if (furthest == i) ++i;   // couldn't skip
        else i = furthest;        // jump to farthest visible
    }

    // Ensure destination present
    if (out.empty() || out.back().x != tilePath.back().x || out.back().y != tilePath.back().y)
        out.push_back(tilePath.back());

    return out;
}



// Tunables
static constexpr int   kMaxLookahead = 8;       // Try up to 14 nodes ahead
static constexpr float kMaxSkipDist  = 800.0f;  // ~7 tiles worth of distance
static constexpr float kEpsFraction  = 0.02f;    // More robust ray marching

std::vector<Vector3> SmoothWorldPath(const std::vector<Vector3>& worldPath)
{
    std::vector<Vector3> out;
    if (worldPath.empty()) return out;

    size_t i = 0;
    const size_t N = worldPath.size();

    while (i < N) {
        // Always keep the current point
        out.push_back(worldPath[i]);

        // Try to jump ahead greedily
        size_t furthest = i;
        const size_t maxJ = std::min(N - 1, i + static_cast<size_t>(kMaxLookahead));

        for (size_t j = maxJ; j > i + 0; --j) {
            const float dist = Vector3Distance(worldPath[i], worldPath[j]);
            if (dist > kMaxSkipDist) continue;

            if (HasWorldLineOfSight(worldPath[i], worldPath[j], kEpsFraction)) {
                furthest = j;
                break; // take the longest valid skip we found
            }
        }

        if (furthest == i) {
            // Couldn’t skip—advance one to avoid infinite loop
            ++i;
        } else {
            // Jump to the furthest visible point
            i = furthest;
        }
    }

    // Ensure destination is included (in case the loop ended on a mid point)
    if (out.back().x != worldPath.back().x ||
        out.back().y != worldPath.back().y ||
        out.back().z != worldPath.back().z) {
        out.push_back(worldPath.back());
    }

    return out;
}


//Raptor steering behavior
Vector3 SeekXZ(const Vector3& pos, const Vector3& target, float maxSpeed) {
    // Fast “point at it”
    Vector3 dir = NormalizeXZ(Vector3Subtract(target, pos));
    return Vector3Scale(dir, maxSpeed);
}

// Ease in within slowRadius
Vector3 ArriveXZ(const Vector3& pos, const Vector3& target,
                        float maxSpeed, float slowRadius)
{
    Vector3 toT = Vector3Subtract(target, pos); toT.y = 0.0f;
    float d = sqrtf(toT.x*toT.x + toT.z*toT.z);
    if (d < 1e-4f) return {0,0,0};
    float t = fminf(1.0f, d / slowRadius);             // 0..1
    float speed = fmaxf(0.0f, maxSpeed * t);           // ramp down near target
    return Vector3Scale({toT.x/d, 0.0f, toT.z/d}, speed);
}

Vector3 FleeXZ(const Vector3& pos, const Vector3& threat, float maxSpeed) {
    Vector3 dir = NormalizeXZ(Vector3Subtract(pos, threat));
    return Vector3Scale(dir, maxSpeed);
}

// orbitRadius: desired ring distance from target
// clockwise: +1 for CW, -1 for CCW
// tangentGain: how strongly to circle (0..1 usually)
// radialGain: how strongly to correct toward the ring
Vector3 OrbitXZ(const Vector3& pos, const Vector3& target,
                       float orbitRadius, int clockwise,
                       float tangentGain, float radialGain,
                       float maxSpeed)
{
    // Radial from raptor -> target
    Vector3 r = Vector3Subtract(target, pos); r.y = 0.0f;
    float d = sqrtf(r.x*r.x + r.z*r.z);
    if (d < 1e-4f) return {0,0,0};
    Vector3 rN = { r.x/d, 0.0f, r.z/d };

    // Tangent: rotate radial ±90° in XZ
    Vector3 tN = (clockwise >= 0) ? Vector3{ rN.z, 0.0f, -rN.x }
                                  : Vector3{ -rN.z, 0.0f,  rN.x };

    // Radial correction pushes toward the ring (out if too close, in if too far)
    float radialErr = (d - orbitRadius);           // positive if too far
    Vector3 radialVel = Vector3Scale(rN, -radialErr * radialGain); // pull toward ring

    // Tangential circling velocity
    Vector3 tangentialVel = Vector3Scale(tN, tangentGain * maxSpeed);

    // Blend and clamp
    Vector3 v = Vector3Add(tangentialVel, radialVel);
    return Limit(v, maxSpeed);
}

// Keep a wanderAngle per-raptor; call each frame.
Vector3 WanderXZ(float& wanderAngle, float wanderTurnRate, float wanderSpeed, float dt) {
    // nudge angle
    wanderAngle += ((float)GetRandomValue(-1000,1000) / 1000.0f) * wanderTurnRate * dt;
    float s = sinf(wanderAngle), c = cosf(wanderAngle);
    return { s * wanderSpeed, 0.0f, c * wanderSpeed };
}

// returns true if movement was blocked by water this frame
bool StopAtWaterEdge(const Vector3& pos,
                            Vector3& desiredVel,     // in/out
                            float waterLevel,
                            float dt)
{
    // if we’re not moving, nothing to do
    float v2 = desiredVel.x*desiredVel.x + desiredVel.z*desiredVel.z;
    if (v2 < 1e-4f) return false;

    // Look a short distance ahead in the movement direction (XZ only)
    Vector3 dir = { desiredVel.x, 0.0f, desiredVel.z };
    float  speed = sqrtf(v2);
    float  look  = fmaxf(100.0f, speed * 0.4f); // small peek ahead; tweak

    Vector3 ahead = { pos.x + dir.x / speed * look,
                      pos.y,
                      pos.z + dir.z / speed * look };

    // Sample terrain height at the ahead point
    float hAhead = GetHeightAtWorldPosition(ahead, heightmap, terrainScale);

    if (hAhead <= waterLevel) {
        // hard stop at the shoreline
        desiredVel.x = desiredVel.z = 0.0f;
        return true;
    }
    return false;
}

bool IsWaterAtXZ(float x, float z, float waterLevel) {
    Vector3 waterPos = {x, waterLevel, z};
    return GetHeightAtWorldPosition(waterPos, heightmap, terrainScale) <= waterLevel;
}





