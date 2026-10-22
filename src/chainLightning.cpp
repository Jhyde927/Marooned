#include "chainLightning.h"
#include "raymath.h"
#include <algorithm>
#include "rlgl.h"
#include "sound_manager.h"
#include "world.h"
#include "pathfinding.h"
#include "collisions.h"

ChainLightning::ChainLightning(Vector3 origin, Vector3 targetPoint, Character* target, float initialDamage)
    : damage(initialDamage)
{
    AddBolt(origin, targetPoint);
    lastHitPosition = targetPoint;

    SoundManager::GetInstance().PlaySoundAtPosition("lightning", origin, player.position, 0.0f, 3000.0f);
    SoundManager::GetInstance().PlaySoundAtPosition("lightningHum", origin, player.position, 0.0f, 3000.0f);

    if (target) {
        hitTargets.push_back(target);
        target->TakeDamage(damage); // Tune damage later.
    }
    else {
        finishedChaining = true;
    }
}

void DamageDestructiblesAlongBolt(LightningBolt& bolt){

}

void ChainLightning::Update(float deltaTime)
{
    if (!active) return;

    for (LightningBolt& bolt : bolts) {
        float previousAge = bolt.age;
        bolt.age += deltaTime;

        if (previousAge < bolt.lifetime && bolt.age >= bolt.lifetime) {
            SoundManager::GetInstance().PlaySoundAtPosition("lightningHit", bolt.end, player.position, 0.0f, 3000.0f);

        }
    }


    
    bolts.erase(
        std::remove_if(bolts.begin(), bolts.end(),
            [](const LightningBolt& bolt) {
                return bolt.age >= bolt.lightLifetime;
            }),
        bolts.end()
    );

    if (!finishedChaining) {
        jumpTimer += deltaTime;

        if (jumpTimer >= jumpInterval) {
            jumpTimer -= jumpInterval;

            if (jumpsRemaining > 0 && TryNextJump()) {
                --jumpsRemaining;

                if (jumpsRemaining == 0) {
                    finishedChaining = true;
                }
            }
            else {
                finishedChaining = true;
            }
        }
    }

    // Allow the final bolts to finish fading before removing the cast.
    if (finishedChaining && bolts.empty()) {
        active = false;
    }
}

void ChainLightning::AddBolt(Vector3 start, Vector3 end)
{
    LightningBolt bolt;
    bolt.start = start;
    bolt.end = end;

    GenerateBoltPoints(bolt);
    bolts.push_back(bolt);

    // TODO: Play a crack sound here, quieter with each jump.
}

void ChainLightning::GenerateBoltPoints(LightningBolt& bolt)
{
    bolt.points.clear();

    Vector3 direction = Vector3Subtract(bolt.end, bolt.start);
    float length = Vector3Length(direction);

    if (length < 0.001f) {
        bolt.points.push_back(bolt.start);
        bolt.points.push_back(bolt.end);
        return;
    }

    direction = Vector3Normalize(direction);

    // Choose a reference axis that isn't parallel to the bolt.
    Vector3 reference = { 0.0f, 1.0f, 0.0f };
    if (direction.y > 0.99f || direction.y < -0.99f) {
        reference = { 1.0f, 0.0f, 0.0f };
    }

    Vector3 right = Vector3Normalize(Vector3CrossProduct(direction, reference));
    Vector3 up = Vector3Normalize(Vector3CrossProduct(right, direction));

    const int segments = 8;
    const float jaggedness = 24.0f;
    bolt.lifetime = 0.2f;

    bolt.points.push_back(bolt.start);

    for (int i = 1; i < segments; ++i) {
        float t = static_cast<float>(i) / segments;

        Vector3 point = Vector3Lerp(bolt.start, bolt.end, t);

        float sideways = GetRandomValue(-1000, 1000) / 1000.0f;
        float vertical = GetRandomValue(-1000, 1000) / 1000.0f;

        point = Vector3Add(point, Vector3Scale(right, sideways * jaggedness));
        point = Vector3Add(point, Vector3Scale(up, vertical * jaggedness));

        bolt.points.push_back(point);
    }

    bolt.points.push_back(bolt.end);
}



