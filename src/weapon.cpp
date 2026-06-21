#include "weapon.h"
#include "raymath.h"
#include <iostream>
#include "rlgl.h"
#include "bullet.h"
#include "sound_manager.h"
#include "resourceManager.h"
#include "player.h"
#include "world.h"
#include "ui.h"

void MeleeWeapon::Init()
{
    //scale = { 1.0f, 1.0f, 1.0f };

    swingTimer = 0.0f;
    swingDuration = 0.7f;
    swinging = false;

    hitboxActive = false;
    hitboxTimer = 0.0f;
    hitboxDuration = 0.25f;

    hitWindowStart = 0.1f;
    hitWindowEnd = 0.25f;
    hitboxTriggered = false;

    blocking = false;
    blockLerp = 0.0f;
    blockSpeed = 6.0f;

    // Block pose offsets
    blockForwardOffset = 50.0f;
    blockVerticalOffset = -50.0f;
    blockSideOffset = 0.0f;

    bobbingTime = 0.0f;
    isMoving = false;
    bobVertical = 0.0f;
    bobSide = 0.0f;

    swingAmount = 20.0f;
    swingOffset = 0.0f;

    verticalSwingOffset = -30.0f;
    verticalSwingAmount = 60.0f;

    horizontalSwingOffset = 0.0f;
    horizontalSwingAmount = 20.0f;

    equipDip = 0.0f;

    timeSinceLastSwing = 999.0f;

    forwardOffset = 60.0f;
    sideOffset = 15.0f;
    verticalOffset = -45.0f;

    currentAttack = SwordAttackType::RightSlash;

    comboIndex = 0;
    comboTimer = 0.0f;
    comboResetTime = 0.75f;

    normalCooldown = 0.75f;
    comboCooldown = 0.45f;

    attackForwardOffset = 0.0f;
    attackSideOffset = 20.0f;
    attackVerticalOffset = -45.0f;

    attackYawDeg = 0.0f;
    attackRollDeg = 0.0f;

    baseRollDeg = -10.0f; // idle sword tilt around Z
}

float MeleeWeapon::GetCurrentDamage() const
{
    switch (currentAttack)
    {
        case SwordAttackType::RightSlash:
            return 50.0f;

        case SwordAttackType::LeftSlash:
            return 50.0f;

        case SwordAttackType::Stab:
            return 100.0f;
    }

    return 25.0f;
}


SwordAttackType GetComboAttack(int comboIndex)
{
    switch (comboIndex)
    {
        case 0: return SwordAttackType::RightSlash;
        case 1: return SwordAttackType::LeftSlash;
        case 2: return SwordAttackType::Stab;
        default: return SwordAttackType::RightSlash;
    }
}

void Crossbow::FireHarpoon(Camera& camera) {
    if (!hasHarpoon) return;
    if (!harpoonReady) return;
    float now = GetTime();
    if (now - lastFired < fireCooldown) return;
    
    // Don't fire if we're reloading or not in loaded state
    if (isReloading || state != CrossbowState::Loaded) return;

    lastFired     = now;
    triggeredFire = true;

    // Show rest model visually (string forward)
    state = CrossbowState::Rest;

    // Start delay BEFORE dip
    reloadDelayTimer  = 0.0f;
    isReloading       = false;
    reloadPhase       = 0.0f;
    swappedModelMidDip = false;
    harpoonTimer = 0.0f;
    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    FireCrossbowHarpoon(muzzlePos, camForward, 2000.0f, 4.0f, false);
    SoundManager::GetInstance().Play("crossbowFire");
    SoundManager::GetInstance().Play("harpoon");

    recoil = recoilAmount;  // kick back
    
}


void Crossbow::Fire(Camera& camera)
{
    float now = GetTime();
    if (now - lastFired < fireCooldown) return;
    if (isReloading || state != CrossbowState::Loaded) return;

    lastFired     = now;
    triggeredFire = true;

    state = CrossbowState::Rest;

    reloadDelayTimer   = 0.0f;
    isReloading        = false;
    reloadPhase        = 0.0f;
    swappedModelMidDip = false;

    Vector2 ret = { GetScreenWidth()*0.5f, GetScreenHeight()*0.5f + 50.0f };
    Ray r = GetMouseRay(ret, camera);

    Vector3 boltDir = Vector3Normalize(r.direction);
    FireCrossbow(muzzlePos, boltDir, 4000.0f, 5.0f, false);

    SoundManager::GetInstance().Play("crossbowFire");

    recoil = recoilAmount;
}

