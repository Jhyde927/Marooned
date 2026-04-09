#include "kraken.h"
#include "resourceManager.h"
#include <cmath>
#include <algorithm>
#include <iostream>

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
    waterY = inWaterY;
    scale = inScale;

    state = State::Hidden;
    visible = true;
    bobEnabled = true;

    baseYawDeg = 180.0f;     
    currentYawDeg = baseYawDeg;
    idleTiltDeg = 0.0f;

    hiddenOffset = -350.0f;
    exposedOffset = 0.0f;

    currentHeightOffset = hiddenOffset;
    targetHeightOffset = hiddenOffset;

    bobTime = 0.0f;
    rockTime = 0.0f;
    characterTime = 0.0f;

    UpdateTransform();
}

void Kraken::Unload()
{
    if (modelLoaded)
    {
        UnloadModel(model);
        modelLoaded = false;
    }
}

void Kraken::Update(float dt, Player& player)
{
    if (!modelLoaded || !visible)
        return;

    UpdateState(dt, player);
    UpdateIdleMotion(dt, player);
    UpdateTransform();
}

void Kraken::Draw() const
{
    if (!modelLoaded || !visible)
        return;

    // Rotation axis is Y-up
    DrawModelEx(
        model,
        drawPosition,
        Vector3{0.0f, 1.0f, 0.0f},
        currentYawDeg,
        Vector3{scale, scale, scale},
        WHITE
    );
}

void Kraken::Rise()
{
    state = State::Rising;
    targetHeightOffset = exposedOffset;
}

void Kraken::Sink()
{
    state = State::Sinking;
    targetHeightOffset = hiddenOffset;
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

            if (dist > 2000.0f){
                Sink();
                break;
            }

            break;
        case State::Hidden:
            if (dist < 2000.0f){
                Rise();

                break;
            }
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

    currentYawDeg = baseYawDeg + rockYaw;
    idleTiltDeg = bobOffset;
}

void Kraken::UpdateTransform()
{
    drawPosition = basePosition;

    // Put the boss relative to the water plane, not terrain
    drawPosition.y = waterY + currentHeightOffset + idleTiltDeg;
}