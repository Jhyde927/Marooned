#include "weapon.h"
#include "raymath.h"
#include <iostream>
#include "rlgl.h"
#include "bullet.h"
#include "sound_manager.h"
#include "resources.h"
#include "player.h"



void Weapon::Fire(Camera& camera) {

    if (GetTime() - lastFired >= fireCooldown) {
        SoundManager::GetInstance().Play("shotgun");
        
        recoil = recoilAmount;
        lastFired = GetTime();
        flashTimer = flashDuration;
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
            2.0f,    // spreadDegrees (tweak this!)
            6,        // pelletCount
            2000.0f,   // bulletSpeed
            2.0f,      // lifetimeSeconds
            false
        );
    }


}

void MeleeWeapon::Update(float deltaTime) {

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
}


void Weapon::Update(float deltaTime) {
    if (recoil > 0.0f) {
        recoil -= recoilRecoverySpeed * deltaTime;
        if (recoil < 0.0f) recoil = 0.0f;
    }

    if (flashTimer > 0.0f) {
        flashTimer -= deltaTime;
        if (flashTimer < 0.0f) flashTimer = 0.0f;
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
}

void Weapon::Draw(const Camera& camera) {
    Matrix lookAt = MatrixLookAt(camera.position, camera.target, { 0, 1, 0 });
    Matrix gunRotation = MatrixInvert(lookAt);
    Quaternion q = QuaternionFromMatrix(gunRotation);

    float angle = 2.0f * acosf(q.w);
    float angleDeg = angle * RAD2DEG;
    float sinTheta = sqrtf(1.0f - q.w * q.w);
    Vector3 axis = (sinTheta < 0.001f) ? Vector3{1, 0, 0} : Vector3{ q.x / sinTheta, q.y / sinTheta, q.z / sinTheta };

    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camForward, { 0, 1, 0 }));
    Vector3 camUp = { 0, 1, 0 };

    float dynamicForward = forwardOffset - recoil;
    float dynamicVertical = verticalOffset - reloadDip;
    Vector3 gunPos = camera.position;
    gunPos = Vector3Add(gunPos, Vector3Scale(camForward, dynamicForward));
    gunPos = Vector3Add(gunPos, Vector3Scale(camRight, sideOffset));
    gunPos = Vector3Add(gunPos, Vector3Scale(camUp, dynamicVertical));


    if (flashTimer > 0.0f) {//Muzzle Flash
        Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        Vector3 muzzlePos = Vector3Add(gunPos, Vector3Scale(camForward, 40.0f));
        
        float flashSize = 120.0f; //could be bigger. 
        rlDisableDepthMask();
        DrawBillboard(camera, muzzleFlashTexture, muzzlePos, flashSize, WHITE);
        rlEnableDepthMask();

    }

    DrawModelEx(model, gunPos, axis, angleDeg, scale, WHITE);


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
    blendedVertical += verticalSwingOffset;

    // === Final sword position ===
    Vector3 swordPos = camera.position;
    swordPos = Vector3Add(swordPos, Vector3Scale(camForward, blendedForward));
    swordPos = Vector3Add(swordPos, Vector3Scale(camRight, blendedSide));
    swordPos = Vector3Add(swordPos, Vector3Scale(camUp, blendedVertical));

    DrawModelEx(model, swordPos, axis, angleDeg, scale, WHITE);
}



void MeleeWeapon::StartBlock() {
    blocking = true;
    swinging = false; // cancel any swings
}

void MeleeWeapon::EndBlock() {
    blocking = false;
}

void MeleeWeapon::StartSwing() {
    if (timeSinceLastSwing >= cooldown) {
        PlaySwipe();
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