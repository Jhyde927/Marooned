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


InputMode currentInputMode = InputMode::KeyboardMouse;
//TODO: add controller support 

void debugControls(Camera& camera, float deltaTime){

    if (IsKeyPressed(KEY_GRAVE)){ // ~
        debugInfo = !debugInfo;
    }



    if (debugInfo){

        if (IsKeyDown(KEY_O)) debugDoorOpenAngleDeg += 60.0f * GetFrameTime();  // open more
        if (IsKeyDown(KEY_P)) debugDoorOpenAngleDeg -= 60.0f * GetFrameTime();  // close

        if (IsKeyPressed(KEY_L)) {

            std::cout << "Player Position: ";
            DebugPrintVector(player.position);

            //Reloading lights live, now works, still flashes dark for one frame though, so we can't really recalculate for door openings. 
            isLoadingLevel = true;
            InitDynamicLightmap(dungeonWidth * 4);
           
            BuildStaticLightmapOnce(dungeonLights);
            R.SetLightingShaderValues();
            isLoadingLevel = false;

        
        }

        if (IsKeyPressed(KEY_SLASH)){
            RemoveAllVegetation();
        }

        if (IsKeyPressed(KEY_APOSTROPHE)){ //hide ceiling for better screenshots. 
            if (!drawCeiling){
                drawCeiling = true;
            }else{
                drawCeiling = false;
            }
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
            player.collectedWeapons.push_back(WeaponType::Crossbow);
            player.collectedWeapons.push_back(WeaponType::Blunderbuss);
            player.collectedWeapons.push_back(WeaponType::MagicStaff);
            hasBlunderbuss = true;
            hasCrossbow = true;
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
    
}



