#include "player.h"
#include "raymath.h"
#include "world.h"
#include <iostream>
#include "resourceManager.h"
#include "input.h"
#include "boat.h"
#include "rlgl.h"
#include "sound_manager.h"
#include "inventory.h"
#include "input.h"
#include "weapon.h"
#include "camera_system.h"
#include "array"
#include "utilities.h"
#include "pathfinding.h"

Weapon weapon;
MeleeWeapon meleeWeapon;
MagicStaff magicStaff;
Crossbow crossbow;



//Refactor this whole mess at some point. 
void InitPlayer(Player& player, Vector3 startPosition) {

    player.position = startPosition;
    player.startPosition = startPosition;
    std:: cout << "player position: \n";
    DebugPrintVector(player.position);
    player.rotation.y = levels[levelIndex].startingRotationY;
    if (levelIndex == 0 && unlockEntrances) player.rotation.y = 90.0f; //face opposite direction when leaving dungeon. 
    player.velocity = {0, 0, 0};
    player.grounded = false;
    player.groundY = 0.0;

    InitBlunderbuss(weapon);
    InitSword(meleeWeapon);
    InitMagicStaff(magicStaff);
    InitCrossbow();
    

    player.inventory.SetupItemTextures();
    playerInit = true;

    if (first){
        first = false; // player first starting position uses first as well, it's set to false here
        player.inventory.AddItem("HealthPotion");
    
    }

    
}

static inline Vector3 FootSample(const Player& p, float offX, float offZ)
{
    Vector3 s = p.position;
    s.y = player.position.y - player.height / 2.0f; //feet position
    s.x += offX;
    s.z += offZ;
    // y not needed for tile lookup; keep it for completeness
    return s;
}

void UpdatePlayerGrapple(Player& player, float dt)
{
    //try making player not grounded when grappling. shouldn't matter that code doesn't run. 
    if (player.state != PlayerState::Grappling) return;

    player.harpoonLifeTimer = (player.harpoonLifeTimer > 0.0f)
        ? (player.harpoonLifeTimer - dt)
        : 0.0f;

    if (player.harpoonLifeTimer <= 0.0f)
    {
        player.state = PlayerState::Normal;
        return;
    }

    // --- XZ-only chase ---
    // float keepY = player.position.y; // preserve current vertical

    Vector3 target = player.grappleTarget;
    // target.y = keepY;                // force same Y

    float dist = DistXZ(target, player.position);

    if (dist <= player.grappleStopDist)
    {
        player.state = PlayerState::Normal;
        player.velocity = {0,0,0};
        player.grappleBulletId = -1;
        return;
    }

    player.position.y -= (player.GRAVITY * 0.125) * dt; // a wee bit of gravity

    Vector3 toTarget = Vector3Subtract(target, player.position);
    toTarget.y = 0.0f; // extra safety

    // Normalize in XZ
    float invDist = 1.0f / dist;
    Vector3 dir = { toTarget.x * invDist, 0.0f, toTarget.z * invDist };

    float step = player.grappleSpeed * dt;
    if (step > dist) step = dist; // avoid overshoot

    player.position.x += dir.x * step;
    player.position.z += dir.z * step;
    //player.position.y = keepY;    // keep locked
    
    player.velocity = {0,0,0};
}

void Player::EquipNextWeapon() {
    meleeWeapon.model.materials[3].maps[MATERIAL_MAP_DIFFUSE].texture = 
        R.GetTexture("swordClean");

    if (collectedWeapons.empty()) {
        activeWeapon = WeaponType::None;
        currentWeaponIndex = -1;
        return;
    }

    WeaponType previous = activeWeapon;

    // cycle
    currentWeaponIndex = (currentWeaponIndex + 1) % collectedWeapons.size();
    activeWeapon = collectedWeapons[currentWeaponIndex];

    // Weapon swap animations
    if (previous == WeaponType::Sword) {
        meleeWeapon.equipDip = 80.0f;   // start low
    }

    // If we PUT AWAY the blunderbuss → force it into a dipped state
    if (previous == WeaponType::Blunderbuss) {
        weapon.reloadDip       = 25.0f;  // dip off-screen
    }

    if (previous == WeaponType::MagicStaff) {
        magicStaff.equipDip = 100.0f;
    }
}



