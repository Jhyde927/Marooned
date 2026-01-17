#pragma once
#include "raylib.h"

enum class InputMode {
    KeyboardMouse,
    Gamepad
};

extern InputMode currentInputMode;


//void UpdateInputMode();
void debugControls(Camera& camera, float deltaTime);
void HandleGamepadLook(float deltaTime);
void HandleMouseLook(float deltaTime);