void Crossbow::Reload()
{
    state = CrossbowState::Loaded;
    reloadDelayTimer = 0.0f;
    isReloading = false;
    reloadPhase = 0.0f;
    SoundManager::GetInstance().Play("crossbowReload");
}


void Weapon::Fire(Camera& camera) {

    if (GetTime() - lastFired >= fireCooldown) {
        SoundManager::GetInstance().Play("shotgun");
        
        recoil = recoilAmount;
        lastFired = GetTime();

        activeMuzzleFlashes.push_back({
            muzzlePos,
            R.GetTexture("muzzleFlash"),
            flashSize,
            0.1f  // lifetime in seconds
        });

        
        // Schedule reload sound
        reloadScheduled = true;
        reloadTimer = 0.0f;

        //offset bulletOrigin to weapon position. 
        Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, { 0, 1, 0 }));
        Vector3 camUp = { 0, 1, 0 };

        // Offsets in local space
        float forwardOffset = -50.0f;
        float sideOffset = 30.0f;
        float verticalOffset = -30.0f; // down

        // Final origin for bullets in world space
        Vector3 bulletOrigin = camera.position;
        bulletOrigin = Vector3Add(bulletOrigin, Vector3Scale(camForward, forwardOffset));
        bulletOrigin = Vector3Add(bulletOrigin, Vector3Scale(camRight, sideOffset));
        bulletOrigin = Vector3Add(bulletOrigin, Vector3Scale(camUp, verticalOffset));



        FireBlunderbuss(
            bulletOrigin,
            camForward,
            player.spreadDegrees,    // spreadDegrees 
            7,        // pelletCount
            2000.0f,   // bulletSpeed //anymore than 2k seems to tunnel through enemies. Maybe we could do something about that. if faster bullets feels better. 
            3.0f,      // lifetimeSeconds
            false
        );
    }


}

void Crossbow::Update(float dt)
{
    crossbow.isMoving = player.isMoving;
    harpoonTimer += dt;

    bool readyNow = (harpoonTimer >= harpoonCooldown);
    if (!harpoonReady && readyNow)
    {
        SoundManager::GetInstance().Play("crossbowReload");
    }

    harpoonReady = readyNow;

    if (!player.onBoard){ // dont bob while on board boat
            // --- Bobbing ---
        if (isMoving) {
            bobbingTime += dt * 8.0f;
            bobVertical = sinf(bobbingTime) * 1.0f;
            bobSide     = sinf(bobbingTime * 0.25f) * 1.0f;
        } else {
            bobVertical = Lerp(bobVertical, 0.0f, dt * 5.0f);
            bobSide     = Lerp(bobSide,     0.0f, dt * 5.0f);
        }

    }



    // --- Recoil recovery ---
    recoil = Lerp(recoil, 0.0f, dt * recoilRecoverySpeed);

    // --- Reload delay BEFORE dip ---
    if (state == CrossbowState::Rest && !isReloading) {
        reloadDelayTimer += dt;
        if (reloadDelayTimer >= autoReloadDelay) {
            // Now start the dip
            isReloading        = true;
            reloadPhase        = 0.0f;
            swappedModelMidDip = false;
        }
    }

    // --- Reload dip animation ---
    if (isReloading) {
        reloadPhase += dt * reloadSpeed;

        if (reloadPhase >= 1.0f) {
            reloadPhase  = 1.0f;
            isReloading  = false;
            reloadDip    = 0.0f;
            
            // fully back up and ready to fire again
            state = CrossbowState::Loaded;
        } else {
            float t = reloadPhase;          // 0..1
            float dipCurve = sinf(t * PI);  // 0 → 1 → 0
            reloadDip = dipCurve * reloadDipAmount;

            // Mid-dip: switch to loaded model at the bottom
            if (!swappedModelMidDip && reloadPhase >= 0.8f) { //80 percent reloaded play the sound so it snaps right when it's ready. 
                swappedModelMidDip = true;
                state = CrossbowState::Loaded;  // string is now visually back
                SoundManager::GetInstance().Play("crossbowReload");
            }
        }
    } else {
        reloadDip = Lerp(reloadDip, 0.0f, dt * 10.0f);
    }
}

