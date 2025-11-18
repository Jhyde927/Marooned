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

Weapon weapon;
MeleeWeapon meleeWeapon;
MagicStaff magicStaff;


//Refactor this whole mess at some point. 
void InitPlayer(Player& player, Vector3 startPosition) {

    player.position = startPosition;
    player.startPosition = startPosition;
    
    player.rotation.y = levels[levelIndex].startingRotationY;
    player.velocity = {0, 0, 0};
    player.grounded = false;
    player.groundY = 0.0;

    InitBlunderbuss(weapon);
    InitSword(meleeWeapon);
    InitMagicStaff(magicStaff);

    player.inventory.SetupItemTextures();
    playerInit = true;

    if (first){
        first = false; // player first starting position uses first as well, it's set to false here
        player.inventory.AddItem("HealthPotion");
    
    }
    
}


void Player::EquipNextWeapon() {
    meleeWeapon.model.materials[3].maps[MATERIAL_MAP_DIFFUSE].texture = R.GetTexture("swordClean"); //wipe the blood off. 
    if (collectedWeapons.empty()) {
       
        activeWeapon = WeaponType::None;
        currentWeaponIndex = -1;
        return;
    }
    
    currentWeaponIndex = (currentWeaponIndex + 1) % collectedWeapons.size();
    activeWeapon = collectedWeapons[currentWeaponIndex];
}


void HandlePlayerMovement(float deltaTime){
    float dt = deltaTime;
    if (!player.canMove) return;

    // --- build desired direction in local space (same as yours)
    Vector2 wish = {0,0};
    if (IsKeyDown(KEY_W)) wish.y += 1;
    if (IsKeyDown(KEY_S)) wish.y -= 1;
    if (IsKeyDown(KEY_A)) wish.x += 1;
    if (IsKeyDown(KEY_D)) wish.x -= 1;

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

    // --- gravity
    player.velocity.y += player.GRAVITY * dt;
    HandleJumpButton(GetTime());
    
    TryQueuedJump();

    player.position = Vector3Add(player.position, Vector3Scale(player.velocity, dt)); //apply movement

}

void HandleKeyboardInput(float deltaTime, Camera& camera) {

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

    if (IsKeyPressed(KEY_Q)) {
 
        player.EquipNextWeapon();
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
        player.position = Vector3Add(player_boat.position, {2.0f, 0.0f, 0.0f}); // step off
    }

    // --- Sync Player to Boat ---
    if (player.onBoard) {
        player.position = Vector3Add(player_boat.position, {0, 200.0f, 0});
    }

    if (IsKeyPressed(KEY_ONE)){
        //use health potion
        if (player.inventory.HasItem("HealthPotion") && !player.dying){ //don't use pot when dying
            
            if (player.currentHealth < player.maxHealth){
                player.currentHealth = player.maxHealth;
                player.inventory.UseItem("HealthPotion");
                SoundManager::GetInstance().Play("gulp");
            }
        }
    }

    if (IsKeyPressed(KEY_TWO)){
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
    static Sound current = {0};  // raylib Sound handle of the *last* played clip

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
        Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, { 0, 1, 0 }));

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



void UpdatePlayer(Player& player, float deltaTime, Camera& camera) {

    HandleMouseLook(deltaTime);
    weapon.Update(deltaTime);
    weapon.isMoving = player.isMoving;
    meleeWeapon.isMoving = player.isMoving;
    magicStaff.isMoving = player.isMoving;
    meleeWeapon.Update(deltaTime);
    magicStaff.Update(deltaTime);

    UpdateMeleeHitbox(camera);
    UpdateFootsteps(deltaTime);

    UpdateBlockHitbox(player, 250, 300, 100);
    vignetteFade += deltaTime * 2.0f; 
    vignetteIntensity = Clamp(1.0f - vignetteFade, 0.0f, 1.0f);
    
    //FOV punch on hit
    if (player.hitTimer > 0.0f){
        player.hitTimer -= deltaTime;
        camera.fovy = Lerp(camera.fovy, player.targetFOV, deltaTime * 12.0f); // fast zoom
    }else{
        player.hitTimer = 0;
        camera.fovy = Lerp(camera.fovy, 45, deltaTime * 4.0f); // smooth return
    }

    float goldLerpSpeed = 5.0f;
    player.displayedGold += (player.gold - player.displayedGold) * goldLerpSpeed * deltaTime;

    if (player.running && player.isMoving && player.stamina > 0.0f) {
        if (!debugInfo) player.stamina -= deltaTime * 30.0f; // drain rate
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
        player.velocity = {0.0f, 0.0f, 0.0f}; //stop moving when dying. should hide the gun as well. 
        player.canMove = false;
        vignetteIntensity = 1.0f; //should stay red becuase its set to 1 everyframe. 
        vignetteFade = 0.0f;

        gFadePhase = FadePhase::FadingOut; //dont fadeout to level, just fade out. updateFade handles the rest. 


        if (player.deathTimer > 1.5f) { 
            player.dying = false;
            player.dead = true;
        }
    }

    if (player.dead) {
        // Reset position and state
        player.position = player.startPosition;
        player.velocity = {0}; 
        player.currentHealth = player.maxHealth;
        player.dead = false;
        player.canMove = true;
        

    }

    //PLAYER MOVEMENT KEYBOARD INPUT
    if (controlPlayer){
        HandleKeyboardInput(deltaTime, camera);
        if (!player.onBoard) HandlePlayerMovement(deltaTime);
    } 

    // === Ground Check ===
    //check gound after moving, use clamped deltaTime. 
    player.groundY = GetHeightAtWorldPosition(player.position, heightmap, terrainScale);
    
    if (isDungeon) {
        player.groundY = dungeonPlayerHeight;
        if (player.overLava) {
            player.groundY -= 150.0f; // or LAVA_DROP
        }
    }
    float feetY = player.position.y - player.height / 2.0f;

    if (feetY <= player.groundY + 5.0f) { //+5 buffer for uneven terrain. 
        player.grounded = true;
        player.velocity.y = 0;
        player.position.y = player.groundY + player.height / 2.0f;
    } else {
        player.grounded = false;
    }

    player.previousPosition = player.position;
}

void Player::TakeDamage(int amount){

    if (!player.dying && !player.dead) {
        player.currentHealth -= amount;

        if (player.currentHealth <= 0) {
            player.dying = true;
            player.deathTimer = 0.0f;
           
        }
    }
    hitTimer = 0.15;
    vignetteIntensity = 1.0f;
    vignetteFade = 0.0f;

    if (rand() % 2 == 0){
        SoundManager::GetInstance().Play("phit1");
    }else{
        SoundManager::GetInstance().Play("phit2");
    }

}

void DrawPlayer(const Player& player, Camera& camera) {
    //DrawCapsule(player.position, Vector3 {player.position.x, player.height/2, player.position.z}, 5, 4, 4, RED);
    //DrawBoundingBox(player.GetBoundingBox(), RED);
    //DrawBoundingBox(player.meleeHitbox, WHITE);

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
                break;
            case WeaponType::None:
                // draw nothing
                break;
        }
    }
}


