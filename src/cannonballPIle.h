#pragma once

#include "raylib.h"

struct Player;

class CannonballPile
{
public:
    void Init(const Vector3& pos);
    void Update(Player& player);
    void Draw() const;

    bool IsPlayerInRange(const Player& player, float extraRange = 0.0f) const;

    Vector3 position{ 0.0f, 0.0f, 0.0f };
    float interactRange = 180.0f;
};