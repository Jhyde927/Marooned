#pragma once

#include "raylib.h"
#include "player.h"
#include <vector>
#include "emitter.h"

enum class CharacterType {
    Raptor,
    Skeleton,
    Pirate,
    Spider,
    Ghost,
    Trex,
    GiantSpider,
    Pterodactyl,
    Wizard,
    Bat,
   
};


enum class CharacterState {
    Idle, 
    Chase,
    Attack,
    RunAway,
    Freeze,
    Stagger,
    Reposition,
    Patrol,
    MeleeAttack,
    Harpooned,
    Death,
};

struct AnimDesc {
    int row;
    int frames;
    float frameTime;
    bool loop;

};

enum class FacingMode {
    Approaching,  // show front
    Leaving,      // show back
    Strafing      // show side profile
};



class Character {
public:
    Vector3 position;
    Texture2D texture;
    Vector3 previousPosition;
    CharacterState state = CharacterState::Idle;
    Emitter bloodEmitter;
    
    int frameWidth, frameHeight;
    int currentFrame, maxFrames;
    int rowIndex;
    float animationTimer, animationSpeed;
    int animationStart = 0;
    int animationFrameCount = 1;
    float scale;
    float rotationY = 0.0f; // in degrees
    float stateTimer = 0.0f;
    float stepTimer = 0.0f;
    int id = -1;
    float raptorSpeed = 700.0f;
    float raptorSoundCooldown = 0.0f;
    Vector3 patrolTarget{0,0,0};
    bool hasPatrolTarget = false;
    float wanderAngle = 0.0f;
    Vector3 velocity = {0,0,0};     // steering velocity (world units/sec) flying enemies    
    bool isDead = false;
    bool hasFired = false;
    bool animationLoop;
    bool canSee;
    float spriteHeight;
    int lastAttackid = -1;
    float deathTimer = 0.0f;
    float attackCooldown = 0.0f;
    float chaseDuration = 0.0f;
    float hitTimer = 0.0f;
    float freezeTimer = 0.0f;
    float runawayAngleOffset = 0.0f;
    int patrolRadius = 3;
    bool hasRunawayAngle = false;
    float idleThreshold = 0.0f; // for random movements when idle
    float randomDistance = 0.0f; //how far away to run before stopping. 
    float randomTime = 0.0f;
    float pathCooldownTimer = 0.0f;
    Vector2 lastPlayerTile = {-1, -1}; // Initialized to invalid tile
    float skeleSpeed = 650;
    bool playerVisible = false;
    float timeSinceLastSeen = 9999.0f;  // Large to start
    float forgetTime = 10.0f;           // After 3 seconds of no visibility, give up
    Vector3 lastKnownPlayerPos;
    bool hasLastKnownPlayerPos = false;
    bool canBleed = true;
    float radius = 100;
    int maxHealth = 150;
    int currentHealth = maxHealth;
    float accumulateDamage = 0.0f;
    float spiderAgroTimer = 0.0f;
    bool canLayEgg = true;
    bool spiderAgro = false;
    bool canMelee = true;
    bool   isLeaving = false;          // choose "walk away" row vs "walk toward"
    float  prevDistToPlayer = -1.0f;   // for distance trend fallback
    Vector3 prevPos = {0,0,0};         // to compute velocity
    Vector3 fleeTarget = { 0, 0, 0 };
    Vector3 harpoonTarget = {0};   // usually player position (updated each frame)
    unsigned int lastBulletIDHit = 2;
    int    approachStreak = 0;         // hysteresis counters
    int    leaveStreak    = 0;
    int    strafeStreak = 0;
    float strafeSideSign = 1.0f; // >0 = one way, <0 = the other
    bool    hasFleeTarget = false;
    bool overLava = false;
    bool overVoid = false;
    bool lavaDamageApplied = false;
    float harpoonDuration = 2.0f;
    float harpoonMinDist  = 175.0f; // stop just in front of player

