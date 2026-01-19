#pragma once
#include "raylib.h"
#include <string>

enum class NPCAnimMode
{
    None = 0,
    OneShot,
    TimedLoop
};

enum class NPCState
{
    Idle = 0,
    Talk,
    Walk
};

enum class NPCType
{
    Hermit = 0,
    Parrot,
    Skeleton,
    Pirate
};

class NPC
{
public:
    // --- core ---
    Vector3 position = {0,0,0};
    Vector3 previousPosition = {0,0,0};

    Texture2D texture = {};

    NPCState state = NPCState::Idle;
    NPCType  type  = NPCType::Hermit;

    float scale = 1.0f;
    float rotationY = 0.0f; // optional if you ever want yaw-facing billboards
    Color tint = RED;

    bool isActive = true;
    bool isVisible = true;

    // --- interaction ---
    float interactRadius = 400.0f;
    bool  isInteractable = true;
    std::string dialogId; // dialog lives elsewhere, this is the key

    // --- animation (same pattern as Character) ---
    int frameWidth  = 0;
    int frameHeight = 0;

    int currentFrame = 0;
    int maxFrames    = 1;   // optional convenience (not strictly required)
    int rowIndex     = 0;

    float animationTimer = 0.0f;
    float animationSpeed = 0.10f; 

    int animationStart      = 0;
    int animationFrameCount = 1;

    // --- one-shot animation support ---
    bool  oneShotActive = false;     // if true, animation plays once and stops

    NPCAnimMode animMode = NPCAnimMode::None;
    float talkLoopTimeLeft = 0.0f;

    // where to return after one-shot / timed loop
    int returnRow = 0;
    int returnFrame = 0;


public:
    NPC() = default;

    void Init(Texture2D sheet, int frameW, int frameH, float sc = 1.0f);

    // Call every frame
    void Update(float dt);

    // Animation helpers
    void ResetAnim(int row, int startFrame, int frameCount, float speed);
    void UpdateAnim(float dt);
    void PlayTalkOneShot();
    void PlayTalkLoopForSeconds(float seconds);
    float GetFeetPosY();
    Rectangle GetSourceRect() const;

    // Interaction helpers
    bool CanInteract(const Vector3& playerPos) const;
};
