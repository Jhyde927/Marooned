#pragma once

#include "raylib.h"
#include "weapon.h"
#include "inventory.h"


extern Weapon weapon;
extern MeleeWeapon meleeWeapon;
extern MagicStaff magicStaff;
extern Crossbow crossbow;

class Box;

enum class PlayerState { Normal, Grappling, Frozen};

struct GroundHit
{
    bool  solid;     // snappable floor (not void/lava)
    bool  lava;
    bool  voidPit;
    float groundY;   // what Y you'd snap to if solid
};



struct Player {
    Vector3 position;
    Vector3 velocity;
    Vector2 rotation;
    Vector3 forward;
    Vector3 startPosition;
    Vector3 previousPosition;
    BoundingBox meleeHitbox;
    BoundingBox blockHitbox;
    Inventory inventory;

    
    PlayerState state = PlayerState::Normal;
    bool hasGoldKey   = false;
    bool hasSilverKey = false;
    bool hasSkeletonKey = false;
    const float ACCEL_GROUND   = 8000.0f;   // how fast we reach target speed
    const float DECEL_GROUND   = 7000.0f;   // how fast we slow to zero
    const float ACCEL_AIR      = 1150.0f;    // small air control
    const float FRICTION_AIR   = 0.01f;    // bleed a bit of air speed
    const float GRAVITY        = 850.0f;

    const float COYOTE_TIME    = 0.0f;  // grace after walking off ledge
    const float JUMP_BUFFER    = 0.05f;  // grace before touching ground

    float lastGroundedTime = 0.0f;
    float lastJumpPressedTime = -999.0f;

    Vector3 grappleTarget = {0,0,0};   // where you're being pulled to
    float   grappleSpeed = 3000.0f;
    float   grappleStopDist = 0.0f;
    int     grappleBulletId = -1;      // optional: link to the harpoon bullet
    float harpoonLifeTimer = 0.0;

    float freezeTimer = 0.0f;
    float canFreeze = true;
    int gold = 0;
    float displayedGold = 0.0f;

    bool running = false;
    float runSpeed = 850.0f; // faster than walk speed
    float walkSpeed = 500.0f; // regular speed
    float startingRotationY = 0.0f; // in degrees
    float lightIntensity = 0.5f;
    float lightRange = 400;
    float gravity = 980.0f;

    float height = 240.0f; //stream says make this higher

    float jumpStrength = 600.0f; 
    float footstepTimer = 0.0f;
    float swimTimer = 0.0f;
    float maxHealth = 100.0f;
    float currentHealth = maxHealth;
    float maxMana = 100.0f;
    float currentMana = maxMana;
    float maxStamina = 100.0f;
    float stamina = maxStamina;
    int attackId = 0; //incremented every melee attack for a unique id. 
    float radius = 100.0f;
    float hitTimer = 0.0f;
    bool dying = false;
    bool dead = false;
    float deathTimer = 0.0f;
    float targetFOV = 50.0f;  //change camera.fovy slightly on hit
    float groundY;
    bool grounded = false;
    bool isSwimming = false;
    bool overLava = false;
    bool overVoid = false;
    bool isMoving = false;
    bool onBoard = false;
    bool disableMovement = false;
    bool blocking = false;
    bool isFallingIntoVoid = false;
    bool godMode = false;

    //box interaction
    bool interactPressed;
    bool dropPressed;
    bool isCarrying;
    Box* carriedBox = nullptr;

    float centerGroundY = 0.0f;

    float spreadMinDeg   = 1.5f;
    float spreadMaxDeg   = 6.0f;
    float crosshairBloom = 0.0;
    float spreadDegrees = 2.5f;       // computed each frame (use in Fire)
    Vector3 crossbowMuzzlePos;

    float lastSafeGroundY   = 100.0f; 

    bool debugShowFootSamples = true;


    bool canRun = true;
    bool canMove = true;

    std::vector<WeaponType> collectedWeapons;

    int currentWeaponIndex = -1; 
    WeaponType activeWeapon = WeaponType::Crossbow;
    WeaponType previousWeapon = WeaponType::Sword;
    float switchTimer = 0.0f;
    float switchDuration = 0.3f; // time to lower or raise
    BoundingBox GetBoundingBox() const;
    void PlayFootstepSound();
    void TakeDamage(int amount);
    void EquipNextWeapon();
};

// Initializes the player at a given position
void InitPlayer(Player& player, Vector3 startPosition);

// Updates player movement and physics
void UpdatePlayer(Player& player, float deltaTime, Camera& camera);
void HandlePlayerMovement(float deltaTime);
void TryQueuedJump();
void DrawPlayer(const Player& player, Camera& camera);
void InitSword(MeleeWeapon& sword);
void InitBlunderbuss(Weapon& blunderbuss);
void InitMagicStaff(MagicStaff& magicStaff);
void InitCrossbow();
void HandleJumpButton(float timeNow);
void OnGroundCheck(bool groundedNow, float timeNow);
void DrawWeapons(const Player& player, Camera& camera);