void HandlePlayerMovement(float deltaTime){
    float dt = deltaTime;
    if (!player.canMove) return;

    // --- build desired direction in local space 
    Vector2 wish = {0,0};
    if (IsKeyDown(KEY_W)) wish.y += 1;
    if (IsKeyDown(KEY_S)) wish.y -= 1;
    if (IsKeyDown(KEY_A)) wish.x += 1;
    if (IsKeyDown(KEY_D)) wish.x -= 1;

    if (CameraSystem::Get().GetMode() == CamMode::Free){
        wish = {0,0};
    }

    player.running = IsKeyDown(KEY_LEFT_SHIFT) && player.canRun;
    const float maxSpeed = player.running ? player.runSpeed : player.walkSpeed;

    Vector3 desiredVel = {0,0,0};
    if (wish.x != 0 || wish.y != 0) {
        float yaw = DEG2RAD * player.rotation.y;
        Vector3 f = { sinf(yaw), 0, cosf(yaw) };
        Vector3 r = { f.z, 0, -f.x };
        Vector3 moveDir = Vector3Normalize({ r.x*wish.x + f.x*wish.y, 0, r.z*wish.x + f.z*wish.y });
        desiredVel = Vector3Scale(moveDir, maxSpeed);
        player.isMoving = true;
        player.forward = f;
    } else {
        player.isMoving = false;
    }

    // --- accelerate toward desired velocity (ground vs air)
    auto approach = [](float current, float target, float rate, float dt){
        float delta = target - current;
        float step  = rate * dt;
        if      (delta >  step) return current + step;
        else if (delta < -step) return current - step;
        else                    return target;
    };

    float accel   = player.grounded ? player.ACCEL_GROUND : player.ACCEL_AIR;
    float decel   = player.grounded ? player.DECEL_GROUND : player.FRICTION_AIR;

    player.velocity.x = approach(player.velocity.x, desiredVel.x, (fabsf(desiredVel.x) > 0.001f) ? accel : decel, dt);
    player.velocity.z = approach(player.velocity.z, desiredVel.z, (fabsf(desiredVel.z) > 0.001f) ? accel : decel, dt);

    const bool falling = (player.velocity.y <= 0.0f) && !player.grounded;

    // --- gravity
    if (player.state != PlayerState::Grappling){

        float g = falling ? player.GRAVITY * 2 : player.GRAVITY;               // e.g. 2400 (your units)
        //if (falling) g = player.GRAVITY*2.5;     // e.g. 2.2f

        player.velocity.y -= g * dt;

    } 

    HandleJumpButton(GetTime());
    
    TryQueuedJump();
    
    player.position = Vector3Add(player.position, Vector3Scale(player.velocity, dt)); //apply movement
    
    

}