void MeleeWeapon::UpdateRightSlashMotion(float t)
{
    float speedFactor = 2.0f;  // higher = faster downstroke
    float tFast = Clamp(t * speedFactor, 0.0f, 1.0f);

    float arc = sinf(tFast * (PI * 0.6f));

    swingOffset = arc * swingAmount;

    // tweak vertical so the sword ends low and forward
    float halfArc = arc;

    verticalSwingOffset   = -halfArc * verticalSwingAmount * 2.0f;
    horizontalSwingOffset = -halfArc * horizontalSwingAmount * 3.0f;

    attackForwardOffset = 0.0f;
    attackSideOffset = 0.0f;
    attackVerticalOffset = 0.0f;

    attackYawDeg = 0.0f;
    attackRollDeg = 0.0f;

}

void MeleeWeapon::UpdateLeftSlashMotion(float t)
{
    float speedFactor = 1.0f; //2.0
    float tFast = Clamp(t * speedFactor, 0.0f, 1.0f);

    float arc = sinf(tFast * (PI * 0.6f));

    swingOffset = arc * swingAmount;

    verticalSwingOffset = -arc * verticalSwingAmount * 3.0f;

    // mirrored horizontal sweep
    horizontalSwingOffset = arc * horizontalSwingAmount * 2.0f;

    // left slash starts more to the left side of screen
    attackSideOffset = -25.0f;

    // maybe pull it slightly forward so it does not feel buried in the camera
    attackForwardOffset = 20.0f;

    // maybe raise/lower a touch depending on the model
    attackVerticalOffset = 0.0f;

    // Y axis was the useful one
    attackYawDeg = -20.0f;
    attackRollDeg = 30.0f;
    
}

void MeleeWeapon::UpdateStabMotion(float t)
{
    // t is 0.0 to 1.0 across swingDuration

    float extendEnd = 0.30f;
    float holdEnd   = 0.80f; // hold from 30% to 60% of the stab
    float retractT  = 0.0f;

    float jab = 0.0f;

    if (t < extendEnd)
    {
        // 0 -> 1 quickly
        float p = t / extendEnd;
        jab = sinf(p * PI * 0.5f); // ease out
    }
    else if (t < holdEnd)
    {
        // stay fully extended
        jab = 1.0f;
    }
    else
    {
        // 1 -> 0
        float p = (t - holdEnd) / (1.0f - holdEnd);
        jab = 1.0f - p;
    }

    swingOffset = jab * 75.0f;

    horizontalSwingOffset = 0.0f;
    verticalSwingOffset = -10.0f;

    attackForwardOffset = 25.0f;
    attackSideOffset = 0.0f;
    attackVerticalOffset = -20.0f;

    attackYawDeg = 0.0f;
    attackRollDeg = 0.0f;
}


