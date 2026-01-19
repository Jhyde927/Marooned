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


    if (state == NPCState::Talk && !CanInteract(player.position)){
        state = NPCState::Idle;
        
    }

    previousPosition = position;

    switch (state)
    {
    case NPCState::Idle:
        // Ensure idle pose
        if (rowIndex != 0 || currentFrame != 0)
            ResetAnim(0, 0, 1, 0.8f);
        break;

    case NPCState::Talk:
        // While talking, do NOT automatically loop the talk animation.
        // Default to idle pose unless a one-shot is currently playing.
        if (animMode != NPCAnimMode::None) break;

    }

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

void NPC::PlayTalkLoopForSeconds(float seconds)
{
    // Safety
    if (seconds <= 0.0f) seconds = 0.05f;

    animMode = NPCAnimMode::TimedLoop;
    talkLoopTimeLeft = seconds;

    // return to idle pose when done
    returnRow = 0;
    returnFrame = 0;

    // start (or restart) the looping talk anim
    ResetAnim(/*row*/1, /*start*/0, /*count*/3, /*speed*/0.8f);
}



void NPC::UpdateAnim(float dt)
{
    // Timed loop countdown (only in that mode)
    if (animMode == NPCAnimMode::TimedLoop)
    {

        talkLoopTimeLeft -= dt;
        if (talkLoopTimeLeft <= 0.0f)
        {
            // stop anim + return to idle pose
            animMode = NPCAnimMode::None;

            rowIndex = returnRow;
            currentFrame = returnFrame;

            animationStart = currentFrame;
            animationFrameCount = 1;
            animationTimer = 0.0f;
            return;
        }
    }

    if (animationFrameCount <= 1) return;

    animationTimer += dt;
    if (animationTimer >= animationSpeed)
    {
        animationTimer = 0.0f;
        currentFrame++;

        int end = animationStart + animationFrameCount;

        if (currentFrame >= end)
        {
            if (animMode == NPCAnimMode::OneShot)
            {
                // finish one-shot
                animMode = NPCAnimMode::None;

                rowIndex = returnRow;
                currentFrame = returnFrame;

                animationStart = currentFrame;
                animationFrameCount = 1;
            }
            else
            {
                // normal looping (this includes TimedLoop mode)
                currentFrame = animationStart;
            }
        }
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

void NPC::PlayTalkOneShot()
{
    // Play talk gesture once, then return to idle pose
    animMode = NPCAnimMode::OneShot;
    returnRow = 0;
    returnFrame = 0;

    ResetAnim(/*row*/1, /*start*/0, /*count*/3, /*speed*/0.8f);
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
