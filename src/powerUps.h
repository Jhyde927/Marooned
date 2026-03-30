#pragma once

#include "raylib.h"
#include "raymath.h"

class Player;

enum class PowerUpType {
    None,
    QuadDamage,
    Haste,
    OverHealth,
    Invulnerability
};

struct PowerUpPickup {
    PowerUpType type;
    Vector3 position;
    float rotationY = 0.0f;
    bool isCollected = false;

    // billboard for now
    Texture2D texture;

    // optional future 3D model support
    Model model;
    Rectangle source;
    Vector2 size;
    bool useModel = false;

    float scale = 40.0f;
    float bobTimer = 0.0f;
    float spinTimer = 0.0f;
    float scaleX = 1.0f;
    float pickupBlockedTimer = 0.0f;

    PowerUpPickup(PowerUpType type, Vector3 position, Texture2D texture);
    PowerUpPickup(PowerUpType type, Vector3 position, Model model);

    void Update(float deltaTime);
    void Draw(const Camera3D& camera);
    bool CheckPickup(Player& player, float pickupRadius = 80.0f);
};

void DrawPowerUps(Player& player, const Camera3D& camera, float deltaTime);
void UpdatePowerUps(Player& player, float deltaTime);