void MeleeWeapon::Update(float deltaTime) {
    if (player.activeWeapon != WeaponType::Sword) return;
    meleeWeapon.isMoving = player.isMoving;
    if (!player.onBoard){
        float amplitude = 1.0f;
        bobVertical = sinf(bobbingTime) * amplitude;
        bobSide = sinf(bobbingTime * 0.5f) * amplitude * 0.5f;

    }
    // Smooth transition into and out of block pose
    if (blocking) {
        blockLerp += deltaTime * blockSpeed;
    } else {
        blockLerp -= deltaTime * blockSpeed;
    }
    blockLerp = Clamp(blockLerp, 0.0f, 1.0f);

    attackRollDeg = 0.0f;
    attackYawDeg = 0.0f;
    if (swinging) {
        swingTimer += deltaTime;

        float t = swingTimer / swingDuration;
        t = Clamp(t, 0.0f, 1.0f);

        switch (currentAttack)
        {
            case SwordAttackType::RightSlash:
                UpdateRightSlashMotion(t);
                break;

            case SwordAttackType::LeftSlash:
                UpdateLeftSlashMotion(t);
                break;

            case SwordAttackType::Stab:
                UpdateStabMotion(t);
                break;
        }

        // float t = swingTimer / swingDuration;
        // float speedFactor = 2.0f;  // higher = faster downstroke
        // float tFast = Clamp(t * speedFactor, 0.0f, 1.0f);

        // float arc = sinf(tFast * (PI * 0.6f));

        // swingOffset = arc * swingAmount;


        // // tweak vertical so the sword ends low and forward
        // float halfArc = arc; // reuse
        // verticalSwingOffset   = -halfArc * verticalSwingAmount*2;
        // horizontalSwingOffset = -halfArc * horizontalSwingAmount*3;

        // Delay turning off swinging until next frame
        if (swingTimer >= swingDuration) {
            swinging = false;

        }

                // Hitbox activation window
        if (!hitboxTriggered && swingTimer >= hitWindowStart && swingTimer <= hitWindowEnd) {
            hitboxActive = true;
            hitboxTriggered = true;
            hitboxTimer = 0.0f;
        }
    } else {

        //const float returnSpeed = 25.0f;
        bool comboStillAlive = comboTimer <= comboResetTime && comboIndex > 0;
        float returnSpeed = comboStillAlive ? 8.0f : 25.0f;
  
        swingOffset = Lerp(swingOffset, 0.0f, deltaTime * returnSpeed);
        verticalSwingOffset = Lerp(verticalSwingOffset, 0.0f, deltaTime * returnSpeed);
        horizontalSwingOffset = Lerp(horizontalSwingOffset, 0.0f, deltaTime * returnSpeed);

        attackForwardOffset = Lerp(attackForwardOffset, 0.0f, deltaTime * returnSpeed);
        attackSideOffset = Lerp(attackSideOffset, 0.0f, deltaTime * returnSpeed);
        attackVerticalOffset = Lerp(attackVerticalOffset, 0.0f, deltaTime * returnSpeed);
        attackYawDeg = Lerp(attackYawDeg, 0.0f, deltaTime * returnSpeed);
        // const float returnSpeed = 25.0f;

        // swingOffset         = Lerp(swingOffset,         0.0f, deltaTime * returnSpeed);
        // verticalSwingOffset = Lerp(verticalSwingOffset, 0.0f, deltaTime * returnSpeed);
        // horizontalSwingOffset = Lerp(horizontalSwingOffset, 0.0f, deltaTime * returnSpeed);
    }

    timeSinceLastSwing += deltaTime;

    comboTimer += deltaTime;

    if (!swinging && comboTimer > comboResetTime) {
        comboIndex = 0;
    }

    // Hitbox timing
    if (hitboxActive) {
        hitboxTimer += deltaTime;
        if (hitboxTimer > hitboxDuration) {
            hitboxActive = false;
        }
    }

    // 🔹 Smoothly raise weapon from dipped state
    const float equipReturnSpeed = 10.0f; // tweak to taste
    equipDip = Lerp(equipDip, 0.0f, deltaTime * equipReturnSpeed);

    if (isMoving) {
        bobbingTime += deltaTime * 12.0f; 
    } else {
        // Smoothly return to idle
        bobbingTime = 0.0f;
        bobVertical = Lerp(bobVertical, 0.0f, deltaTime * 5.0f);
        bobSide = Lerp(bobSide, 0.0f, deltaTime * 5.0f);
        return;
    }
}

static float Approach(float v, float target, float delta)
{
    if (v < target) return (v + delta > target) ? target : v + delta;
    else            return (v - delta < target) ? target : v - delta;
}



void Weapon::Update(float deltaTime) {
    weapon.isMoving = player.isMoving;
    float targetBloom = 0.0f;

    if (!isMoving)               targetBloom = 0.0f;   // stopped
    else if (!player.running)    targetBloom = 0.45f;  // walking
    else                         targetBloom = 1.0f;   // running

    float rate = (targetBloom > player.crosshairBloom) ? bloomExpandRate : bloomShrinkRate;

    player.crosshairBloom = Approach(player.crosshairBloom, targetBloom, rate * deltaTime);


    // Handle delayed reload sound
    if (reloadScheduled) {
        reloadTimer += deltaTime; //you can still reload while weapon isn't equipped
        if (reloadTimer >= reloadDelay) {

            SoundManager::GetInstance().Play("reload");
            reloadScheduled = false;
        }

        if (reloadTimer > 0.5f){
            reloadDip = Lerp(reloadDip, 20.0f, deltaTime * 20.0f); // dip fast
        }
    }

    if (player.activeWeapon != WeaponType::Blunderbuss) return; //dont update the rest if not equipped. 

    if (!player.onBoard){
        float amplitude = 0.5f;
        bobVertical = sinf(bobbingTime) * amplitude;
        bobSide = sinf(bobbingTime * 0.5f) * amplitude * 0.5f;

    }


    if (recoil > 0.0f) { 
        recoil -= recoilRecoverySpeed * deltaTime; 
        if (recoil < 0.0f) 
        recoil = 0.0f; 
    }

    if (flashTimer > 0.0f) {
        flashTimer -= deltaTime;
        if (flashTimer <= 0.0f) {
            flashTriggered = false;  // turn off after visible frame
        }
    }



    if (reloadDip > 0.0f) {
        reloadDip -= 100.0f * deltaTime; 
        if (reloadDip < 0.0f) reloadDip = 0.0f;
    }

    if (isMoving) {
        bobbingTime += deltaTime * 12.0f; 
    }  else {
        // Smoothly return to idle (but keep updating crosshairBloom/spread)
        bobbingTime = 0.0f;
        bobVertical = Lerp(bobVertical, 0.0f, deltaTime * 5.0f);
        bobSide     = Lerp(bobSide, 0.0f, deltaTime * 5.0f);
    }

}