    int   navPathIndex = -1;        // current waypoint
    bool  navHasPath   = false;     // are we using nav path for this chase?

    // optional: to avoid rebuilding path too often
    float navRepathTimer = 0.0f;
    static constexpr float NAV_REPATH_INTERVAL = 1.5f; // seconds, tweak

    //dactyl
    float verticalVel   = 0.0f;   // Y velocity
    float desiredAlt    = 0.0f;   // target altitude above ground
    bool  inAir         = false;  // optional latch: are we airborne?
    float patrolAlt     = 1200.0f;

    //Bat bob
    float bobPhase = 0.0f;        // radians
    float bobSpeed = 3.5f;        // radians per second
    float bobAmplitude = 18.0f;   // world units
    float hoverHeight = 225.0f;   // base offset above ground

    //bloat
    bool bloatBat = false;
    bool hasExploded = false;



    FacingMode facingMode = FacingMode::Approaching;
    CharacterType type;

    std::vector<Vector2> currentPath;
    std::vector<Vector3> currentWorldPath;

    std::vector<Vector3> navPath;   // heightmap pathfinding path

    Character(Vector3 pos, Texture2D& tex, int fw, int fh, int frames, float speed, float scl, int row = 0, CharacterType t = CharacterType::Raptor);
    BoundingBox GetBoundingBox() const;
    Vector3 ComputeRepulsionForce(const std::vector<Character*>& allRaptors, float repulsionRadius = 500.0f, float repulsionStrength = 6000.0f);
    Vector3 GetFeetPos() const;
    void SetFeetPos(const Vector3& feet);
    void ApplyGroundSnap();
    void Update(float deltaTime, Player& player);
    void UpdateTrexAI(float deltaTime, Player& player);
    void UpdateRaptorAI(float deltaTime, Player& player);
    void UpdateDactylAI(float deltaTime, Player& player);
    void UpdateAltitude(float dt, float groundY, float desiredAltitude);
    void UpdateAI(float deltaTime, Player& player); 
    void UpdateSkeletonAI(float deltaTime, Player& player);
    void UpdateBatAI(float deltaTime, Player& player);
    void UpdateGiantSpiderAI(float deltaTime, Player& player);
    void UpdatePirateAI(float deltaTime, Player& player);
    void UpdateWizardAI(float deltaTime, Player& player);
    void UpdatePlayerVisibility(const Vector3& playerPos, float dt, float epsilon);
    bool FindRepositionTarget(const Player& player, const Vector3& selfPos, Vector3& outTarget);
    void AlertNearbySkeletons(Vector3 alertOrigin, float radius);
    void UpdateRaptorVisibility(const Player& player, float dt);
    void UpdatePatrolSteering(float dt);
    void UpdateChaseSteering(float dt);
    void SetPath(Vector2 start);
    void UpdateMovementAnim();
    void SetPathTo(const Vector3& goalWorld);
    bool ChooseFleeTarget();
    void BuildPathToPlayer();
    bool ChoosePatrolTarget();
    bool BuildPathTo(const Vector3& goalWorld);
    void PlayDeathSound();
    void TakeDamage(int amount);
    void SetAnimation(int row, int frames, float speed, bool loop=true);
    void playRaptorSounds();
    void ChangeState(CharacterState next);
    bool MoveAlongPath(std::vector<Vector3>& path, Vector3& pos, float& yawDeg,float speed, float dt, float arriveEps = 100.0f, Vector3 repulsion = {});
    void UpdatePatrol(float deltaTime);
    void UpdateRunaway(float deltaTime);
    void UpdateRunawaySteering(float dt);
    void UpdateChase(float deltaTime);
    void UpdateTrexStepSFX(float dt);
    void ApplyAreaDamage();
    AnimDesc GetAnimFor(CharacterType type, CharacterState state);

    void UpdateLeavingFlag(const Vector3& playerPos, const Vector3& playerPrevPos);
};

