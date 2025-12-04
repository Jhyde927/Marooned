#pragma once

#include "raylib.h"

enum class WeaponType { 
    Blunderbuss, 
    Sword,
    MagicStaff,
    None,

};

enum class MagicType {
    Fireball,
    Iceball,
};

struct MuzzleFlash {
    Vector3 position;
    Texture2D texture;
    float size;
    float lifetime;
    float age = 0.0f;
};



struct MeleeWeapon {
    Model model;
    Vector3 scale = { 1.0f, 1.0f, 1.0f };

    float swingTimer = 0.0f;
    float swingDuration = 0.7f;
    bool swinging = false;

    bool hitboxActive = false;
    float hitboxTimer = 0.0f;
    float hitboxDuration = 0.25f; // how long hitbox stays active

    float hitWindowStart = 0.1f;  // seconds into the swing
    float hitWindowEnd = 0.25f;   // seconds into the swing
    bool hitboxTriggered = false;

    bool blocking = false;
    float blockLerp = 0.0f; // 0 → 1 smooth transition
    float blockSpeed = 6.0f; // how fast it moves to blocking position

    // Block pose offsets
    float blockForwardOffset = 50.0f;
    float blockVerticalOffset = -50.0f;
    float blockSideOffset = 0.0f;

    
    float bobbingTime = 0.0f;
    bool isMoving = false; // set this externally based on player movement
    float bobVertical = 0.0f;
    float bobSide = 0.0f;


    float swingAmount = 20.0f;   // how far forward it jabs or sweeps
    float swingOffset = 0.0f;    // forward movement

    float verticalSwingOffset = -30.0f;
    float verticalSwingAmount = 60.0f; // how far it chops down

    float horizontalSwingOffset = 0.0f;
    float horizontalSwingAmount = 20.0f; // little lateral arc


    float cooldown = 1.0f;
    float timeSinceLastSwing = 999.0f;

    float forwardOffset = 60.0f;//default postion
    float sideOffset = 20.0f; // pull it left of the screen
    float verticalOffset = -50.0f; //default position


    void StartBlock();
    void EndBlock();
    void StartSwing(Camera& camera);
    void PlaySwipe();
    void Update(float deltaTime);
    void Draw(const Camera& camera);
};


struct Weapon {
    Model model;
    Vector3 scale = { 1.0f, 1.0f, 1.0f };
    Vector3 muzzlePos;
    float flashSize = 120.0f;
    float forwardOffset = 80.0f;
    float sideOffset = 30.0f;
    float verticalOffset = -30.0f;

    float recoil = 0.0f;
    float recoilAmount = 15.0f;
    float recoilRecoverySpeed = 30.0f;

    float lastFired = -999.0f;
    float fireCooldown; //set in player init

    float bobbingTime = 0.0f;
    bool isMoving = false; // set this externally based on player movement
    float bobVertical = 0.0f;
    float bobSide = 0.0f;

    Texture2D muzzleFlashTexture;
    float flashDuration = 0.05f;
    float flashTimer = 0.0f;
    //delayed reload sound
    bool reloadScheduled = false;
    float reloadTimer = 0.0f;
    float reloadDelay = 1.6f; // adjust as needed
    float reloadDip = 0.0f; // controls how far the gun dips down
    bool flashTriggered = false;  // set to true when firing

    void Fire(Camera& camera);
    void Update(float deltaTime);
    void Draw(const Camera& camera);
};


struct MagicStaff {
    Model model;
    Vector3 scale = { 1.0f, 1.0f, 1.0f };
    MagicType magicType = MagicType::Fireball;
    // === Melee / Swinging ===
    float swingTimer = 0.0f;
    float swingDuration = 1.0f;
    bool swinging = false;

    bool hitboxActive = false;
    float hitboxTimer = 0.0f;
    float hitboxDuration = 0.25f;

    float hitWindowStart = 0.1f;
    float hitWindowEnd = 0.25f;
    bool hitboxTriggered = false;

    float cooldown = 1.0f;
    float timeSinceLastSwing = 999.0f;

    // Offsets for first-person placement
    float forwardOffset = 80.0f;
    float sideOffset = 28.0f;
    float verticalOffset = -25.0f;

    float bobbingTime = 0.0f;
    bool isMoving = false; // set this externally based on player movement
    float bobVertical = 0.0f;
    float bobSide = 0.0f;

    float swingAmount = -50.0f; 
    float swingOffset = 0.0f;

    float verticalSwingOffset = -25.0f;
    float verticalSwingAmount = 35.0f;

    float horizontalSwingOffset = 0.0f;
    float horizontalSwingAmount = 30.0f;

    // === Shooting / Magic Projectile ===
    Vector3 muzzlePos;
    float fireCooldown = 1.0f;
    float lastFired = -999.0f;

    float recoil = 0.0f;
    float recoilAmount = 8.0f;
    float recoilRecoverySpeed = 20.0f;

    Texture2D muzzleFlashTexture;
    float flashDuration = 0.08f;
    float flashTimer = 0.0f;
    float flashSize = 64.0f;

    float reloadDip = 0.0f;

    // === Methods ===
    void StartSwing(Camera& camera);
    void Update(float deltaTime);
    void Fire(const Camera& camera);
    void Draw(const Camera& camera);
    void PlaySwipe();
};


