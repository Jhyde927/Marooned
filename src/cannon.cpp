#include "cannon.h"

#include "player.h"
#include "world.h"
#include "raymath.h"
#include "sound_manager.h"

// Include whatever header has your fireBullet function.
// Example:
// #include "bullet.h"
// #include "world.h"
// #include "resources.h"

namespace
{
    constexpr float kDegToRad = PI / 180.0f;

    // Tweak these until the cannon fires from the right spot
    constexpr float kMuzzleForwardOffset = 140.0f;
    constexpr float kMuzzleUpOffset = 55.0f;

    constexpr float kInteractionBoxHalfX = 90.0f;
    constexpr float kInteractionBoxHalfY = 70.0f;
    constexpr float kInteractionBoxHalfZ = 90.0f;

    constexpr float kCannonballSpeed = 2600.0f;
}

void Cannon::Init(const Vector3& pos, float yaw)
{
    position = pos;
    yawDeg = yaw;
    isLoaded = false;
    isCoolingDown = false;
    cooldownTimer = 0.0f;
    recoilTimer = 0.0f;
}

void Cannon::Update(float dt, Player& player)
{
    UpdateTimers(dt);



    // Minimal interact example:
    // If player is close and presses E, fire if loaded.
    // Loading itself can be triggered elsewhere when a cannonball box is dropped nearby.
    if (IsPlayerInRange(player) && IsKeyPressed(KEY_E))
    {
        if (CanFire())
        {
            Fire();
        }else
        {
            if (!isLoaded && player.isCarrying && player.carriedBox)
            {
                Load();
                RemoveCarriedBox(player, boxes);
                
            }
        }
    }
}

void Cannon::Draw() const
{

    // Optional: small fake recoil by shifting backward while recoilTimer is active
    Vector3 drawPos = position;

    if (recoilTimer > 0.0f)
    {
        Vector3 forward = GetForward();
        drawPos = Vector3Subtract(drawPos, Vector3Scale(forward, 12.0f));
    }

    DrawModelEx(
        R.GetModel("cannon"),
        drawPos,
        Vector3{ 0.0f, 1.0f, 0.0f },
        yawDeg,
        Vector3{ 25.0f, 25.0f, 25.0f },
        GRAY
    );
    

}

bool Cannon::IsPlayerInRange(const Player& player, float extraRange) const
{
    float range = interactRange + extraRange;
    float rangeSq = range * range;

    Vector3 toPlayer = Vector3Subtract(player.position, position);
    toPlayer.y = 0.0f;

    return Vector3LengthSqr(toPlayer) <= rangeSq;
}

bool Cannon::CanLoad() const
{
    return !isLoaded && !isCoolingDown;
}

bool Cannon::CanFire() const
{
    return isLoaded && !isCoolingDown;
}

void Cannon::Load()
{
    if (!CanLoad()) return;

    isLoaded = true;
}

void Cannon::Fire()
{
    if (!CanFire()) return;

    Vector3 muzzlePos = GetMuzzlePosition();
    Vector3 dir = GetForward();
    Vector3 velocity = Vector3Scale(dir, kCannonballSpeed);
    Vector3 target = Vector3Add(muzzlePos, Vector3Scale(dir, 200.0f));

    FireCannon(muzzlePos, target, 2000.0f, 5.0f, false);
    SoundManager::GetInstance().PlaySoundAtPosition("CannonShot", muzzlePos, player.position, 0.0f, 3000.0f);
    
    isLoaded = false;
    isCoolingDown = true;
    cooldownTimer = cooldownTime;
    recoilTimer = recoilDuration;
}

Vector3 Cannon::GetMuzzlePosition() const
{
    Vector3 forward = GetForward();

    Vector3 muzzle = position;
    muzzle = Vector3Add(muzzle, Vector3Scale(forward, kMuzzleForwardOffset));
    muzzle.y += kMuzzleUpOffset;

    return muzzle;
}

Vector3 Cannon::GetForward() const
{
    float yawRad = (yawDeg - 90.0f)* kDegToRad;

    Vector3 forward = {
        sinf(yawRad),
        0.0f,
        cosf(yawRad)
    };

    return Vector3Normalize(forward);
}

BoundingBox Cannon::GetInteractionBox() const
{
    BoundingBox box{};
    box.min = {
        position.x - kInteractionBoxHalfX,
        position.y - kInteractionBoxHalfY,
        position.z - kInteractionBoxHalfZ
    };
    box.max = {
        position.x + kInteractionBoxHalfX,
        position.y + kInteractionBoxHalfY,
        position.z + kInteractionBoxHalfZ
    };
    return box;
}

void Cannon::UpdateTimers(float dt)
{
    if (isCoolingDown)
    {
        cooldownTimer -= dt;
        if (cooldownTimer <= 0.0f)
        {
            cooldownTimer = 0.0f;
            isCoolingDown = false;
        }
    }

    if (recoilTimer > 0.0f)
    {
        recoilTimer -= dt;
        if (recoilTimer < 0.0f)
        {
            recoilTimer = 0.0f;
        }
    }
}