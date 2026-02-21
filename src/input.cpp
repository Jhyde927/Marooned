#include "input.h"
#include "raymath.h"
#include "world.h"
#include <iostream>
#include "player.h"
#include "utilities.h"
#include "lighting.h"
#include "resourceManager.h"
#include "sound_manager.h"
#include "vegetation.h"
#include "dungeonGeneration.h"


InputMode currentInputMode = InputMode::KeyboardMouse;
//TODO: add controller support 

void debugControls(Camera& camera, float deltaTime){

    if (IsKeyPressed(KEY_GRAVE)){ // ~
        debugInfo = !debugInfo;
    }

    if (debugInfo){

        if (IsKeyPressed(KEY_J)){
            raft.AddBody();
        }

        if (IsKeyDown(KEY_M)){
            //M for murder
            player.TakeDamage(9999);
        }

        if (IsKeyDown(KEY_K)){
            player.hasGoldKey = true;
            player.hasSilverKey = true;
            player.hasSkeletonKey = true;
        }

        if (IsKeyPressed(KEY_L)) {

            std::cout << "Player Position: ";
            DebugPrintVector(player.position);

            //Reloading lights live, now works, still flashes dark for one frame though, so we can't really recalculate for door openings. 
            // isLoadingLevel = true;
            // InitDynamicLightmap(dungeonWidth * 4);
           
            // BuildStaticLightmapOnce(dungeonLights);
            // R.SetLightingShaderValues();
            // isLoadingLevel = false;

        
        }

        if (IsKeyPressed(KEY_SLASH)){
            RemoveAllVegetation();
        }

        if (IsKeyPressed(KEY_P)){
            //teleport player to free cam pos
            MovePlayerToFreeCam();
        }

        if (IsKeyPressed(KEY_O)){
            DebugOpenAllDoors();
        }

        if (IsKeyPressed(KEY_APOSTROPHE)){ //hide ceiling for better screenshots. 
            drawCeiling = !drawCeiling; //maybe just don't draw ceiling ever in free cam. 
        }
        //control player with arrow keys while in free cam. 
        Vector3 input = {0};
        if (IsKeyDown(KEY_UP)) input.z += 1;
        if (IsKeyDown(KEY_DOWN)) input.z -= 1;
        if (IsKeyDown(KEY_LEFT)) input.x += 1;
        if (IsKeyDown(KEY_RIGHT)) input.x -= 1;

        player.running = IsKeyDown(KEY_LEFT_SHIFT) && player.canRun;
        float speed = player.running ? player.runSpeed : player.walkSpeed;

        if (input.x != 0 || input.z != 0) {
            input = Vector3Normalize(input);
            float yawRad = DEG2RAD * player.rotation.y;
            player.isMoving = true;
            Vector3 forward = { sinf(yawRad), 0, cosf(yawRad) };
            Vector3 right = { forward.z, 0, -forward.x };

            Vector3 moveDir = {
                forward.x * input.z + right.x * input.x,
                0,
                forward.z * input.z + right.z * input.x
            };

            moveDir = Vector3Scale(Vector3Normalize(moveDir), speed * deltaTime);
            player.position = Vector3Add(player.position, moveDir);
            player.forward = forward;
        }

    }


    //give all weapons
    if (IsKeyPressed(KEY_SEMICOLON) && debugInfo) {
        if (player.collectedWeapons.size() <= 1) {
            // player.collectedWeapons.push_back(WeaponType::Crossbow);
            // player.collectedWeapons.push_back(WeaponType::Blunderbuss);
            // player.collectedWeapons.push_back(WeaponType::MagicStaff); //we no longer use collectedWeapons vector, but we still have it becuase we might need it maybe. 

            hasBlunderbuss = true; //just a bool for each weapon simplified things. 
            hasCrossbow = true;
            hasHarpoon = true;
            hasStaff = true;
            player.activeWeapon = WeaponType::Blunderbuss;
        }

    }
}

void HandleMouseLook(float deltaTime){
    Vector2 mouseDelta = GetMouseDelta();
    float mouseSensitivity = 0.05f;
    player.rotation.y -= mouseDelta.x * mouseSensitivity;
    player.rotation.x -= mouseDelta.y * mouseSensitivity;
    player.rotation.x = Clamp(player.rotation.x, -30.0f, 30.0f);


    // --- ALWAYS recompute forward from yaw ---
    float yaw = DEG2RAD * player.rotation.y;
    player.forward = Vector3Normalize({ sinf(yaw), 0.0f, cosf(yaw) });
    
}

float Expo(float x)
{
    const float expo = 1.6f;
    return copysignf(powf(fabsf(x), expo), x);
}

void HandleGamepadLook(float dt)
{
    if (!IsGamepadAvailable(0)) return;

    float rx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
    float ry = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);

    rx = Expo(rx);
    ry = Expo(ry);

    const float DEADZONE = 0.15f;
    if (fabsf(rx) < DEADZONE) rx = 0.0f;
    if (fabsf(ry) < DEADZONE) ry = 0.0f;

    const float yawSpeed   = 120.0f; // horizontal
    const float pitchSpeed = 70.0f;  // vertical (lower!)

    player.rotation.y -= rx * yawSpeed   * dt;
    player.rotation.x -= ry * pitchSpeed * dt;

    player.rotation.x = Clamp(player.rotation.x, -30.0f, 30.0f);
}





