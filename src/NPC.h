#pragma once
#include "raylib.h"
#include <string>
#include "character.h"

enum class NPCAnimMode
{
    None,
    OneShot,
    TimedLoop
};

enum class NPCState
{
    Idle,
    Talk,
    Walk,
    Aim,
    Fire,
    Patrol,
};

enum class NPCType
{
    Hermit,
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
    float rotationY = 180.0f; 
    Color tint = RED;
    bool flipX = false;



    float flipCooldown = 0.25f;      // seconds between allowed flips (tune 0.08–0.20)
    float flipCooldownLeft = 0.0f;

    float flipDeadZone = 40.0f;      // world-units dead zone near center line (tune 20–80)

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

    // --- turret combat (hermit) ---
    float visionRadius = 2000.0f;     // how far the hermit can "notice" enemies
    float visionRadiusSq = 2000.0f * 2000.0f; // optional cache (set in Init too)
    float fireRadiusSq = 1000.0f * 1000.0f;

    float aimDuration = 1.0f;        // how long he aims before firing
    float aimTimer = 0.0f;

    float fireCooldown = 3.0f;       // delay between shots
    float fireCooldownLeft = 0.0f;
    float firingTimer = 0.0f; // show the firing frame for more than 1 frame. 
    float stateTimer = 0.0f;      // time spent in current state
    float fireHoldDuration = 0.5f; // how long to show the firing frame
    float targetDropGrace = 0.5f;    // seconds allowed out of vision before dropping target //show shooting frame.
    float targetDropTimer = 0.25f;
    bool firedThisCycle = false;      // prevents firing every frame while in Fire

    // --- Patrol (home + wander) ---
    bool   hasIslandNavNPC = false;     // or reuse your global hasIslandNav if you prefer
    Vector3 homePos = {0,0,0};

    float  idleNoTargetTimer = 0.0f;    // counts up while no targets exist
    float  patrolIdleDelay = 5.0f;     // wait this long w/ no targets, then patrol

    float  patrolMinR = 500.0f;         // ring min radius from home
    float  patrolMaxR = 2000.0f;         // ring max radius from home
    float  patrolStopRadius = 100.0f;   // how close is "arrived"
    float  patrolSpeed = 850.0f;        // tune to feel right

    Vector3 patrolGoal = {0,0,0};
    bool    patrolHasGoal = false;

    std::vector<Vector3> navPath;
    int     navPathIndex = -1;
    bool    navHasPath   = false;

    float   navRepathCooldown = 1.25f;  // seconds between BFS calls
    float   navRepathTimer    = 0.0f;   // counts up

    float   patrolStuckTimer  = 0.0f;   // optional: detect stuck
    Vector3 patrolLastPos     = {0,0,0};


    // store a direct pointer to the chosen enemy 
    class Character* target = nullptr;

    Vector3 muzzleOffset = { 0.0f, 0.0f, 0.0f };
    Character* AcquireClosestEnemy(const std::vector<Character*>& enemyPtrs);
    void UpdateHermitTurret(float dt, const std::vector<Character*>& enemyPtrs);



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
    void ChangeState(NPCState newState);
    void SetFlipX();
    Rectangle GetSourceRect() const;

    //pathfinding
    bool BuildPathTo(const Vector3& goalWorld);
    void FollowNavPath(float dt);
    void UpdateHermitPatrol(float dt);
    // Interaction helpers
    bool CanInteract(const Vector3& playerPos) const;
};
