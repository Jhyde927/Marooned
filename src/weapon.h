#pragma once

#include "raylib.h"

enum class SwordAttackType {
    RightSlash,
    LeftSlash,
    Stab
};

enum class CrossbowState {
    Loaded,
    Rest
};

enum class WeaponType {
    None,
    Sword,
    Crossbow,
    Blunderbuss, 
    MagicStaff,
    COUNT

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

    float swingTimer;
    float swingDuration;
    bool swinging;

    bool hitboxActive;
    float hitboxTimer;
    float hitboxDuration;

    float hitWindowStart;
    float hitWindowEnd;
    bool hitboxTriggered;

    bool blocking;
    float blockLerp;
    float blockSpeed;

    float blockForwardOffset;
    float blockVerticalOffset;
    float blockSideOffset;

    float bobbingTime;
    bool isMoving;
    float bobVertical;
    float bobSide;

    float swingAmount;
    float swingOffset;

    float verticalSwingOffset;
    float verticalSwingAmount;

    float horizontalSwingOffset;
    float horizontalSwingAmount;

    float attackForwardOffset;
    float attackSideOffset;
    float attackVerticalOffset;
    float attackYawDeg;

    float equipDip;

    float normalCooldown;
    float comboCooldown;
    float comboResetTime;
    float timeSinceLastSwing;
    float comboTimer;
    int comboIndex;
    SwordAttackType currentAttack;

    float baseRollDeg;
    float attackRollDeg;

    float forwardOffset;
    float sideOffset;
    float verticalOffset;

    void Init();

    void StartBlock();
    void EndBlock();
    void StartSwing(Camera& camera);


    void PlaySwipe();
    void Update(float deltaTime);
    void Draw(const Camera& camera);
    void DrawThirdPerson();
    void UpdateRightSlashMotion(float t);
    void UpdateLeftSlashMotion(float t);
    void UpdateStabMotion(float t);
    float GetCurrentDamage() const;
};


struct Weapon {
    Model model;
    Vector3 scale = { 1.0f, 1.0f, 1.0f };
    Vector3 muzzlePos;
    float flashSize = 120.0f;
    float forwardOffset = 80.0f; //80
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

    // --- Blunderbuss double shot ---
    bool doubleLoaded = false;          // next shot fires extra pellets
    bool doubleLoadScheduled = false;   // right-click reload animation is happening
    float doubleLoadTimer = 0.0f;
    float doubleLoadDelay = 0.65f;      // time before doubleLoaded becomes true
    float doubleLoadDipAmount = 28.0f;

    // --- Crosshair / spread control (for Blunderbuss, can be reused later) ---
    float crosshairBloom = 0.0f;      // 0..1 (0 = settled, 1 = fully bloomed)

    float bloomExpandRate = 4.0f;     // grows fast when moving
    float bloomShrinkRate = 3.0f;     // shrinks slower when stopping

    void Fire(Camera& camera);
    void Update(float deltaTime);
    void Draw(const Camera& camera);
    void DrawThirdPerson();
    void DrawFirstPerson(const Camera& camera);
};


struct MagicStaff {
    Model model;
    Vector3 scale = { 1.0f, 1.0f, 1.0f };
    MagicType magicType = MagicType::Fireball;
    // === Melee / Swinging ===
    float swingTimer = 0.0f;
    float swingDuration = 0.6f;
    bool swinging = false;

    bool hitboxActive = false;
    float hitboxTimer = 0.0f;
    float hitboxDuration = 0.25f;

    float hitWindowStart = 0.1f;
    float hitWindowEnd = 0.7f;
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
    float equipDip = 0.0f;
    // === Methods ===
    void StartSwing(Camera& camera);
    void Update(float deltaTime);
    void Fire(const Camera& camera);
    void Draw(const Camera& camera);
    void PlaySwipe();
};



struct Crossbow {
    Model loadedModel;
    Model restModel;
    CrossbowState state = CrossbowState::Loaded;

    Vector3 muzzlePos;

    Vector3 scale = { 1.0f, 1.0f, 1.0f };
    float forwardOffset  = 40.0f;
    float sideOffset     = 25.0f;
    float verticalOffset = -35.0f;

    // Bobbing
    float bobbingTime = 0.0f;
    bool  isMoving    = false;
    float bobVertical = 0.0f;
    float bobSide     = 0.0f;

    // Recoil
    float recoil               = 0.0f;
    float recoilAmount         = 20.0f;  // how hard it kicks back
    float recoilRecoverySpeed  = 20.0f;  // how fast it returns

    // Fire / reload
    float lastFired   = -999.0f;
    float fireCooldown = 0.8f;

    // Reload dip animation
    float reloadPhase      = 0.0f;   // 0 → 1 (full cycle)
    bool  isReloading      = false;
    float reloadDip        = 0.0f;   // computed each frame
    float reloadDipAmount  = 24.0f;  // how far down it dips
    float reloadSpeed      = 0.5f;   // how fast 0→1
    float autoReloadDelay  = 0.5f;   // wait before starting dip
    float reloadDelayTimer = 0.5f;

    bool  swappedModelMidDip = false;  // did we already switch to loaded model?
    float harpoonCooldown = 2.0f;
    float harpoonTimer = 0.0;
  
    bool harpoonReady = true; // persistent state
    bool triggeredFire = false;




    void Update(float dt);
    void Fire(Camera& camera);
    void FireHarpoon(Camera& camera);
    void Reload();     // optional manual reload trigger
    void Draw(const Camera& camera);
};