void Weapon::Draw(const Camera& camera) {
    // === Camera rotation math ===
    Matrix lookAt = MatrixLookAt(camera.position, camera.target, { 0, 1, 0 });
    Matrix gunRotation = MatrixInvert(lookAt);
    Quaternion q = QuaternionFromMatrix(gunRotation);

    float angle = 2.0f * acosf(q.w);
    float angleDeg = angle * RAD2DEG;
    float sinTheta = sqrtf(1.0f - q.w * q.w);
    Vector3 axis = (sinTheta < 0.001f) ? Vector3{1, 0, 0} : Vector3{ q.x / sinTheta, q.y / sinTheta, q.z / sinTheta };

    // === Camera basis vectors ===
    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, { 0, 1, 0 }));
    Vector3 camUp = { 0, 1, 0 };

    // === Aspect ratio correction ===
    float screenAspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    float baseAspect = 16.0f / 9.0f;

    float aspectCorrection = (baseAspect - screenAspect) * 10.0f;  // Tune this multiplier
    float correctedSideOffset = sideOffset - aspectCorrection;

    // === Gun positioning ===
    float dynamicForward = forwardOffset - recoil;
    float dynamicVertical = verticalOffset - reloadDip + bobVertical;

    Vector3 gunPos = camera.position;
    gunPos = Vector3Add(gunPos, Vector3Scale(camForward, dynamicForward));
    gunPos = Vector3Add(gunPos, Vector3Scale(camRight, correctedSideOffset + bobSide));
    gunPos = Vector3Add(gunPos, Vector3Scale(camUp, dynamicVertical));

    // === Muzzle position and drawing ===
    muzzlePos = Vector3Add(gunPos, Vector3Scale(camForward, 40.0f));
    Color tint = TintFromDarkness(weaponDarkness);


    DrawModelEx(model, gunPos, axis, angleDeg, scale, tint);

}

