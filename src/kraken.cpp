#include "kraken.h"
#include "resourceManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include "pathfinding.h"

void Kraken::Init(Vector3 spawnPosition,
                  float inWaterY,
                  float inScale)
{
    if (modelLoaded)
    {
        UnloadModel(model);
        modelLoaded = false;
    }

    model = R.GetModel("squidHead");
    modelLoaded = true;

    basePosition = spawnPosition;
    startPos = spawnPosition;
    waterY = inWaterY;
    scale = inScale;

    state = State::Hidden;
    visible = true;
    bobEnabled = true;

    baseYawDeg = 180.0f;     
    currentYawDeg = baseYawDeg;
    idleTiltDeg = 0.0f;

    hiddenOffset = -850.0f;
    exposedOffset = 0.0f;

    currentHeightOffset = hiddenOffset;
    targetHeightOffset = hiddenOffset;

    bobTime = 0.0f;
    rockTime = 0.0f;
    characterTime = 0.0f;

    hitBox = {
        { basePosition.x - 200.0f, basePosition.y - 100.0f, basePosition.z - 200.0f },
        { basePosition.x + 200.0f, basePosition.y + 500.0f, basePosition.z + 200.0f }
    };

    UpdateTransform();
}

Vector3 Kraken::GetHeadPosition() const
{
    return {
        basePosition.x,
        basePosition.y + currentHeightOffset,
        basePosition.z
    };
}

void Kraken::UpdateHitBox()
{
    Vector3 p = GetHeadPosition();

    hitBox = {
        { p.x - 200.0f, p.y - 100.0f, p.z - 200.0f },
        { p.x + 200.0f, p.y + 500.0f, p.z + 200.0f }
    };
}

void Kraken::TakeDamage(float amount)
{
    if (!canTakeDamage) return;

    canTakeDamage = false;
    currentHealth -= amount;
    hitTimer = 0.5f;

    if (!didHalfHealthReposition && currentHealth <= maxHealth * 0.5f)
    {
        repositionAfterSink = true;
        didHalfHealthReposition = true;
        Sink();

        return;
    }

    if (!didQuarterHealthReposition && currentHealth <= maxHealth * 0.25f)
    {
        didQuarterHealthReposition = true;
        repositionAfterSink = true;
        Sink();
        //Reposition(startPos); // or original basePosition saved at Init
        return;
    }

    if (currentHealth <= 0.0f)
    {
        isDead = true;
        state = State::Death;
        // squid death sound effect
    }
}

void Kraken::Update(float dt, Player& player)
{
    if (!modelLoaded || !visible)
        return;

    if (hitTimer >= 0.0f){
        hitTimer -= dt;
    }else{
        canTakeDamage = true;
    }

    UpdateState(dt, player);
    UpdateIdleMotion(dt, player);
    UpdateTransform();
    UpdateHitBox();
}

void Kraken::Draw() const
{
    if (!modelLoaded || !visible)
        return;

    Color tintColor = WHITE;

    if (hitTimer > 0.0f){
        tintColor = RED;
    }

    // Rotation axis is Y-up
    DrawModelEx(
        model,
        drawPosition,
        Vector3{0.0f, 1.0f, 0.0f},
        currentYawDeg,
        Vector3{scale, scale, scale},
        tintColor
    );

    DrawBoundingBox(hitBox, tintColor);
}

void Kraken::Rise()
{
    state = State::Rising;
    targetHeightOffset = exposedOffset;
    playerInRange = true;
    
}

void Kraken::Sink()
{
    state = State::Sinking;
    targetHeightOffset = hiddenOffset;
    playerInRange = false;


}

void Kraken::HideImmediately()
{
    state = State::Hidden;
    currentHeightOffset = hiddenOffset;
    targetHeightOffset = hiddenOffset;
    UpdateTransform();
}

void Kraken::ShowImmediately()
{
    state = State::Exposed;
    currentHeightOffset = exposedOffset;
    targetHeightOffset = exposedOffset;
    UpdateTransform();
}

void Kraken::TeleportTo(Vector3 newBasePosition)
{
    basePosition = newBasePosition;
    UpdateTransform();
}