void HandleKeyboardInput(Camera& camera) {

    // Right mouse state //blocking
    const bool rmb = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);

    // Desired block state this frame
    const bool wantBlock = rmb && (player.activeWeapon == WeaponType::Sword);

    // Transition into block
    if (wantBlock && !player.blocking) {
        meleeWeapon.StartBlock();
    }

    // Transition out of block (includes: switching off the sword while still holding RMB)
    if (!wantBlock && player.blocking) {
        meleeWeapon.EndBlock();
    }

    // Commit state
    player.blocking = wantBlock;

    // Other RMB actions (only when not blocking with sword)
    if (rmb && player.activeWeapon == WeaponType::MagicStaff) {
        magicStaff.Fire(camera);
    }

    if (rmb && player.activeWeapon == WeaponType::Crossbow) {
        crossbow.FireHarpoon(camera);
    }

    if (IsKeyPressed(KEY_Q))
    {
        meleeWeapon.model.materials[3].maps[MATERIAL_MAP_DIFFUSE].texture = R.GetTexture("swordClean"); //wipe off the blood on sword
        // Swap weapons
        WeaponType newWeapon = player.previousWeapon;
        player.previousWeapon = player.activeWeapon;
        player.activeWeapon = newWeapon;

        // Apply equip / reload dip based on weapon we switched TO
        switch (player.activeWeapon)
        {
            case WeaponType::Sword:
                meleeWeapon.equipDip = 50;
                break;

            case WeaponType::Blunderbuss:
                weapon.reloadDip = 40;
                break;

            case WeaponType::Crossbow:
                crossbow.reloadDip = 40;
                break;

            case WeaponType::MagicStaff:
                magicStaff.equipDip = 50;
                break;
        }
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (!player.isSwimming){ //dont fire gun in water
           if (player.activeWeapon == WeaponType::Blunderbuss){
                weapon.Fire(camera); 
           } 
        }else{
            if (player.activeWeapon == WeaponType::Blunderbuss) SoundManager::GetInstance().Play("reload"); //play "click" if in water with gun
        }

        if (player.activeWeapon == WeaponType::Sword){
            meleeWeapon.StartSwing(camera);
        }

        if (player.activeWeapon == WeaponType::MagicStaff){
                magicStaff.StartSwing(camera);
           }

        if (player.activeWeapon == WeaponType::Crossbow){
            crossbow.Fire(camera);
        }
    }

    // --- Boarding Check ---
    if (!player.onBoard) { //board the boat, lock player position to boat position, keep free look
        float distanceToBoat = Vector3Distance(player.position, player_boat.position);
        if (distanceToBoat < 300.0f && IsKeyPressed(KEY_E)) {
            player.onBoard = true;
            player_boat.playerOnBoard = true;
            player.position = Vector3Add(player_boat.position, {0, 200.0f, 0}); // sit up a bit
            return; // skip rest of update this frame
        }
    }

    // --- Exit Boat ---
    if (player.onBoard && IsKeyPressed(KEY_E)) {
        player.onBoard = false;
        player_boat.playerOnBoard = false;
        player.position = Vector3Add(player_boat.position, {2.0f, 200.0f, 0.0f}); // step off
    }

    // --- Sync Player to Boat ---
    if (player.onBoard) {
        player.position = Vector3Add(player_boat.position, {0, 200.0f, 0});
    }

    if (IsKeyPressed(KEY_ONE) && player.activeWeapon != WeaponType::Sword ){
        meleeWeapon.model.materials[3].maps[MATERIAL_MAP_DIFFUSE].texture = R.GetTexture("swordClean");
        player.previousWeapon = player.activeWeapon;
        player.activeWeapon = WeaponType::Sword;
        meleeWeapon.equipDip = 50.0f;   // start low
        
    }

    if (IsKeyPressed(KEY_TWO) && hasCrossbow && player.activeWeapon != WeaponType::Crossbow ){
        player.previousWeapon = player.activeWeapon;
        player.activeWeapon = WeaponType::Crossbow;
        crossbow.reloadDip = 40;
    }

    if (IsKeyPressed(KEY_THREE) && hasBlunderbuss && player.activeWeapon != WeaponType::Blunderbuss ){
        player.previousWeapon = player.activeWeapon;
        player.activeWeapon = WeaponType::Blunderbuss;
        weapon.reloadDip = 40;
    }

    if (IsKeyPressed(KEY_FOUR) && hasStaff && player.activeWeapon != WeaponType::MagicStaff){
        player.previousWeapon = player.activeWeapon;
        player.activeWeapon = WeaponType::MagicStaff;
        magicStaff.equipDip = 50;
    }

    if (IsKeyPressed(KEY_F)){
        //use health potion
        if (player.inventory.HasItem("HealthPotion") && !player.dying){ //don't use pot when dying
            
            if (player.currentHealth < player.maxHealth){ //don't use pot when full health
                player.currentHealth = player.maxHealth;
                player.inventory.UseItem("HealthPotion");
                SoundManager::GetInstance().Play("gulp");
            }
        }
    }

    if (IsKeyPressed(KEY_G) || IsKeyPressed(KEY_LEFT_ALT)){
        if (player.inventory.HasItem("ManaPotion")){
            if (player.currentMana < player.maxMana){
                player.currentMana = player.maxMana;
                player.inventory.UseItem("ManaPotion");
                SoundManager::GetInstance().Play("gulp");
            }
        }
    }

    if (IsKeyPressed(KEY_T)){
       if (magicStaff.magicType == MagicType::Fireball){
            magicStaff.magicType = MagicType::Iceball;
       }else{
            magicStaff.magicType = MagicType::Fireball;
       }
    }

}


