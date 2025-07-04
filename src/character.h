#pragma once

#include "raylib.h"
#include "player.h"
#include <vector>

enum class CharacterType {
    Raptor,
    Skeleton,
    Pirate,
   
};


enum class CharacterState {
    Idle,
    Chase,
    Attack,
    RunAway,
    Stagger,
    Reposition,
    Patrol,
    MeleeAttack,
    Death
};



class Character {
public:
    Vector3 position;
    Texture2D* texture;
    Vector3 previousPosition;
    CharacterState state = CharacterState::Idle;
    int frameWidth, frameHeight;
    int currentFrame, maxFrames;
    int rowIndex;
    float animationTimer, animationSpeed;
    int animationStart = 0;
    int animationFrameCount = 1;
    float scale;
    float rotationY = 0.0f; // in degrees
    float stateTimer = 0.0f;
    bool isDead = false;
    bool hasFired = false;
    bool animationLoop;
    float deathTimer = 0.0f;
    float attackCooldown = 0.0f;
    float chaseDuration = 0.0f;
    float hitTimer = 0.0f;
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

    float radius = 50;
    float hearingRadius = 600.0f;
    bool heardPlayer = false;
    //BoundingBox collider;
    int maxHealth = 150;
    int currentHealth = maxHealth;
    CharacterType type;

    std::vector<Vector2> currentPath;
    std::vector<Vector3> currentWorldPath;


    Character(Vector3 pos, Texture2D* tex, int fw, int fh, int frames, float speed, float scl, int row = 0, CharacterType t = CharacterType::Raptor);
    BoundingBox GetBoundingBox() const;
    void Update(float deltaTime, Player& player,const Image& heightmap, Vector3 terrainScale);
    Vector3 ComputeRepulsionForce(const std::vector<Character*>& allRaptors, float repulsionRadius = 500.0f, float repulsionStrength = 6000.0f);
    void UpdateRaptorAI(float deltaTime, Player& player,const Image& heightmap, Vector3 terrainScale);
    void UpdateAI(float deltaTime, Player& player,const Image& heightmap, Vector3 terrainScale); 
    void UpdateSkeletonAI(float deltaTime, Player& player);
    void UpdatePirateAI(float deltaTime, Player& player);
    void AlertNearbySkeletons(Vector3 alertOrigin, float radius);

    void eraseCharacters();
    void setPath();
    void TakeDamage(int amount);
    void Draw(Camera3D camera);
    void SetAnimation(int row, int frames, float speed, bool loop=true);
    void playRaptorSounds();
};

