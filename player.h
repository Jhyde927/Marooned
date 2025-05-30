#pragma once

#include "raylib.h"

struct Player {
    Vector3 position;
    Vector3 velocity;
    Vector2 rotation;
    Vector3 forward;
    bool running = false;
    float runSpeed = 800.0f; // faster than walk speed
    float walkSpeed = 500.0f; // regular speed

    float gravity = 980.0f;
    float height = 200.0f;
    float jumpStrength = 600; 
    bool grounded = false;
    bool isSwimming = false;
    bool onBoard = false;
    bool disableMovement = false;
    
};

// Initializes the player at a given position
void InitPlayer(Player& player, Vector3 startPosition);

// Updates player movement and physics
void UpdatePlayer(Player& player, float deltaTime, Mesh terrainMesh);

// Draws the player model or debug marker
void DrawPlayer(const Player& player);
void HandleGamepadInput(float deltaTime);
void HandleKeyboardInput(float deltaTime);
void HandleMouseLook(float deltaTime);
void HandleStickLook(float deltaTime);
float GetHeightAtWorldPosition(Vector3 position, Image heightmap, Vector3 terrainScale);