void MeleeWeapon::Draw(const Camera& camera) {
    Matrix lookAt = MatrixLookAt(camera.position, camera.target, { 0, 1, 0 });
    Matrix swordRotation = MatrixInvert(lookAt);
    Quaternion q = QuaternionFromMatrix(swordRotation);

    float angle = 2.0f * acosf(q.w);
    float angleDeg = angle * RAD2DEG;
    float sinTheta = sqrtf(1.0f - q.w * q.w);
    Vector3 axis = (sinTheta < 0.001f) ? Vector3{1, 0, 0} : Vector3{ q.x / sinTheta, q.y / sinTheta, q.z / sinTheta };

    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, { 0, 1, 0 }));
    Vector3 camUp = { 0, 1, 0 };

    // === Blend offsets between normal and block pose ===
    float blendedForward = Lerp(forwardOffset, blockForwardOffset, blockLerp);
    float blendedSide = Lerp(sideOffset, blockSideOffset, blockLerp);
    float blendedVertical = Lerp(verticalOffset, blockVerticalOffset, blockLerp);

    // === Apply swing arc if not blocking ===
    blendedForward += swingOffset;
    blendedSide += horizontalSwingOffset;
    blendedVertical += verticalSwingOffset + bobVertical;

    blendedForward += attackForwardOffset;
    blendedSide += attackSideOffset;
    blendedVertical += attackVerticalOffset;

    // 🔹 Apply equip dip (positive = push sword *down*)
    blendedVertical -= equipDip;

    // === Final sword position ===
    //Vector3 swordPos = camera.position;
    Vector3 swordPos = {camera.position.x, camera.position.y-5, camera.position.z};
    swordPos = Vector3Add(swordPos, Vector3Scale(camForward, blendedForward));
    swordPos = Vector3Add(swordPos, Vector3Scale(camRight, blendedSide + bobSide));
    swordPos = Vector3Add(swordPos, Vector3Scale(camUp, blendedVertical));
    Color tint = TintFromDarkness(weaponDarkness);

    Matrix cameraRot = QuaternionToMatrix(q);

    // Permanent idle/model correction
    Matrix baseRoll = MatrixRotateZ(baseRollDeg * DEG2RAD);

    // Attack-specific rotations
    Matrix attackYaw = MatrixRotateY(attackYawDeg * DEG2RAD);
    Matrix attackRoll = MatrixRotateZ(attackRollDeg * DEG2RAD);

    // Combined weapon-local rotation
    Matrix extraRot = MatrixMultiply(baseRoll, attackYaw);
    extraRot = MatrixMultiply(attackRoll, extraRot);

    Matrix finalRot = MatrixMultiply(extraRot, cameraRot);

    Quaternion finalQ = QuaternionFromMatrix(finalRot);

    float finalAngle = 2.0f * acosf(finalQ.w);
    float finalAngleDeg = finalAngle * RAD2DEG;
    float finalSinTheta = sqrtf(1.0f - finalQ.w * finalQ.w);

    Vector3 finalAxis = (finalSinTheta < 0.001f)
        ? Vector3{1, 0, 0}
        : Vector3{
            finalQ.x / finalSinTheta,
            finalQ.y / finalSinTheta,
            finalQ.z / finalSinTheta
        };

    DrawModelEx(model, swordPos, finalAxis, finalAngleDeg, scale, tint);

}



void MeleeWeapon::StartBlock() {
    blocking = true;
    swinging = false; // cancel any swings
}

void MeleeWeapon::EndBlock() {
    blocking = false;
}

void MeleeWeapon::StartSwing(Camera& camera) {
    bool comboStillAlive = comboTimer <= comboResetTime && comboIndex > 0;
    float requiredCooldown = comboStillAlive ? comboCooldown : normalCooldown;

    if (timeSinceLastSwing >= requiredCooldown && !blocking)  {
        PlaySwipe();

             // Choose which attack this swing is
        currentAttack = GetComboAttack(comboIndex);

        //player swipe with code triggers here. 
        Vector2 basePos = { GetScreenWidth() * 0.60f, GetScreenHeight() * 0.70f };
        //SpawnSwordSlash(basePos);

        SpawnSwordSlashForAttack(currentAttack);
        // Reset combo timer because we successfully continued/started the chain
        comboTimer = 0.0f;
        
        player.attackId++; //for unique id each attack, so eggs are only damaged once. 
        swinging = true;
        swingTimer = 0.0f;
        timeSinceLastSwing = 0.0f;
        player.meleeVolume.attackId = player.attackId;
        hitboxActive = false;
        hitboxTimer = 0.0f;
        hitboxTriggered = false;

        // Advance combo for next click
        comboIndex++;

        if (comboIndex > 2) {
            comboIndex = 0;
        }


        switch (currentAttack)
        {
            case SwordAttackType::RightSlash:
                TraceLog(LOG_INFO, "Sword combo: RightSlash");
                break;

            case SwordAttackType::LeftSlash:
                TraceLog(LOG_INFO, "Sword combo: LeftSlash");
                break;

            case SwordAttackType::Stab:
                TraceLog(LOG_INFO, "Sword combo: Stab");
                break;
        }
    }


}


void MeleeWeapon::PlaySwipe(){

    static std::vector<std::string> swipes = {"swipe1", "swipe2", "swipe3"};
    static int lastIndex = -1;

    int index;
    do {
        index = GetRandomValue(0, swipes.size() - 1);
    } while (index == lastIndex && swipes.size() > 1);  // avoid repeat if more than 1

    lastIndex = index;
    std::string stepKey = swipes[index];

    SoundManager::GetInstance().Play(stepKey);

}

