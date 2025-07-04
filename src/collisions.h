#pragma once

#include "raylib.h"
#include "player.h"
#include "character.h"
#include "bullet.h"
#include "vegetation.h"

void CheckBulletHits(Camera& camera);
void HandleMeleeHitboxCollision(Camera& camera);
bool CheckCollisionPointBox(Vector3 point, BoundingBox box);
void HandleDoorInteraction(Camera& camera);
void DoorCollision();
void TreeCollision(Camera& camera);
bool CheckBulletHitsTree(const TreeInstance& tree, const Vector3& bulletPos);
void ResolveBoxSphereCollision(const BoundingBox& box, Vector3& position, float radius);
void WallCollision();
void barrelCollision();
void ChestCollision();
void pillarCollision(); 