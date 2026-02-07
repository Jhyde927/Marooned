#include "box.h"
#include "raymath.h"
#include <cmath>
#include "player.h"
#include "dungeonGeneration.h"
#include "pathfinding.h"
#include "utilities.h"


static float ClampAxisAgainstWalls(
    float startAxis,                // previous axis value (x or z)
    float desiredAxis,              // target axis value
    Vector3 basePos,                // position with the OTHER axis already resolved
    bool clampX,                    // true = X axis, false = Z axis
    const Box& box,
    const std::vector<BoundingBox>& walls)
{
    float outAxis = desiredAxis;

    // Early out if no movement
    float delta = desiredAxis - startAxis;
    if (fabsf(delta) < 0.0001f)
        return startAxis;

    // Box extents (must match UpdateBounds)
    const float half = 50.0f * box.scale;
    const float skin = 1.0f; // prevents jitter / z-fighting

    // Candidate position
    Vector3 pos = basePos;
    if (clampX) pos.x = outAxis;
    else        pos.z = outAxis;

    auto MakeAABB = [&](Vector3 p) {
        BoundingBox bb;
        bb.min = { p.x - half, p.y,       p.z - half };
        bb.max = { p.x + half, p.y + 100.0f * box.scale, p.z + half };
        return bb;
    };

    BoundingBox boxBB = MakeAABB(pos);

    for (const BoundingBox& wall : walls)
    {
        if (!CheckCollisionBoxes(boxBB, wall))
            continue;

        // We collided — clamp against the wall
        if (clampX)
        {
            if (delta > 0.0f)
            {
                // Moving +X → clamp our max.x to wall.min.x
                outAxis = wall.min.x - half - skin;
            }
            else
            {
                // Moving -X → clamp our min.x to wall.max.x
                outAxis = wall.max.x + half + skin;
            }

            pos.x = outAxis;
        }
        else
        {
            if (delta > 0.0f)
            {
                // Moving +Z → clamp our max.z to wall.min.z
                outAxis = wall.min.z - half - skin;
            }
            else
            {
                // Moving -Z → clamp our min.z to wall.max.z
                outAxis = wall.max.z + half + skin;
            }

            pos.z = outAxis;
        }

        // Rebuild AABB after correction so we can safely test against
        // remaining walls without double-penetration
        boxBB = MakeAABB(pos);
    }

    return outAxis;
}

bool CanDrop(Vector3 position){
    // Convert world -> tile coords (float)
    Vector2 tileF = WorldToImageCoords(position);

    // Convert to int tile coords (choose rounding style)
    // floorf tends to be safest for grid indexing
    int tx = (int)floorf(tileF.x);
    int ty = (int)floorf(tileF.y);

    // Bounds check BEFORE indexing walkable
    bool inBounds = (tx >= 0 && ty >= 0 && tx < dungeonWidth && ty < dungeonHeight);

    bool canDrop = false;
    if (inBounds)
    {
        canDrop = (walkable[tx][ty] == 1);
    }

    return canDrop;

}


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
        //do nothing?
        //PickUp() is called from player
    }

    // --- While carried ---
    if (state == BoxState::Carried)
    {

        Vector3 lookFwd = ForwardFromYawPitch(playerRot.x, playerRot.y);

        // Carry position follows look direction
        Vector3 target = Vector3Add(playerPos, Vector3Scale(lookFwd, carryOffset.z));

        // Lock to waist height
        target.y = playerPos.y - carryOffset.y;

        // desired target based on player look direction
        Vector3 desired = target;

        // gather nearby wall boxes around desired 
        std::vector<BoundingBox> nearbyWalls = GatherWallBoxesNear(desired);

        Vector3 corrected = position;     // start from current
        corrected.y = desired.y;

        // Step 1: X
        corrected.x = ClampAxisAgainstWalls(position.x, desired.x, corrected, true,  *this, nearbyWalls);
        // Step 2: Z
        corrected.z = ClampAxisAgainstWalls(position.z, desired.z, corrected, false, *this, nearbyWalls);

        position = corrected;

        // Drop (snapped to tile center)

        bool canDrop = CanDrop(position);

        if (canDrop && (dropPressed || interactPressed)) // whichever you prefer: same key toggles, or separate
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
    Vector2 tilePos = WorldToImageCoords(position);
    SetTileWalkable(tilePos.x, tilePos.y, false);
    state = BoxState::Carried;
}

void Box::DropToTileCenter(Vector3 tileCenter)
{
    position = tileCenter;
    state    = BoxState::OnGround;
    Vector2 tilePos = WorldToImageCoords(position);
    SetTileUnwalkable(tilePos.x, tilePos.y, false);
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
        position.y + (150.0f * scale), //stop player from stepping over the box. 
        position.z + half
    };
}
