#include "box.h"
#include "raymath.h"
#include "player.h"

static Vector3 ForwardFromYawPitch(float pitchDeg, float yawDeg)
{
    float pitch = pitchDeg * DEG2RAD;
    float yaw   = yawDeg   * DEG2RAD;

    Vector3 f;
    f.x = sinf(yaw) * cosf(pitch);
    f.y = -sinf(pitch);
    f.z = cosf(yaw) * cosf(pitch);

    return Vector3Normalize(f);
}

Box::Box(BoxType t, Vector3 pos)
{
    type     = t;
    position = pos;

    UpdateBounds();
}

void Box::Update(float dt,
                 Vector3 playerPos,
                 Vector3 playerForward,
                 Vector2 playerRot,
                 bool interactPressed,
                 bool dropPressed,
                 Vector3 dropTileCenter)
{
    // --- Pick up ---
    if (state == BoxState::OnGround)
    {
        if (interactPressed && CanPickup(playerPos))
        {
            Pickup();
        }
    }

    // --- While carried ---
    if (state == BoxState::Carried)
    {

        Vector3 lookFwd = ForwardFromYawPitch(playerRot.x, playerRot.y);

        // Carry position follows look direction
        Vector3 target = Vector3Add(playerPos, Vector3Scale(lookFwd, carryOffset.z));

        // Lock to waist height
        target.y = playerPos.y - carryOffset.y;

        position = target;


        // Drop (snapped to tile center)
        if (dropPressed || interactPressed) // whichever you prefer: same key toggles, or separate
        {
            DropToTileCenter(dropTileCenter);
        }
    }

    UpdateBounds();

}


bool Box::CanPickup(Vector3 playerPos) const
{
    if (!canBeCarried) return false;
    if (state != BoxState::OnGround) return false;

    float distSq = Vector3DistanceSqr(playerPos, position);
    return distSq <= pickupRadius * pickupRadius;
}

void Box::Pickup()
{
    state = BoxState::Carried;
}

void Box::DropToTileCenter(Vector3 tileCenter)
{
    position = tileCenter;
    state    = BoxState::OnGround;
}

void Box::UpdateBounds()
{
    float half = 50.0f * scale;

    bounds.min = {
        position.x - half,
        position.y,
        position.z - half
    };

    bounds.max = {
        position.x + half,
        position.y + (200.0f * scale),
        position.z + half
    };
}
