#pragma once

#include "raylib.h"
#include "raymath.h"
#include "weapon.h" // Contains enum class WeaponType
#include "player.h"

struct CollectableWeapon {
    WeaponType type; //weaponType used again for collectable weapons
    Vector3 position;
    Model model;
    float rotationY;
    bool isCollected;

    CollectableWeapon(WeaponType type, Vector3 position, Model model);
    
    void Update(float deltaTime);
    void Draw();
    bool CheckPickup(Player& player, float pickupRadius = 80.0f);
};

void DrawCollectableWeapons();
void UpdateCollectableWeapons(float deltaTime);