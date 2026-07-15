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
#include "shaderSetup.h"
#include "game_settings.h"
#include "dungeon_props.h"


InputMode currentInputMode = InputMode::KeyboardMouse;

void ControlPlayerWhileFreeCam(float deltaTime){
    //control player with arrow keys while in free cam. 
    Vector3 input = {0, 0, 0};
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



void debugControls(Camera& camera, float deltaTime){
    //use console. 
    (void)camera;
    if (CameraSystem::Get().GetMode() == CamMode::Free){
        ControlPlayerWhileFreeCam(deltaTime); //move the player around with the arrow keys while controlling free cam. 
    }

    if (IsKeyPressed(KEY_F3))
    {
        ToggleThirdPerson();
    }

}

void HandleMouseLook()
{
    CameraSystem& cameraSystem = CameraSystem::Get();
    CamMode mode = cameraSystem.GetMode();

    const bool look =
        mode == CamMode::Player ||
        mode == CamMode::ThirdPerson;

    if (!look) return;

    Vector2 mouseDelta = GetMouseDelta();

    float sensitivity = GameSettings::mouseSensitivity;

    if (mode == CamMode::ThirdPerson)
    {
        sensitivity *= 1.0f;
    }

    player.rotation.y -= mouseDelta.x * sensitivity;
    player.rotation.x -= mouseDelta.y * sensitivity;
    player.rotation.x = Clamp(player.rotation.x, -30.0f, 30.0f);

    float yaw = player.rotation.y * DEG2RAD;
    float pitch = player.rotation.x * DEG2RAD;

    player.lookForward = Vector3Normalize({
        cosf(pitch) * sinf(yaw),
        sinf(pitch),
        cosf(pitch) * cosf(yaw)
    });

    player.forward = Vector3Normalize({
        sinf(yaw),
        0.0f,
        cosf(yaw)
    });
}

// void HandleMouseLook()
// {
//     const bool look = CameraSystem::Get().GetMode() == CamMode::Player ||  CameraSystem::Get().GetMode() == CamMode::ThirdPerson;
//     if (!look) return;
//     Vector2 mouseDelta = GetMouseDelta();

//     player.rotation.y -= mouseDelta.x * GameSettings::mouseSensitivity;
//     player.rotation.x -= mouseDelta.y * GameSettings::mouseSensitivity;
//     player.rotation.x = Clamp(player.rotation.x, -30.0f, 30.0f);

//     float yaw = DEG2RAD * player.rotation.y;
//     player.forward = Vector3Normalize({ sinf(yaw), 0.0f, cosf(yaw) });
// }


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