void MagicStaff::Fire(const Camera& camera) {

    if (GetTime() - lastFired < fireCooldown) return;

    if (player.currentMana >= 10){
        player.currentMana -= 10;
    }else{
        return;
    }

    lastFired = GetTime();
    recoil += recoilAmount;
    //flashTimer = flashDuration;

    activeMuzzleFlashes.push_back({
            muzzlePos,
            R.GetTexture("muzzleFlash"),
            flashSize,
            0.1f  // lifetime in seconds
    });

    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 targetPoint = Vector3Add(camera.position, Vector3Scale(camForward, 1000.0f));
    
    if (magicType == MagicType::Fireball){
        FireFireball(muzzlePos, targetPoint, 2000, 10.0f, false, false, false);
    }else{
        FireIceball(muzzlePos, targetPoint, 2000, 10.0f, false, false);
    }
    
    
}

void MagicStaff::PlaySwipe(){

    static std::vector<std::string> swipes = {"swipe1", "swipe2", "swipe3"};
    static int lastIndex = -1;

    int index;
    do {
        index = GetRandomValue(0, swipes.size() - 1);
    } while (index == lastIndex && swipes.size() > 1);  // avoid repeat if more than 1

    lastIndex = index;
    std::string stepKey = swipes[index];
    SoundManager::GetInstance().Play(stepKey);

}

void MagicStaff::StartSwing(Camera& camera) {
    if (swinging || timeSinceLastSwing < cooldown) return;
    PlaySwipe();
    player.attackId++;
    swinging = true;
    swingTimer = 0.0f;
    hitboxActive = false;
    hitboxTriggered = false;
    timeSinceLastSwing = 0.0f;
    //spawn slash effect
    Vector2 basePos = { GetScreenWidth() * 0.70f, GetScreenHeight() * 0.50f };
    SpawnSwordSlash(basePos);

    // Apply initial swing offsets
    swingOffset = -swingAmount;
    verticalSwingOffset = -verticalSwingAmount;
    horizontalSwingOffset = -horizontalSwingAmount;

}


void MagicStaff::Update(float deltaTime) {
    hitboxActive = false;
    if (player.activeWeapon != WeaponType::MagicStaff) return;
    magicStaff.isMoving = player.isMoving;
    if (!player.onBoard){
        float amplitude = 2.0f;
        bobVertical = sinf(bobbingTime) * amplitude;
        bobSide = sinf(bobbingTime * 0.5f) * amplitude * 0.5f;

    }


    // Melee swing timer

    if (swinging) {
        swingTimer += deltaTime;
        if (swingTimer >= hitWindowStart && swingTimer <= hitWindowEnd) {
            hitboxActive = true;
        }

        if (swingTimer >= swingDuration) {
            swinging = false;
            hitboxActive = false;
            hitboxTriggered = false;
        }

    }


    timeSinceLastSwing += deltaTime;

    // Recoil recovery
    recoil -= recoilRecoverySpeed * deltaTime;
    if (recoil < 0.0f) recoil = 0.0f;

    // Muzzle flash
    flashTimer -= deltaTime;
    if (flashTimer < 0.0f) flashTimer = 0.0f;

    swingOffset = Lerp(swingOffset, 0.0f, deltaTime * 10.0f);
    verticalSwingOffset = Lerp(verticalSwingOffset, 0.0f, deltaTime * 10.0f);
    horizontalSwingOffset = Lerp(horizontalSwingOffset, 0.0f, deltaTime * 10.0f);

    // 🔹 Smoothly raise weapon from dipped state
    const float equipReturnSpeed = 10.0f; // tweak to taste
    equipDip = Lerp(equipDip, 0.0f, deltaTime * equipReturnSpeed);

    if (isMoving) {
        bobbingTime += deltaTime * 12.0f; 
    } else {
        // Smoothly return to idle
        bobbingTime = 0.0f;
        bobVertical = Lerp(bobVertical, 0.0f, deltaTime * 5.0f);
        bobSide = Lerp(bobSide, 0.0f, deltaTime * 5.0f);
        return;
    }


}

