#include "boat.h"
#include "raymath.h"
#include "resourceManager.h"
#include "world.h"
#include <iostream>
#include "shaderSetup.h"

Boat player_boat{};

void InitBoat(Boat& boat, Vector3 startPos) {
    boat.position = startPos;
    boat.velocity = { 0.0f, 0.0f, 0.0f };
    boat.rotationY = 0.0f;
    boat.speed = 0.0f;
    boat.maxSpeed = 600.0f;
    boat.acceleration = 300.0f;
    boat.turnSpeed = 20.0f; // degrees per second
    boat.playerOnBoard = false;
    boat.beached = false;
    boat.previousBoatPosition = startPos;
    boat.showMessage = true;


    for (const auto& p : levels[levelIndex].overworldProps) {
        if (p.type == PropType::Boat){
            player_boat.position.x = p.position.x;
            player_boat.position.z = p.position.z;
        }
    }
}

void UpdateBoat(Boat& boat, float deltaTime) {
    if (!boat.playerOnBoard) return;


    // Turning
    if (controlPlayer) {
        if (IsKeyDown(KEY_D)) boat.rotationY -= boat.turnSpeed * deltaTime;
        if (IsKeyDown(KEY_A)) boat.rotationY += boat.turnSpeed * deltaTime;

        // Forward/slowdown
        if (IsKeyDown(KEY_W)) {
            boat.speed += boat.acceleration * deltaTime;
        } else if (IsKeyDown(KEY_S)) {
            boat.speed -= boat.acceleration * deltaTime;
        } else {
            boat.speed *= 0.999f; // drag
        }
    }



    boat.speed = Clamp(boat.speed, -boat.maxSpeed, boat.maxSpeed);

    // Compute movement vector
    float radians = DEG2RAD * boat.rotationY;
    Vector3 direction = { sinf(radians), 0.0f, cosf(radians) };
    boat.velocity = Vector3Scale(direction, boat.speed);

    // Predict next position
    Vector3 proposedPosition = Vector3Add(boat.position, Vector3Scale(boat.velocity, deltaTime));

    // Sample height at next position
    float terrainHeight = GetHeightAtWorldPosition(proposedPosition, heightmap, terrainScale);

    if (terrainHeight <= 60.0f) {
        // Water — move the boat
        boat.previousBoatPosition = boat.position;
        boat.position = proposedPosition;
    } else {
        // Land — stop or beach
        boat.speed = 0;
        boat.velocity = {0, 0, 0};
        boat.beached = true;
       
    }
}

static Color GetTimeOfDayGrayTint()
{
    Vector3 skyColor = ShaderSetup::GetCurrentSkyTopFogColor();

    // Convert sky color brightness to grayscale luminance.
    // Assumes skyColor is 0.0f - 1.0f.
    float brightness =
        skyColor.x * 0.299f +
        skyColor.y * 0.587f +
        skyColor.z * 0.114f;

    // Clamp just in case.
    brightness = Clamp(brightness, 0.0f, 1.0f);

    // Tune these.
    // 1.0 = full daytime brightness.
    // 0.35 = darkest night tint.
    float tintStrength = Lerp(0.35f, 1.0f, brightness);

    unsigned char gray = (unsigned char)(tintStrength * 255.0f);

    return { gray, gray, gray, 255 };
}


void DrawBoat(const Boat& boat) {
    float bob = sinf(GetTime() * 2.0f) * 2.0f;
    Vector3 drawPos = boat.position;
    if (!boat.beached) drawPos.y += bob;

    Color boatTint = GetTimeOfDayGrayTint();
    
    DrawModelEx(R.GetModel("boatModel"), drawPos, {0, 1, 0}, boat.rotationY, {1.0f, 1.0f, 1.0f}, boatTint);
}