bool ChainLightning::TryNextJump()
{
    const float jumpRange = 2600.0f;
    float closestDistance = jumpRange;

    Character* target = nullptr;
    Vector3 targetPoint = {};

    for (Character* enemy : enemyPtrs) {
        if (!enemy) continue;
        if (enemy->isDead) continue;
        // Add the same dead-enemy check used in FireChainLightning().

        if (std::find(hitTargets.begin(), hitTargets.end(), enemy) != hitTargets.end())
            continue;

        Vector3 enemyPosition = enemy->position;
        float distance = Vector3Distance(lastHitPosition, enemyPosition);

        if (distance >= closestDistance) continue;
        if (isDungeon && !DDAHasLineOfSightWorld(lastHitPosition, enemyPosition)) continue;

        closestDistance = distance;
        target = enemy;
        targetPoint = enemyPosition;
    }

    if (!target) return false;
    damage *= 0.5f;
    hitTargets.push_back(target);
    target->TakeDamage(damage);

    AddBolt(lastHitPosition, targetPoint);
    lastHitPosition = targetPoint;

    SoundManager::GetInstance().StartPositionalSound(
        "lightningHit",
        targetPoint,
        player.position,
        player.lookForward,
        3000.0f
    );

    return true;
}

void ChainLightning::Draw(const Camera3D& camera, const Texture2D& texture) const
{
    const float halfWidth = 16.0f;

    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);

    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);

    for (const LightningBolt& bolt : bolts) {
        float fade = Clamp(1.0f - bolt.age / bolt.lifetime, 0.0f, 1.0f);
        Color boltColor = { 30, 120, 255, 255 };
        Color tint = Fade(boltColor, fade);

        rlColor4ub(tint.r, tint.g, tint.b, tint.a);

        for (size_t i = 1; i < bolt.points.size(); ++i) {
            Vector3 start = bolt.points[i - 1];
            Vector3 end = bolt.points[i];

            Vector3 direction = Vector3Subtract(end, start);
            if (Vector3LengthSqr(direction) < 0.0001f) continue;
            direction = Vector3Normalize(direction);

            Vector3 midpoint = Vector3Scale(Vector3Add(start, end), 0.5f);
            Vector3 toCamera = Vector3Subtract(camera.position, midpoint);
            Vector3 side = Vector3CrossProduct(direction, toCamera);

            // Fallback when looking directly along the segment.
            if (Vector3LengthSqr(side) < 0.0001f) {
                side = Vector3CrossProduct(direction, camera.up);
            }
            if (Vector3LengthSqr(side) < 0.0001f) continue;

            side = Vector3Scale(Vector3Normalize(side), halfWidth);

            Vector3 a = Vector3Subtract(start, side);
            Vector3 b = Vector3Add(start, side);
            Vector3 c = Vector3Add(end, side);
            Vector3 d = Vector3Subtract(end, side);

            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(a.x, a.y, a.z);
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(b.x, b.y, b.z);
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(c.x, c.y, c.z);
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(d.x, d.y, d.z);

        }
    }

    rlEnd();
    rlSetTexture(0);

    // Flush while depth writing is still disabled.
    rlDrawRenderBatchActive();
    EndBlendMode();
    rlEnableDepthMask();
}

// void ChainLightning::Draw(const Camera3D& camera) const
// {
//     (void)camera; // Unused until we implement camera-facing ribbons.

//     // Temporary visualization of each bolt's connected points.
//     for (const LightningBolt& bolt : bolts) {
//         float fade = 1.0f - bolt.age / bolt.lifetime;
//         Color tint = Fade(SKYBLUE, fade);

//         for (size_t i = 1; i < bolt.points.size(); ++i) {
//             DrawLine3D(bolt.points[i - 1], bolt.points[i], tint);
//         }
//     }
// }