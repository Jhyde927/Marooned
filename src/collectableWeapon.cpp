#include "collectableWeapon.h"
#include "player.h"
#include "world.h"

CollectableWeapon::CollectableWeapon(WeaponType type, Vector3 position, Model model)
    : type(type), position(position), model(model), rotationY(0.0f), isCollected(false) {}

void CollectableWeapon::Update(float deltaTime) {
    rotationY += 45.0f * deltaTime;
    if (rotationY >= 360.0f) rotationY -= 360.0f;
}

void CollectableWeapon::Draw() {
    if (isCollected) return;

    Vector3 drawPos = position;
    drawPos.y += sin(GetTime() * 2.0f) * 2.0f; // Hover effect
    DrawModelEx(model, drawPos, {0, 1, 0}, rotationY, {1, 1, 1}, WHITE);
}

bool CollectableWeapon::CheckPickup(Player& player, float pickupRadius) {
    if (isCollected) return false;
    if (Vector3Distance(player.position, position) < pickupRadius) {
        isCollected = true;
        return true;
    }
    return false;
}

void DrawCollectableWeapons(Player& player, float deltaTime){
    for (CollectableWeapon& cw : worldWeapons){
        cw.Update(deltaTime);
        cw.Draw();

        if (cw.CheckPickup(player) && player.collectedWeapons.size() < 3) {
            player.collectedWeapons.push_back(cw.type);
            if (cw.type == WeaponType::MagicStaff){
                hasStaff = true;
            }else if (cw.type == WeaponType::Crossbow){
                hasCrossbow = true;
            }else if (cw.type == WeaponType::Blunderbuss){
                hasBlunderbuss = true;
            }

            if (player.activeWeapon == WeaponType::None) {
    
                player.activeWeapon = cw.type;
                player.currentWeaponIndex = 0;
                
            }else{
            
                player.activeWeapon = cw.type;
            }
        }

    }
}
