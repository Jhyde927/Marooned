#include "NPC.h"
#include <cmath>
#include "world.h"

void NPC::Init(Texture2D sheet, int frameW, int frameH, float sc)
{
    texture = sheet;
    frameWidth  = frameW;
    frameHeight = frameH;
    scale = sc;

    // Default: idle row 0, single frame
    ResetAnim(/*row*/0, /*start*/0, /*count*/1, /*speed*/0.15f);
}

void NPC::Update(float dt)
{
    if (!isActive) return;

    previousPosition = position;

    // You can add simple wandering here later.
    // For now, just animate based on state:
    UpdateAnim(dt);
}

void NPC::ResetAnim(int row, int startFrame, int frameCount, float speed)
{
    rowIndex = row;

    animationStart      = startFrame;
    animationFrameCount = (frameCount <= 0) ? 1 : frameCount;

    animationSpeed = speed;
    animationTimer = 0.0f;

    currentFrame = animationStart;

    // Optional convenience
    maxFrames = animationStart + animationFrameCount;
}

void NPC::UpdateAnim(float dt)
{
    if (animationFrameCount <= 1) return;

    animationTimer += dt;
    if (animationTimer >= animationSpeed)
    {
        animationTimer = 0.0f;

        currentFrame++;

        int end = animationStart + animationFrameCount;
        if (currentFrame >= end)
            currentFrame = animationStart;
    }
}

Rectangle NPC::GetSourceRect() const
{
    return Rectangle{
        (float)(currentFrame * frameWidth),
        (float)(rowIndex     * frameHeight),
        (float)frameWidth,
        (float)frameHeight
    };
}



bool NPC::CanInteract(const Vector3& playerPos) const
{
    if (!isActive || !isVisible || !isInteractable) return false;

    float dx = playerPos.x - position.x;
    float dz = playerPos.z - position.z;
    float dist2 = dx*dx + dz*dz;

    return dist2 <= (interactRadius * interactRadius);
}

float NPC::GetFeetPosY(){
    return GetHeightAtWorldPosition(position, heightmap, terrainScale);
}