void UpdateBlockHitbox(Player& player, float blockDistance = 500.0f, float width = 300.0f, float height = 64.0f) {
    //BlockHitbox is a large rectange that covers an area in front of the player. It is active when blocking. If enemies are inside the rectangle
    //there attacks will be blocked. 
    if (!player.blocking) return;

    Vector3 forward = {
        sinf(DEG2RAD * player.rotation.y),
        0,
        cosf(DEG2RAD * player.rotation.y)
    };

    Vector3 center = Vector3Add(player.position, Vector3Scale(forward, blockDistance));
    center.y = player.position.y;

    player.blockHitbox.min = {
        center.x - width / 2.0f,
        center.y - height / 2.0f,
        center.z - width / 2.0f
    };
    player.blockHitbox.max = {
        center.x + width / 2.0f,
        center.y + height / 2.0f,
        center.z + width / 2.0f
    };
}

BoundingBox Player::GetBoundingBox() const {
    float halfWidth = (200* 0.5f * 0.4f) / 2.0f; 
    float halfHeight = (200 * 0.5f) / 2.0f;

    return {
        { player.position.x - halfWidth, player.position.y - halfHeight, player.position.z - halfWidth },
        { player.position.x + halfWidth, player.position.y + halfHeight, player.position.z + halfWidth }
    };
}


void PlaySwimOnce()
{
    if (isDungeon) return;
    static const std::array<const char*,4> KEYS = { "swim1","swim2","swim3","swim4" };
    static int lastIndex = -1;
    static Sound current = {};  // raylib Sound handle of the *last* played clip

    // If a previous swim sound is still playing, do nothing
    if (current.frameCount > 0 && IsSoundPlaying(current)){
         return;

    }

    int idx;
    do { idx = GetRandomValue(0, (int)KEYS.size() - 1); }
    while (idx == lastIndex && KEYS.size() > 1);
    lastIndex = idx;

    current = SoundManager::GetInstance().GetSound(KEYS[idx]);
    PlaySound(current);
}




void Player::PlayFootstepSound() {
    if (player.isSwimming) return;
    static std::vector<std::string> footstepKeys = { "step1", "step2", "step3", "step4" };
    static int lastIndex = -1;

    int index;
    do {
        index = GetRandomValue(0, footstepKeys.size() - 1);
    } while (index == lastIndex && footstepKeys.size() > 1);  // avoid repeat if more than 1

    lastIndex = index;
    std::string stepKey = footstepKeys[index];

    SoundManager::GetInstance().Play(stepKey);
}

void UpdateFootsteps(float deltaTime){

    if (player.isMoving && player.grounded && !player.onBoard) {
        player.footstepTimer += deltaTime;

        float interval = player.running ? 0.4f : 0.6f;

        if (player.footstepTimer >= interval) {
            player.PlayFootstepSound();
            player.footstepTimer = 0.0f;
        }
    } else {
        player.footstepTimer = 0.0f;
    }

}

void UpdateSwimSounds(float deltaTime){
    if (!player.isSwimming) return;
    if (player.isMoving && player.isSwimming && !player.onBoard) {
        player.swimTimer += deltaTime;
        float interval = 0.9f;

        if (player.swimTimer >= interval) {
            PlaySwimOnce();
            player.swimTimer = 0.0f;
        }
    } else {
        player.swimTimer = 0.0f;
    }

}

void UpdateMeleeHitbox(Camera& camera){
    if (meleeWeapon.hitboxActive || magicStaff.hitboxActive) {
        Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        //Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, { 0, 1, 0 }));

        Vector3 hitboxCenter = Vector3Add(player.position, Vector3Scale(forward, 200.0f));
        hitboxCenter.y += 0.0f; 

        Vector3 boxSize = {100.0f, 100.0f, 100.0f};

        Vector3 min = {
            hitboxCenter.x - boxSize.x * 0.5f,
            hitboxCenter.y - boxSize.y * 0.5f,
            hitboxCenter.z - boxSize.z * 0.5f
        };
        Vector3 max = {
            hitboxCenter.x + boxSize.x * 0.5f,
            hitboxCenter.y + boxSize.y * 0.5f,
            hitboxCenter.z + boxSize.z * 0.5f
        };

        player.meleeHitbox = { min, max };
    } else {
        // Collapse the hitbox to prevent accidental damage
        player.meleeHitbox = { player.position, player.position };
    }
}

