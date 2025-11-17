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
        float forwardOffset = 0.0f;
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
            2.0f,    // spreadDegrees 
            7,        // pelletCount
            2000.0f,   // bulletSpeed //anymore than 2k seems to tunnel through enemies. Maybe we could do something about that. if faster bullets feels better. 
            2.0f,      // lifetimeSeconds
            false
        );
    }


}

void MeleeWeapon::Update(float deltaTime) {
    float amplitude = 1.0f;
    bobVertical = sinf(bobbingTime) * amplitude;
    bobSide = sinf(bobbingTime * 0.5f) * amplitude * 0.5f;

    // Smooth transition into and out of block pose
    if (blocking) {
        blockLerp += deltaTime * blockSpeed;
    } else {
        blockLerp -= deltaTime * blockSpeed;
    }
    blockLerp = Clamp(blockLerp, 0.0f, 1.0f);


    if (swinging) {
        swingTimer += deltaTime;

        float t = swingTimer / swingDuration;
        if (t > 1.0f) t = 1.0f;

        // Nice arcs using sine curve
        float arc = sinf(t * PI);  // goes 0 → 1 → 0
        swingOffset = arc * swingAmount;
        float halfArc = sinf(t * PI); // goes 0 → 1 → 0
        verticalSwingOffset = -(halfArc - 0.25f) * 2.0f * verticalSwingAmount;

        //verticalSwingOffset = sinf((t + 0.25f) * 2 * PI) * verticalSwingAmount;
        horizontalSwingOffset = -sinf(t * PI) * horizontalSwingAmount;

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
        swingOffset = 0.0f;
        verticalSwingOffset = 0.0f;
        horizontalSwingOffset = 0.0f;
    }

    timeSinceLastSwing += deltaTime;

    // Hitbox timing
    if (hitboxActive) {
        hitboxTimer += deltaTime;
        if (hitboxTimer > hitboxDuration) {
            hitboxActive = false;
        }
    }

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


void Weapon::Update(float deltaTime) {
    float amplitude = 0.5f;
    bobVertical = sinf(bobbingTime) * amplitude;
    bobSide = sinf(bobbingTime * 0.5f) * amplitude * 0.5f;


    if (recoil > 0.0f) {
        recoil -= recoilRecoverySpeed * deltaTime;
        if (recoil < 0.0f) recoil = 0.0f;
    }


    if (flashTimer > 0.0f) {
        flashTimer -= deltaTime;
        if (flashTimer <= 0.0f) {
            flashTriggered = false;  // turn off after visible frame
        }
    }

    

    // Handle delayed reload sound
    if (reloadScheduled) {
        reloadTimer += deltaTime;

        if (reloadTimer >= reloadDelay) {
            SoundManager::GetInstance().Play("reload");
            reloadScheduled = false;
        }

        if (reloadTimer > 0.5f){
            reloadDip = Lerp(reloadDip, 20.0f, deltaTime * 20.0f); // dip fast
        }
    }

    if (reloadDip > 0.0f) {
        reloadDip -= 100.0f * deltaTime; 
        if (reloadDip < 0.0f) reloadDip = 0.0f;
    }

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

// darkness in [0..1]: 0 = bright, 1 = very dark
inline Color WeaponTintFromDarkness(float darkness, Color base = {255,255,255,255})
{
    // How strong the darkening feels:
    const float strength = 0.55f; // raise for more darkening
    float f = 1.0f - strength * darkness; // clamps to [0..1]
    if (f < 0.0f) f = 0.0f; else if (f > 1.0f) f = 1.0f;

    return (Color){
        (unsigned char)(base.r * f),
        (unsigned char)(base.g * f),
        (unsigned char)(base.b * f),
        base.a
    };
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
    Color tint = WeaponTintFromDarkness(weaponDarkness);
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

    // === Final sword position ===
    //Vector3 swordPos = camera.position;
    Vector3 swordPos = {camera.position.x, camera.position.y -10, camera.position.z};
    swordPos = Vector3Add(swordPos, Vector3Scale(camForward, blendedForward));
    swordPos = Vector3Add(swordPos, Vector3Scale(camRight, blendedSide + bobSide));
    swordPos = Vector3Add(swordPos, Vector3Scale(camUp, blendedVertical));
    Color tint = WeaponTintFromDarkness(weaponDarkness);
    DrawModelEx(model, swordPos, axis, angleDeg, scale, tint);
}



void MeleeWeapon::StartBlock() {
    blocking = true;
    swinging = false; // cancel any swings
}

void MeleeWeapon::EndBlock() {
    blocking = false;
}

void MeleeWeapon::StartSwing(Camera& camera) {
    if (timeSinceLastSwing >= cooldown && !blocking) {
        PlaySwipe();
        player.attackId++; //for unique id each attack, so eggs are only damaged once. 
        swinging = true;
        swingTimer = 0.0f;
        timeSinceLastSwing = 0.0f;

        hitboxActive = false;
        hitboxTimer = 0.0f;
        hitboxTriggered = false;
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
        FireFireball(muzzlePos, targetPoint, 2000, 10, false, false);
    }else{
        FireIceball(muzzlePos, targetPoint, 2000, 10, false);
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

    // Apply initial swing offsets
    swingOffset = -swingAmount;
    verticalSwingOffset = -verticalSwingAmount;
    horizontalSwingOffset = -horizontalSwingAmount;
}


void MagicStaff::Update(float deltaTime) {
    float amplitude = 2.0f;
    bobVertical = sinf(bobbingTime) * amplitude;
    bobSide = sinf(bobbingTime * 0.5f) * amplitude * 0.5f;

    // Melee swing timer
    if (swinging) {
        swingTimer += deltaTime;
        if (swingTimer >= swingDuration) {
            swinging = false;
            hitboxActive = false;
            hitboxTriggered = false;
        } else if (swingTimer >= hitWindowStart && swingTimer <= hitWindowEnd) {
            hitboxActive = true;
        } else {
            hitboxActive = false;
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

    // === Final staff position ===
    Vector3 staffPos = camera.position;
    staffPos = Vector3Add(staffPos, Vector3Scale(camForward, finalForward));
    staffPos = Vector3Add(staffPos, Vector3Scale(camRight, finalSide + bobSide));
    staffPos = Vector3Add(staffPos, Vector3Scale(camUp, finalVertical));

    muzzlePos = Vector3Add(staffPos, Vector3Scale(camForward, 40.0f));
    Color tint = WeaponTintFromDarkness(weaponDarkness);
    DrawModelEx(model, staffPos, axis, angleDeg, scale, tint);


}