void MagicStaff::Draw(const Camera& camera) {
    // Camera orientation basis
    Matrix lookAt = MatrixLookAt(camera.position, camera.target, { 0, 1, 0 });
    Matrix staffRotation = MatrixInvert(lookAt);
    Quaternion q = QuaternionFromMatrix(staffRotation);

    float angle = 2.0f * acosf(q.w);
    float angleDeg = angle * RAD2DEG;
    float sinTheta = sqrtf(1.0f - q.w * q.w);
    Vector3 axis = (sinTheta < 0.001f) ? Vector3{1, 0, 0} : Vector3{ q.x / sinTheta, q.y / sinTheta, q.z / sinTheta };

    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, { 0, 1, 0 }));
    Vector3 camUp = { 0, 1, 0 };

    // === Apply swing arc offsets ===
    float finalForward = forwardOffset + swingOffset - recoil;
    float finalSide = sideOffset + horizontalSwingOffset;
    float finalVertical = verticalOffset + verticalSwingOffset - reloadDip + bobVertical;

        // 🔹 Apply equip dip (positive = push sword *down*)
    finalVertical -= equipDip;

    // === Final staff position ===
    Vector3 staffPos = camera.position;
    staffPos = Vector3Add(staffPos, Vector3Scale(camForward, finalForward));
    staffPos = Vector3Add(staffPos, Vector3Scale(camRight, finalSide + bobSide));
    staffPos = Vector3Add(staffPos, Vector3Scale(camUp, finalVertical));



    muzzlePos = Vector3Add(staffPos, Vector3Scale(camForward, 40.0f));
    Color tint = TintFromDarkness(weaponDarkness);
    DrawModelEx(model, staffPos, axis, angleDeg, scale, tint);


}


void Crossbow::Draw(const Camera& camera) {
    // === Camera rotation math ===
    Matrix lookAt = MatrixLookAt(camera.position, camera.target, { 0, 1, 0 });
    Matrix gunRotation = MatrixInvert(lookAt);
    Quaternion q = QuaternionFromMatrix(gunRotation);

    float angle = 2.0f * acosf(q.w);
    float angleDeg = angle * RAD2DEG;
    float sinTheta = sqrtf(fmaxf(0.0f, 1.0f - q.w * q.w));
    Vector3 axis = (sinTheta < 0.001f)
        ? Vector3{ 1, 0, 0 }
        : Vector3{ q.x / sinTheta, q.y / sinTheta, q.z / sinTheta };

    // === Camera basis vectors ===
    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 camRight   = Vector3Normalize(Vector3CrossProduct(camForward, { 0, 1, 0 }));
    Vector3 camUp      = Vector3CrossProduct(camRight, camForward);

    Vector3 camDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

    // === Aspect ratio correction ===
    float screenAspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    float baseAspect   = 16.0f / 9.0f;

    float aspectCorrection    = (baseAspect - screenAspect) * 10.0f;
    float correctedSideOffset = sideOffset - aspectCorrection;

    // === Weapon positioning ===
    float dynamicForward  = forwardOffset - recoil;
    float dynamicVertical = verticalOffset + bobVertical - reloadDip;

    Vector3 weaponPos = camera.position;
    weaponPos = Vector3Add(weaponPos, Vector3Scale(camForward, dynamicForward));
    weaponPos = Vector3Add(weaponPos, Vector3Scale(camRight,   correctedSideOffset + bobSide));
    weaponPos = Vector3Add(weaponPos, Vector3Scale(camUp,      dynamicVertical));

    // Muzzle: still using your rotated local offset
    Vector3 muzzleLocal = { 0.0f, 0.0f, 0.0f };
    Vector3 muzzleOffsetWorld = Vector3RotateByQuaternion(muzzleLocal, q);


    // final muzzle position in WORLD space
    muzzlePos = Vector3Add(weaponPos, muzzleOffsetWorld);


    Color tint = TintFromDarkness(weaponDarkness);

    Model& m = (state == CrossbowState::Loaded) ? loadedModel : restModel;
    DrawModelEx(m, weaponPos, axis, angleDeg, scale, tint);
}




// void Crossbow::Draw(const Camera& camera)
// {
//     // Rifle-style viewmodel placement
//     Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
//     Vector3 right   = Vector3Normalize(Vector3CrossProduct(forward, {0,1,0}));

//     Vector3 pos = camera.position;
//     pos = Vector3Add(pos, Vector3Scale(forward, forwardOffset + recoil));
//     pos = Vector3Add(pos, Vector3Scale(right,   sideOffset + bobSide));
//     pos.y += verticalOffset + bobVertical;

//     DrawModelEx(
//         model,
//         pos,
//         {0,1,0},         // axis
//         0.0f,            // no rotation here
//         scale,
//         WHITE
//     );
// }


