#include "spiderEgg.h"
#include "raymath.h"
#include "transparentDraw.h"
#include <algorithm>
#include "dungeonGeneration.h"
#include "sound_manager.h"
#include "pathfinding.h"

std::vector<SpiderEgg> eggs;

void ClearSpiderEggs() {
    eggs.clear();
}



static int GetRowForState(const SpiderEgg& egg) {
    switch (egg.state) {
        case SpiderEggState::Dormant:
        case SpiderEggState::Awakening:
            return egg.rowDormant;    // same visuals for now

        case SpiderEggState::Hatching:
            return egg.rowHatching;

        case SpiderEggState::Destroyed:
            return egg.rowDestroyed;

        case SpiderEggState::Husk:
            return egg.rowHusk;
    }
    return egg.rowDormant;
}


SpiderEgg& SpawnSpiderEgg(Vector3 pos,
                          Texture2D texture,
                          int frameW,
                          int frameH,
                          int framesPerRow,
                          float scale)
{
    SpiderEgg egg{};
    
    egg.position      = pos;
    egg.texture       = texture;
    egg.frameWidth    = frameW;
    egg.frameHeight   = frameH;
    egg.framesPerRow  = framesPerRow;
    egg.scale         = scale;
    egg.gooEmitter.SetPosition(egg.position);
    float eggHeight = 100.0f;
    egg.collider.min = { pos.x - 50, pos.y,          pos.z - 50 };
    egg.collider.max = { pos.x + 50, pos.y + eggHeight, pos.z + 50 };

    egg.maxHealth = 100;
    egg.health = egg.maxHealth;

    eggs.push_back(egg);
    return eggs.back();
}

void UpdateEggAnimation(SpiderEgg& egg, float dt, Vector3 playerPos) {
    egg.frameTimer += dt;
    if (egg.frameTimer >= egg.frameTime) {
        egg.frameTimer = 0.0f;
        egg.currentFrame++;

        int maxFramesThisRow = egg.framesPerRow; // full row for now

        if (egg.state == SpiderEggState::Hatching) {
            // One-shot animation: when we run out of frames, go to Husk
            if (egg.currentFrame >= maxFramesThisRow) {
                egg.currentFrame = 0;                 // start at frame 0 of husk row
                egg.state = SpiderEggState::Husk;     // next update GetRowForState will map to husk
                // Spawn spider here, once

                if (!egg.spiderSpawned) {
                    Vector3 spawnPos = egg.position;
                    SoundManager::GetInstance().PlaySoundAtPosition("eggHatch", egg.position, playerPos, 0.0f, 3000.0f);
                    // Push the spider toward the player so it appears in front
                    Vector3 toPlayer = Vector3Subtract(playerPos, egg.position);
                    if (Vector3Length(toPlayer) > 0.001f) {
                        toPlayer = Vector3Normalize(toPlayer);
                        float forwardOffset = 60.0f; // about 0.3 tile in your 200-unit world
                        spawnPos.x += toPlayer.x * forwardOffset;
                        spawnPos.z += toPlayer.z * forwardOffset;
                    }

                    spawnPos.y = egg.position.y;

                    SpawnSpiderFromEgg(spawnPos);
                    egg.spiderSpawned = true;
                    egg.hatchTimer = 0.0f;
                }

            }
        } else {
            // Normal looping for other states
            if (egg.currentFrame >= maxFramesThisRow) {
                egg.currentFrame = 0;
            }
        }
    }
}

// ---------------------------------------------------------
void UpdateSpiderEggs(float dt, const Vector3& playerPos)
{
    
    for (SpiderEgg& egg : eggs) {
        egg.gooEmitter.UpdateBlood(dt);
        //check for player LOS and distance
        float distanceTo = Vector3Distance(egg.position, playerPos);
        if (HasWorldLineOfSight(egg.position, playerPos, 0.1) && distanceTo < 3000.0f && !egg.triggered && egg.state != SpiderEggState::Destroyed){
            egg.triggered = true;
            egg.hatchTimer = egg.hatchDelay;
            egg.state = SpiderEggState::Awakening;
        }

        // Hit flash timer
        egg.hitFlashTimer = std::max(0.0f, egg.hitFlashTimer - dt);
        egg.flashCooldown  = std::max(0.0f, egg.flashCooldown - dt);

        // Trigger condition (simple distance for now)
        float dist = Vector3Distance(playerPos, egg.position);
        if (!egg.triggered && dist < 300.0f && egg.state != SpiderEggState::Destroyed) {
            egg.triggered = true;
            egg.hatchTimer = egg.hatchDelay;
            egg.state = SpiderEggState::Awakening;
        }



        // Hatching countdown
        if (egg.triggered && egg.state != SpiderEggState::Destroyed) {
            if (egg.hatchTimer > 0.0f) {
                egg.hatchTimer -= dt;
                if (egg.hatchTimer <= 0.0f && !egg.spiderSpawned) {
                    egg.state = SpiderEggState::Hatching;
                    egg.currentFrame = 0;    // start at beginning of hatching anim
                    egg.frameTimer   = 0.0f;


                }
            }
        }

        // Awakening pulse logic
        if (egg.state == SpiderEggState::Awakening) {

            // If we are NOT red and cooldown is over → pulse again
            if (egg.hitFlashTimer <= 0.0f && egg.flashCooldown <= 0.0f) {
                egg.hitFlashTimer = 0.60f;   // time tinted red
                egg.flashCooldown = 0.80f;   // delay before next pulse starts
            }
        }


        // Choose row based on state
        egg.currentRow = GetRowForState(egg);

        UpdateEggAnimation(egg, dt, playerPos);
    }
}

void DamageSpiderEgg(SpiderEgg& egg, float amount, Vector3 playerPos)
{
    // Already dead? Ignore.
    if (egg.state == SpiderEggState::Destroyed)
        return;

    egg.health -= amount;
    if (egg.health < 0.0f) egg.health = 0.0f;

    // Immediate red flash on hit (overrides awakening pulse briefly)
    egg.hitFlashTimer = 0.12f;
    egg.flashCooldown = 0.15f; // small delay before any awakening pulse resumes

    // If the egg dies, kill it for good: no more hatching
    if (egg.health <= 0.0f) {
        egg.state      = SpiderEggState::Destroyed;
        egg.triggered  = false;
        egg.hatchTimer = 0.0f;

        // Optional: reset animation so destroyed row starts at frame 0
        egg.currentFrame = 0;
        SoundManager::GetInstance().PlaySoundAtPosition("squish", egg.position, playerPos, 0.0f, 3000);

        // Toward the *camera/player* in world space
        Vector3 toPlayer = Vector3Normalize(Vector3Subtract(playerPos, egg.position));
        Vector3 newPos   = Vector3Add(egg.position, Vector3Scale(toPlayer, 100.0f)); // 100 units in front of the enemy
        egg.gooEmitter.EmitBlood(newPos, 25, GREEN);

    }
}
