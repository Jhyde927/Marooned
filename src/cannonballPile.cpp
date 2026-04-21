#include "cannonballPile.h"
#include "player.h"
#include "raymath.h"
#include "resourceManager.h"
#include "iostream"

// assuming you have a global resource manager like R

void CannonballPile::Init(const Vector3& pos)
{
    position = pos;
}

bool CannonballPile::IsPlayerInRange(const Player& player, float extraRange) const
{
    float range = interactRange + extraRange;
    float rangeSq = range * range;

    Vector3 d = Vector3Subtract(player.position, position);
    d.y = 0.0f;

    return Vector3LengthSqr(d) <= rangeSq;
}

void CannonballPile::Update(Player& player)
{
    if (!IsPlayerInRange(player)) return;

    if (IsKeyPressed(KEY_E))
    {
        if (!player.isCarrying)
        {
            player.SpawnBoxInHand(player, position); //spawn box at piles position, player automatically picks it up on pressing E.
        }
    }
}

void CannonballPile::Draw() const
{

    DrawModel(R.GetModel("cannonBalls"), position, 25.0f, DARKGRAY);

}