void InitSword(MeleeWeapon& meleeWeapon){
    //init sword
    meleeWeapon.model = R.GetModel("swordModel");
    meleeWeapon.scale = {2, 2, 2};
    meleeWeapon.model.materials[3].maps[MATERIAL_MAP_DIFFUSE].texture = R.GetTexture("swordClean");
    
}

void InitBlunderbuss(Weapon& weapon){
    //init blunderbuss
    weapon.model = R.GetModel("blunderbuss");
    weapon.scale = { 2.0f, 2.0f, 2.0f };
    weapon.muzzleFlashTexture = R.GetTexture("muzzleFlash");
    weapon.fireCooldown = 2.0f;
    weapon.flashDuration = 1.0;

}

void InitMagicStaff(MagicStaff& magicStaff) {
    magicStaff.model = R.GetModel("staffModel");
    magicStaff.scale = {1.0f, 1.0f, 1.0f};
    magicStaff.muzzleFlashTexture = R.GetTexture("muzzleFlash");
    magicStaff.fireCooldown = 1.0f;

    //set starting offsets here
    magicStaff.forwardOffset = 75.0f;
    magicStaff.sideOffset = 28.0f;
    magicStaff.verticalOffset = -25.0f;

   
}

void InitCrossbow()
{
    Model& model = R.GetModel("crossbow");
    Model& model2 = R.GetModel("crossbowRest");
    
    crossbow.loadedModel = model;
    crossbow.restModel   = model2;
    crossbow.state = CrossbowState::Loaded;

    // Reset any weird import transform
    crossbow.loadedModel.transform = MatrixIdentity();
    crossbow.restModel.transform = MatrixIdentity();
    // Rotate the model 180 degrees around Y so its "forward" matches your gun rig
    Matrix flipY = MatrixRotateY(PI); // 180 degrees
    crossbow.loadedModel.transform = flipY;
    crossbow.restModel.transform = flipY;

    player.crossbowMuzzlePos = crossbow.muzzlePos; //hackish
    



}



void OnGroundCheck(bool groundedNow, float timeNow) {
    if (groundedNow) player.lastGroundedTime = timeNow;
    player.grounded = groundedNow;
}

void HandleJumpButton(float timeNow){
    OnGroundCheck(player.grounded, timeNow);
    if (IsKeyPressed(KEY_SPACE)) player.lastJumpPressedTime = timeNow;
    
}

void TryQueuedJump(){
    float now = GetTime(); // or your own clock
    bool canCoyote = (now - player.lastGroundedTime) <= player.COYOTE_TIME;
    bool buffered  = (now - player.lastJumpPressedTime) <= player.JUMP_BUFFER;

    if (buffered && canCoyote) {
        player.velocity.y = player.jumpStrength;
        player.grounded = false;
        // consume buffer so we don’t multi-fire
        player.lastJumpPressedTime = -999.f;
    }
}

void TriggerMonsterDoors(){
    for (Door& door : doors){
        float distanceTo = Vector3Distance(player.position, door.position);
        if (distanceTo > 2000.0f) continue; //only check for close doors

        if (door.doorType == DoorType::Monster && !door.monsterTriggered){
            //LineOfSightRaycast(WorldToImageCoords(door.position), WorldToImageCoords(player.position), dungeonImg, 5, 0.001)
            if (HasWorldLineOfSight(door.position, player.position, 0.1, LOSMode::Lighting)){
                door.monsterTriggered = true;
                door.monsterTimer = 5.0f;
                break;
            }

        }

    }
}


//move this someplace
constexpr float LAVA_DROP      = 250.0f;
constexpr float VOID_DROP      = 12000.0f;  // how far you "fall"
constexpr float VOID_KILL_PAD  = 10000.0f;    // extra depth below that
constexpr float VOID_SNAP_REENABLE_Y = 200.0f;  // how close to a floor you must be before snapping is allowed again, 1 tileSize

constexpr float VOID_COMMIT_FALL = 200.0f;      // how far below the "expected floor" before we commit to void fall
constexpr float VOID_SNAP_MAX_UP = 200.0f;      // max upward snap allowed (prevents big teleports)



