#include "spiderEgg.h"
#include "raymath.h"
#include "transparentDraw.h"
#include <algorithm>

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

    float eggHeight = 100.0f;
    egg.collider.min = { pos.x - 50, pos.y,          pos.z - 50 };
    egg.collider.max = { pos.x + 50, pos.y + eggHeight, pos.z + 50 };

    eggs.push_back(egg);
    return eggs.back();
}

static void UpdateEggAnimation(SpiderEgg& egg, float dt) {
    egg.frameTimer += dt;
    if (egg.frameTimer >= egg.frameTime) {
        egg.frameTimer = 0.0f;
        egg.currentFrame++;
        if (egg.currentFrame >= egg.framesPerRow) {
            egg.currentFrame = 0;
        }
    }
}


// ---------------------------------------------------------
void UpdateSpiderEggs(float dt, const Vector3& playerPos)
{
    for (SpiderEgg& egg : eggs) {
        // Hit flash timer
        egg.hitFlashTimer = std::max(0.0f, egg.hitFlashTimer - dt);

        // Trigger condition (simple distance for now)
        float dist = Vector3Distance(playerPos, egg.position);
        if (!egg.triggered && dist < 300.0f) {
            egg.triggered = true;
            egg.hatchTimer = egg.hatchDelay;
            egg.state = SpiderEggState::Awakening;
        }

        // Hatching countdown
        if (egg.triggered && egg.state != SpiderEggState::Destroyed) {
            if (egg.hatchTimer > 0.0f) {
                egg.hatchTimer -= dt;
                if (egg.hatchTimer <= 0.0f) {
                    egg.state = SpiderEggState::Hatching;
                    // You will spawn spider when needed
                }
            }
        }
        // Choose row based on state
        egg.currentRow = GetRowForState(egg);

        UpdateEggAnimation(egg, dt);
    }
}