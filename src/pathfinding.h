#pragma once
#include "raylib.h"
#include <vector>

enum class LOSMode { Lighting, AI };

extern std::vector<std::vector<bool>> walkable;
class Character;
void ConvertImageToWalkableGrid(const Image& dungeonMap);
Vector2 WorldToImageCoords(Vector3 worldPos);
bool IsWalkable(int x, int y, const Image& dungeonMap);
bool IsSeeThroughForLOS(int x, int y);
bool IsLavaTile(int x, int y);
bool IsTileOccupied(int x, int y, const std::vector<Character*>& skeletons, const Character* self);
Character* GetTileOccupier(int x, int y, const std::vector<Character*>& skeletons, const Character* self);
Vector2 TileToWorldCenter(Vector2 tile);
bool HasWorldLineOfSight(Vector3 from, Vector3 to, float epsilonFraction = 0.0f, LOSMode mode = LOSMode::AI);
bool LineOfSightRaycast(Vector2 start, Vector2 end, const Image& dungeonMap, int maxSteps, float epsilon);
bool SingleRayBlocked(Vector2 start, Vector2 end, const Image& dungeonMap, int maxSteps, float epsilon);
bool TileLineOfSight(Vector2 start, Vector2 end, const Image& dungeonMap);
Vector2 GetRandomReachableTile(const Vector2& start, const Character* self, int maxAttempts = 100);
bool TrySetRandomPatrolPath(const Vector2& start, Character* self, std::vector<Vector3>& outPath);
bool TrySetRetreatPath(const Vector2& startTile, const Vector2& playerTile, Character* self, std::vector<Vector3>& outPath, 
    float targetDistance,     // e.g. 12
    float tolerance,          // e.g. 3
    int   maxAttempts,        // e.g. 100
    int   maxPathLen,         // e.g. 30   <-- HARD CAP
    int   maxShrinkSteps      // fallback: shrink distance band a bit
);

std::vector<Vector2> SmoothTilePath(const std::vector<Vector2>& tilePath, const Image& dungeonMap);
std::vector<Vector3> SmoothWorldPath(const std::vector<Vector3>& worldPath);
std::vector<Vector2> FindPath(Vector2 start, Vector2 goal);

//raptor steering
Vector3 ArriveXZ(const Vector3& pos, const Vector3& target, float maxSpeed, float slowRadius);
Vector3 SeekXZ(const Vector3& pos, const Vector3& target, float maxSpeed);
Vector3 FleeXZ(const Vector3& pos, const Vector3& threat, float maxSpeed);
Vector3 WanderXZ(float& wanderAngle, float wanderTurnRate, float wanderSpeed, float dt);
Vector3 OrbitXZ(const Vector3& pos, const Vector3& target, float orbitRadius, int clockwise,float tangentGain, float radialGain,float maxSpeed);

bool StopAtWaterEdge(const Vector3& pos,Vector3& desiredVel, float waterLevel, float dt);
bool IsWaterAtXZ(float x, float z, float waterLevel);