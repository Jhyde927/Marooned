#pragma once

#include "raylib.h"
#include "player.h"
#include "character.h"
#include "bullet.h"
#include "vegetation.h"

void UpdateCollisions(Camera& camera);
void CheckBulletHits(Camera& camera);
void HandleMeleeHitboxCollision(Camera& camera);
bool CheckCollisionPointBox(Vector3 point, BoundingBox box);
void HandleDoorInteraction(Camera& camera);
void DoorCollision();
void TreeCollision(Camera& camera);
bool CheckTreeCollision(const TreeInstance& tree, const Vector3& playerPos);
void ResolveTreeCollision(const TreeInstance& tree, Vector3& playerPos);
void HandleEnemyPlayerCollision(Player* player);
bool CheckBulletHitsTree(const TreeInstance& tree, const Vector3& bulletPos);
void ResolveBoxSphereCollision(const BoundingBox& box, Vector3& position, float radius);
void ResolvePlayerEnemyMutualCollision(Character* enemy, Player* player);
bool ResolveBoxBoxCollisionXZ(const BoundingBox& enemyBox, const BoundingBox& wallBox, Vector3& enemyPos);
void SwitchCollision();
void WallCollision();
void barrelCollision();
void SpiderWebCollision();
void ChestCollision();
void pillarCollision(); 
void launcherCollision();
void SpiderEggCollision();
void BulletParticleBounce(Bullet& b, Color c);
void BulletParticleRicochetNormal(Bullet& b, Vector3 n, Color c);
bool HandleBarrelHitsForBullet(Bullet& b, Camera& camera);
bool TryBulletRicochet(Bullet& b, Vector3 n, float damp = 0.6f, float minSpeed = 80.0f, float headOnCosThreshold = 0.999f);