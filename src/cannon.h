#pragma once

#include "raylib.h"
#include <string>
#include "player.h"
#include "resourceManager.h"

// If your project already has a bullet or weapon system header,
// include that in cannon.cpp instead of here to keep this lightweight.

class Cannon
{
public:
    Cannon() = default;

    void Init(const Vector3& pos, float yawDeg);
    void Update(float dt, Player& player);
    void Draw() const;

    // Interaction
    bool IsPlayerInRange(const Player& player, float extraRange = 0.0f) const;
    bool CanLoad() const;
    bool CanFire() const;

    void Load();
    void Fire();

    // Helpers
    Vector3 GetMuzzlePosition() const;
    Vector3 GetForward() const;
    BoundingBox GetInteractionBox() const;

    Vector3 position{ 0.0f, 0.0f, 0.0f };
    float yawDeg = 0.0f;

    bool isLoaded = false;
    bool isCoolingDown = false;

    float interactRange = 400.0f;
    float cooldownTime = 1.5f;

    // Optional visual feedback
    float recoilTimer = 0.0f;
    float recoilDuration = 0.2f;


private:
    float cooldownTimer = 0.0f;

    void UpdateTimers(float dt);
};