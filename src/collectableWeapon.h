#pragma once

#include "raylib.h"
#include "raymath.h"
#include "weapon.h" // Contains enum class WeaponType
#include "player.h"

struct CollectableWeapon {
    WeaponType type; //weaponType used again for collectable weapons
    Vector3 position;
    float rotationY;
    bool isCollected;
    Model model;

    CollectableWeapon(WeaponType type, Vector3 position, Model model);
    
    void Update(float deltaTime);
    void Draw();
    bool CheckPickup(Player& player, float pickupRadius = 80.0f);
};

void DrawCollectableWeapons(Player& player, float deltaTime);
void UpdateCollectableWeapons(float deltaTime);