void Kraken::Reposition(Vector3 newBasePosition)
{
    if (state != State::Hidden) return;
    // Simple version:
    // only really makes sense to call while submerged
    basePosition = newBasePosition;
    currentHeightOffset = hiddenOffset;
    targetHeightOffset = exposedOffset;
    state = State::Rising;
    UpdateTransform();
}

void Kraken::SetBasePosition(Vector3 newBasePosition)
{
    basePosition = newBasePosition;
}

Vector3 Kraken::GetBasePosition() const
{
    return basePosition;
}

Vector3 Kraken::GetDrawPosition() const
{
    return drawPosition;
}

float Kraken::GetYawDegrees() const
{
    return currentYawDeg;
}

void Kraken::SetYawDegrees(float yawDeg)
{
    baseYawDeg = yawDeg;
}

void Kraken::AddYawDegrees(float deltaYawDeg)
{
    baseYawDeg += deltaYawDeg;
}

void Kraken::SetScale(float newScale)
{
    scale = newScale;
}

float Kraken::GetScale() const
{
    return scale;
}

void Kraken::SetWaterY(float newWaterY)
{
    waterY = newWaterY;
}

float Kraken::GetWaterY() const
{
    return waterY;
}

void Kraken::SetBobEnabled(bool enabled)
{
    bobEnabled = enabled;
}

bool Kraken::IsBobEnabled() const
{
    return bobEnabled;
}

void Kraken::SetVisible(bool inVisible)
{
    visible = inVisible;
}

bool Kraken::IsVisible() const
{
    return visible;
}

Kraken::State Kraken::GetState() const
{
    return state;
}

void Kraken::UpdateState(float dt, Player& player)
{
    float speed = 0.0f;
    float dist = Vector3Distance(basePosition, player.position);
    bool hasLOS = HasWorldLineOfSight(basePosition, player.position);
    switch (state)
    {
        case State::Rising:
            speed = riseSpeed;
            currentHeightOffset += speed * dt;
            if (currentHeightOffset >= targetHeightOffset)
            {
                currentHeightOffset = targetHeightOffset;
                state = State::Exposed;
            }
            break;

        case State::Sinking:
            speed = sinkSpeed;
            currentHeightOffset -= speed * dt;
            if (currentHeightOffset <= targetHeightOffset)
            {
                currentHeightOffset = targetHeightOffset;
                state = State::Hidden;
            }
            break;

        case State::Exposed:

            if (dist > 3000.0f && hasLOS){
                Sink();
                break;
            }

            break;
        case State::Hidden:
            if (currentHealth <= 0.0f){
                //stay hidden
                break;
            }

            if (repositionAfterSink){
                repositionAfterSink = false;
                if (basePosition == repPos){
                    Reposition(startPos);
                }else{
                    Reposition(repPos);
                }


                break;
            }

            if (dist < 3000.0f && hasLOS && !repositionAfterSink){
                Rise();
                break;
            }
            break;

        case State::Death:
            Sink();
            break;
            
        default:
            break;
    }
}




void Kraken::UpdateIdleMotion(float dt, Player& player)
{
    bobTime += dt;
    rockTime += dt;
    characterTime += dt;

    float bobOffset = 0.0f;
    if (bobEnabled && state == State::Exposed)
    {
        bobOffset = std::sinf(bobTime * bobSpeed) * bobAmplitude;
    }

    // Gentle side-to-side yaw rocking while exposed
    float rockYaw = 0.0f;
    if (state == State::Exposed)
    {
        rockYaw = std::sinf(rockTime * rockSpeed) * rockAmplitudeDeg;
    }

    Vector3 toPlayer = {
        player.position.x - basePosition.x,
        0.0f,
        player.position.z - basePosition.z
    };

    // Optional: avoid jitter if player is exactly on top of the kraken
    float lenSq = toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z;
    if (lenSq > 0.001f)
    {
        float yawRad = atan2f(toPlayer.x, toPlayer.z);
        currentYawDeg = (yawRad * RAD2DEG) - 30.0f + rockYaw;
    }

    idleTiltDeg = 0.0f;
}

void Kraken::UpdateTransform()
{
    drawPosition = basePosition;
    // Put the boss relative to the water plane, not terrain
    drawPosition.y = waterY + currentHeightOffset + idleTiltDeg;
}