void UpdatePlayer(Player& player, float deltaTime, Camera& camera) {

    HandleMouseLook(deltaTime);
    TriggerMonsterDoors();
    weapon.Update(deltaTime);
    weapon.isMoving = player.isMoving;
    meleeWeapon.isMoving = player.isMoving;
    magicStaff.isMoving = player.isMoving;
    crossbow.isMoving = player.isMoving;
    meleeWeapon.Update(deltaTime);
    magicStaff.Update(deltaTime);
    crossbow.Update(deltaTime);

    UpdateMeleeHitbox(camera);
    UpdateFootsteps(deltaTime);

    UpdateBlockHitbox(player, 250, 300, 100);
    if (player.state == PlayerState::Frozen){
        vignetteFade += deltaTime * 0.25; //keep vignette for longer if frozen
    }else{
        vignetteFade += deltaTime * 2.0f; 
    }

    vignetteIntensity = Clamp(1.0f - vignetteFade, 0.0f, 1.0f);
    
    //FOV punch on hit
    if (player.hitTimer > 0.0f){
        player.hitTimer -= deltaTime;
        //camera.fovy = Lerp(camera.fovy, player.targetFOV, deltaTime * 12.0f); // fast zoom
    }else{
        player.hitTimer = 0;
        //camera.fovy = Lerp(camera.fovy, 45, deltaTime * 4.0f); // smooth return
    }

    float goldLerpSpeed = 5.0f;
    player.displayedGold += (player.gold - player.displayedGold) * goldLerpSpeed * deltaTime;

    if (player.running && player.isMoving && player.stamina > 0.0f) {
        if (!debugInfo) player.stamina -= deltaTime * 15.0f; //20.0f drain rate
        if (player.stamina <= 0.0f) {
            player.stamina = 0.0f;
            player.canRun = false; // forced stop
        }
    }
    else {
        // Recover stamina
        player.stamina += deltaTime * 20.0f; // regen rate
        if (player.stamina > 50.0f) {
            player.canRun = true; // can run as soon as some stamina is back
        }
        if (player.stamina >= player.maxStamina) {
            player.stamina = player.maxStamina;
        }
    }


    // === Swimming Check ===
    if (player.position.y <= waterHeightY + player.height / 2.0f && !player.onBoard) { //player.height is the mid point not the feet. 
        player.isSwimming = true;
        player.canRun = false;
        UpdateSwimSounds(deltaTime);
    } else {
        player.isSwimming = false;
        if (player.stamina > 50.0) player.canRun = true;
    }

    //start the dying process. 
    if (player.dying) {

        player.deathTimer += deltaTime;

        //player.canMove = false;
        vignetteIntensity = 1.0f; //should stay red becuase its set to 1 everyframe. 
        vignetteFade = 0.0f;

        if (!player.overVoid && !player.overLava){ //dont animate death cam if you fall into a pit. it would warp you back up. same for lava
            CameraSystem::Get().SnapAllToPlayer();
            CameraSystem::Get().StartDeathCam(100.0f, GetHeightAtWorldPosition(player.position, heightmap, terrainScale));
        }

        //fall to the ground dead first, then fade out.
        if (player.deathTimer > 2.0f){
            gFadePhase = FadePhase::FadingOut; //dont fadeout to level, just fade out. updateFade handles the rest.
            //fading out stops all updates 

        }

        if (player.deathTimer > 4.0f) { 
            
            player.dying = false;
            player.dead = true;
        }
    }

    if (player.dead) {
        // Reset position and state
        player.position = player.startPosition + Vector3 {0, 100, 0}; //high off ground 
        player.velocity = {}; 
        player.currentHealth = player.maxHealth;
        player.dead = false;
        player.canMove = true;
        CameraSystem::Get().SetMode(CamMode::Player);

    }



    //PLAYER MOVEMENT KEYBOARD INPUT
    //if (controlPlayer){
        HandleKeyboardInput(camera);

    if (player.state == PlayerState::Grappling) {
        UpdatePlayerGrapple(player, deltaTime);
        // skip normal movement update
    } else if (player.state == PlayerState::Frozen){
        if (player.freezeTimer > 0.0){
            player.freezeTimer -= deltaTime;

        }else{
            player.state = PlayerState::Normal;
            player.freezeTimer = 0.0f;
            player.canFreeze = true;
        }


    }else{
        if (!player.onBoard) HandlePlayerMovement(deltaTime);
    } 
    //}

    // === Ground Check ===
    float feetY = (player.position.y - player.height * 0.5f);
    // === Ground Check (multi-sample) ===
    float footOff = 20.0f; // try 10–40 depending on your tile size / player radius

    Vector3 samples[9] = {
        FootSample(player,  0,        0),        // center

        FootSample(player, +footOff,  0),        // E
        FootSample(player, -footOff,  0),        // W
        FootSample(player,  0,       +footOff),  // N
        FootSample(player,  0,       -footOff),  // S

        FootSample(player, +footOff, +footOff),  // NE
        FootSample(player, -footOff, +footOff),  // NW
        FootSample(player, +footOff, -footOff),  // SE
        FootSample(player, -footOff, -footOff),  // SW
    };

    // ---- tuning knobs ----
    const float SNAP_EPS     = 5.0f;    // feet can be this far above a surface and still "snap"
    const float MAX_STEP_UP  = 150.0f;   // max allowed step up this frame (prevents wall-climb) -if we only climb halfway up the wall, we fall through the world. 

    // For choosing ground:
    float bestSupportingGroundY = -99999.0f;
    int   supportCount = 0;

    // (Optional) if you want to know if ANY foot is over void/lava
    bool anyOverVoid = false;
    bool anyOverLava = false;

    // We'll also compute a fallback center groundY (used if supportCount < 2)
    player.centerGroundY = 0.0f;
    if (!isDungeon)
    {
        player.centerGroundY = GetHeightAtWorldPosition(player.position, heightmap, terrainScale);
    }
    else
    {
        player.centerGroundY = floorHeight;

        float cx = GetDungeonImageX(player.position.x, tileSize, dungeonWidth);
        float cy = GetDungeonImageY(player.position.z, tileSize, dungeonHeight);

        if (IsLava(cx, cy)) player.centerGroundY -= LAVA_DROP;
        if (IsVoid(cx, cy)) player.centerGroundY -= VOID_DROP;
    }

    for (Vector3 p : samples)
    {
        float gy = 0.0f;

        if (!isDungeon)
        {
            gy = GetHeightAtWorldPosition(p, heightmap, terrainScale);
        }
        else
        {
            gy = floorHeight;

            float x = GetDungeonImageX(p.x, tileSize, dungeonWidth);
            float y = GetDungeonImageY(p.z, tileSize, dungeonHeight);

            bool overVoid = IsVoid(x, y);
            bool overLava = IsLava(x, y);

            if (overLava) { gy -= LAVA_DROP; anyOverLava = true; }
            if (overVoid) { gy -= VOID_DROP; anyOverVoid = true; }
        }

        // Use a small step-up only if grounded; otherwise prevent ledge “magnet” mid-jump
        const float MAX_STEP_UP_GROUNDED = 25.0f;  // your normal step
        const float MAX_STEP_UP_AIR      = 0.0f;   // basically “don’t step up in air”

        float maxStepUp = player.grounded ? MAX_STEP_UP_GROUNDED : MAX_STEP_UP_AIR;
        float stepUp = gy - feetY;

        bool supports =
            (feetY <= gy + SNAP_EPS) &&   // feet are close enough to "land" on it
            (stepUp <= maxStepUp);      // but not too far above feet (wall/platform side)

        if (supports)
        {
            supportCount++;
            if (gy > bestSupportingGroundY) bestSupportingGroundY = gy;
        }
    }

    // Choose groundY:
    // - need at least 2 supporting probes to "stand" on that higher surface
    // - otherwise fallback to center to prevent 1-probe hovering/climbing
    if (supportCount >= 5) //more feet required to stay on platform. lower is more forgiving. 5/9 hit samples means your on a platform.
    {
        player.groundY = bestSupportingGroundY;
    }
    else
    {
        player.groundY = player.centerGroundY;
    }

    // Keep your existing dungeon flags working
    if (isDungeon)
    {
        player.overVoid = anyOverVoid;
        player.overLava = anyOverLava;
    }


    // --- VOID FALL LATCH ---
    if (isDungeon && player.overVoid)
    {
        player.isFallingIntoVoid = true;
    }

    if (isDungeon && player.isFallingIntoVoid)
    {
        player.grounded = false;

        float killY = -1000.0f;
        if (player.position.y <= killY)
        {
            //player.currentHealth = 0;
            if (!player.dying){
                player.dying = true;
                player.deathTimer = 2.0f;
                player.TakeDamage(9999);

            }

        }

        if (!player.overVoid)
        {
            float snapDelta = (player.groundY + player.height / 2.0f) - player.position.y;
            if (snapDelta >= 0.0f && snapDelta <= VOID_SNAP_REENABLE_Y)
            {
                player.isFallingIntoVoid = false;
            }
        }

        // skip normal snap
    }
    else
    {
        const bool falling = (player.velocity.y <= 0.0f) && !player.grounded;

        // Optional extra safety: don't allow snapping UP more than MAX_STEP_UP
        float targetY = player.groundY + player.height / 2.0f;
        if (targetY > player.position.y + MAX_STEP_UP)
        {
            player.grounded = false;
        }
        else if (feetY <= player.groundY + 5.0f && falling)
        {
            player.grounded = true;
            player.velocity.y = 0.0f;
            player.position.y = targetY;
        }
        else
        {
            player.grounded = false;
        }
    }

    player.previousPosition = player.position;

}

