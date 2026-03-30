#include "powerUps.h"
#include <vector>
#include <cmath>
#include "world.h"
#include "player.h"
#include "sound_manager.h"

PowerUpPickup::PowerUpPickup(PowerUpType type, Vector3 position, Texture2D texture)
    : type(type), position(position), texture(texture)
{
    rotationY = 0.0f;
    isCollected = false;
    useModel = false;
    scale = 1.0f;
    bobTimer = 0.0f;
    spinTimer = 0.0f;
    scaleX = 1.0f;
    model = { 0 };
}

PowerUpPickup::PowerUpPickup(PowerUpType type, Vector3 position, Model model)
    : type(type), position(position), model(model)
{
    rotationY = 0.0f;
    isCollected = false;
    useModel = true;
    scale = 1.0f;
    bobTimer = 0.0f;
    spinTimer = 0.0f;
    scaleX = 1.0f;
    texture = { 0 };
}

Texture2D GetPowerUpTexture(PowerUpType type)
{
    switch (type)
    {
        case PowerUpType::QuadDamage: return R.GetTexture("quadDamage");
        case PowerUpType::Haste:      return R.GetTexture("haste");
        case PowerUpType::OverHealth: return R.GetTexture("overHealth");

        default:                      return Texture2D{};
    }
}

void PowerUpPickup::Update(float deltaTime)
{
    if (isCollected) return;

    bobTimer += deltaTime;
    rotationY += 45.0f * deltaTime; // slow spin

    if (pickupBlockedTimer > 0.0f)
        pickupBlockedTimer -= deltaTime;

    spinTimer += deltaTime;
    scaleX = cosf(spinTimer * 2.0f);

    if (rotationY >= 360.0f)
        rotationY -= 360.0f;
}

void PowerUpPickup::Draw(const Camera3D& camera)
{
    if (isCollected) return;

    float bobOffset = std::sin(bobTimer * 2.5f) * 4.0f;
    Vector3 drawPos = position;
    drawPos.y += bobOffset;

    if (useModel)
    {
        // --------------------------------------------------
        // FUTURE 3D MODEL DRAWING
        // --------------------------------------------------
        // For later:
        // DrawModelEx(
        //     model,
        //     drawPos,
        //     Vector3{ 0.0f, 1.0f, 0.0f },   // rotation axis
        //     rotationY,
        //     Vector3{ scale, scale, scale },
        //     WHITE
        // );
        //
        // You could also tint based on type here.
        // --------------------------------------------------
    }
    else
    {
        if (texture.id == 0) return;

        // Billboard size in world units. Adjust to taste.
        //float billboardSize = 40.0f * scale;

        float absScaleX = fabsf(scaleX);

        source = {
            0.0f,
            0.0f,
            (scaleX < 0.0f) ? -static_cast<float>(texture.width) : static_cast<float>(texture.width),
            static_cast<float>(texture.height)
        };

        size = {
            128.0f * absScaleX,   // width changes continuously
            128.0f                // height stays the same
        };

    }
}

bool PowerUpPickup::CheckPickup(Player& player, float pickupRadius)
{
    if (isCollected) return false;
    if (pickupBlockedTimer > 0.0f) return false;

    float distanceSq = Vector3DistanceSqr(position, player.position);
    float pickupRadiusSq = pickupRadius * pickupRadius;

    if (distanceSq > pickupRadiusSq)
        return false;

    PowerUpType oldPowerUp = player.currentPowerUp; //if your currently holding a power up. Swap them. 
    PowerUpType newPowerUp = type;

    // Collect this one
    isCollected = true;

    // Drop the old one at this pickup's position
    if (oldPowerUp != PowerUpType::None && oldPowerUp != newPowerUp)
    {
        PowerUpPickup dropped(oldPowerUp, position, GetPowerUpTexture(oldPowerUp));
        dropped.pickupBlockedTimer = 0.75f; // enough time to move away
        g_powerUps.push_back(dropped);
    }

    // Give the player the new one
    player.currentPowerUp = newPowerUp;
    

    return true;
}


void UpdatePowerUps(Player& player, float deltaTime)
{
    for (auto& powerUp : g_powerUps)
    {
        if (powerUp.isCollected) continue;

        powerUp.Update(deltaTime);
        powerUp.CheckPickup(player);
    }
}

void DrawPowerUps(Player& player, const Camera3D& camera, float deltaTime)
{
    for (auto& powerUp : g_powerUps)
    {
        if (powerUp.isCollected) continue;
        powerUp.Draw(camera);
    }
}