void Player::TakeDamage(int amount){
    if (godMode) return;

    CameraSystem::Get().Shake(0.004f, 0.25f); // 0.004 barely perceptable

    if (!player.dying && !player.dead) {
        player.currentHealth -= amount;

        if (player.currentHealth <= 0) {
            player.dying = true;
            player.deathTimer = 0.0f;
           
        }
    }
    hitTimer = 0.15;
    if (player.state == PlayerState::Frozen) {
        vignetteMode = 1;
        vignetteIntensity = 1.0f;
        vignetteFade = 0.0f;
    }else{
        vignetteMode = 0;
        vignetteIntensity = 1.0f;
        vignetteFade = 0.0f;
    }

    if (rand() % 2 == 0){
        SoundManager::GetInstance().Play("phit1");
    }else{
        SoundManager::GetInstance().Play("phit2");
    }

}

void DrawWeapons(const Player& player, Camera& camera) {

        //draw weapon
    if (controlPlayer) {
        switch (player.activeWeapon) {
            case WeaponType::Blunderbuss:
                weapon.Draw(camera);
                break;
            case WeaponType::Sword:
                meleeWeapon.Draw(camera);
                if (meleeWeapon.hitboxActive) {
                    // DrawBoundingBox(player.meleeHitbox, RED);
                }
                if (player.blocking) {
                    // DrawBoundingBox(player.blockHitbox, RED);
                }
                break;
            case WeaponType::MagicStaff:
                magicStaff.Draw(camera);
                //DrawBoundingBox(player.meleeHitbox, RED);
                break;

            case WeaponType::Crossbow:
                crossbow.Draw(camera);
                break;
            case WeaponType::None:
                // draw nothing
                break;
        }
    }

}

void DrawPlayer(const Player& player, Camera& camera) {
    if (CameraSystem::Get().GetMode() == CamMode::Free){
        DrawCapsule(player.position, Vector3 {player.position.x, player.height/2, player.position.z}, 5, 4, 4, RED);
        DrawBoundingBox(player.GetBoundingBox(), RED);
    }

    if (player.debugShowFootSamples)
    {
        float footOff = 1.0f; // try 10–40 depending on your tile size / player radius

        Vector3 samples[9] = {
            FootSample(player,  0,        0),        // center

            FootSample(player, +footOff,  0),        // E
            FootSample(player, -footOff,  0),        // W
            FootSample(player,  0,       +footOff),  // N
            FootSample(player,  0,       -footOff),  // S

            FootSample(player, +footOff, +footOff),  // NE
            FootSample(player, -footOff, +footOff),  // NW
            FootSample(player, +footOff, -footOff),  // SE
            FootSample(player, -footOff, -footOff),  // SW
        };

        for (int i = 0; i < 9; ++i)
        {
            Vector3 p = samples[i];

            float gy = isDungeon
                ? player.centerGroundY
                : GetHeightAtWorldPosition(p, heightmap, terrainScale);

            Vector3 top = { p.x, p.y + 60.0f, p.z };
            Vector3 hit = { p.x, gy,          p.z };

            DrawLine3D(top, hit, (i == 0) ? ORANGE : LIME);
            DrawSphere(hit, 3.0f, RED);        // ground hit point
            DrawSphere(p,   3.0f, SKYBLUE);    // sample origin
        }
    }

    //DrawBoundingBox(player.meleeHitbox, WHITE);
 


}


