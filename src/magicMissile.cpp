#include "magicMissile.h"
#include "raymath.h"
#include "utilities.h"
#include "world.h"
#include "particleSystem.h"
#include "pathfinding.h"
#include "sound_manager.h"


// MagicMissile::MagicMissile(Vector3 startPosition, Vector3 launchDirection, Vector3 targetPoint)
//     : position(startPosition), targetPoint(targetPoint), speed(900.0f)

// magicMissile.cpp — constructor
MagicMissile::MagicMissile(Vector3 startPosition, Vector3 launchDirection,
                           Vector3 targetPoint, bool playWoosh)
    : position(startPosition),
      targetPoint(targetPoint),
      speed(900.0f),
      playWoosh(playWoosh)
{
    InitParameters();

    float scatterAmount = RandomFloat(-0.25, 0.25);
    float scatterY = RandomFloat(-0.1f, 0.2f);
    Vector3 scatter = {
        scatterAmount,
        scatterY,
        scatterAmount
    };

    travelDirection = Vector3Normalize(
        Vector3Add(launchDirection, scatter)
    );
}

void MagicMissile::InitParameters()
{
    //tweek these
    speed = 1600.0f;
    lifetime = 2.0f;
    age = 0.0f;

    burstTime = 0.08f;
    turnSpeed = 10.0f;
    arrivalRadius = 30.0f;
    collisionRadius = 5.0f;
    damage = 20.0f;

    active = true;
    targetAcquired = false;
    retargetTimer = 0.0f;
    retargetInterval = 0.01f;

    corkscrewPhase = RandomFloat(0, 6.28);//GetRandomValue(0, 628) / 100.0f;
    corkscrewRadius = 20.0f;
    corkscrewAngularSpeed = 10.0f;
}

void MagicMissile::DestroyOnImpact(Vector3 impactPosition)
{
    particleSystem.EmitBurst(
        impactPosition,
        25,
        ParticleType::MagicBurst
    );


    SoundManager::GetInstance().StartPositionalSound( "missileHit",impactPosition,player.position,2000.0f);
    active = false;
}

bool MagicMissile::HandleEnemyCollision(){
    if (!active) return false;

    for (Character* enemy : enemyPtrs){
        if (!enemy) continue;
        if (enemy->isDead) continue;


        if (CheckCollisionBoxSphere(enemy->GetBoundingBox(), position, collisionRadius)){
            particleSystem.EmitBurst(position, 100, ParticleType::MagicBurst);
            enemy->TakeDamage(damage);
            DestroyOnImpact(position);
            return true;
        }
    }

    return false;
}

bool MagicMissile::HandleWorldCollision(
    Vector3 previousPosition,
    Vector3 nextPosition
)
{

    int curTileX = -1;
    int curTileY = -1;
    float killFloorY = 0.0f;
    // Walls
    if (isDungeon &&
        !DDAHasLineOfSightWorld(previousPosition, nextPosition))
    {
        DestroyOnImpact(previousPosition);
        return true;
    }

    if (isDungeon)
    {
        // Ceiling
        if (drawCeiling &&
            previousPosition.y < ceilingHeight &&
            nextPosition.y >= ceilingHeight)
        {
            Vector3 impactPosition = nextPosition;
            impactPosition.y = ceilingHeight;

            DestroyOnImpact(impactPosition);
            return true;
        }

        int tx = GetDungeonImageX(
            nextPosition.x,
            tileSize,
            dungeonWidth
        );

        int ty = GetDungeonImageY(
            nextPosition.z,
            tileSize,
            dungeonWidth
        );



        if (tx != curTileX || ty != curTileY)
        {
            curTileX = tx;
            curTileY = ty;

            bool tileIsLava = (lavaMask[Idx(tx, ty)] == 1);
            bool tileIsVoid = (voidMask[Idx(tx, ty)] == 1);

            if (tileIsVoid)
            {
                killFloorY = -1000.0f;
            }
            else if (tileIsLava)
            {
                killFloorY = floorHeight - lavaOffsetY;
            }
            else
            {
                killFloorY = floorHeight + 20.0f;
            }
        }

        // Crossed the dungeon floor this frame.
        if (previousPosition.y > killFloorY &&
            nextPosition.y <= killFloorY)
        {
            Vector3 impactPosition = nextPosition;
            impactPosition.y = killFloorY;

            DestroyOnImpact(impactPosition);
            return true;
        }
    }
    else
    {
        // Overworld terrain
        float terrainHeight = GetHeightAtWorldPosition(
            nextPosition,
            heightmap,
            terrainScale
        );

        if (previousPosition.y > terrainHeight &&
            nextPosition.y <= terrainHeight)
        {
            Vector3 impactPosition = nextPosition;
            impactPosition.y = terrainHeight;

            DestroyOnImpact(impactPosition);
            return true;
        }
    }

    return false;
}

void MagicMissile::Update(float deltaTime)
{
    if (!active) return;

    age += deltaTime;

    if (playWoosh && !wooshStarted)
    {
        wooshId = SoundManager::GetInstance().StartPositionalSound(
            "missileBlast", position, player.position, 2000.0f);
        wooshStarted = true;
    }

    // if (!wooshStarted)
    // {
    //     SoundManager::GetInstance().PlaySoundAtPosition("missileBlast", position, player.position, 0.0f, 2000.0f);
    //     wooshStarted = true;
    // }

    // Begin homing after the initial outward burst.
    if (age >= burstTime)
    {

        retargetTimer -= deltaTime;

        if (retargetTimer <= 0.0f)
        {
            //AcquireTarget();
            retargetTimer = retargetInterval;
        }

        Vector3 toTarget = Vector3Subtract(targetPoint, position);

        if (Vector3LengthSqr(toTarget) > 0.001f)
        {
            Vector3 desiredDirection = Vector3Normalize(toTarget);
            float turnAmount = Clamp(
                turnSpeed * deltaTime,
                0.0f,
                1.0f
            );

            travelDirection = Vector3Normalize(
                Vector3Lerp(
                    travelDirection,
                    desiredDirection,
                    turnAmount
                )
            );
        }
    }

    corkscrewPhase += corkscrewAngularSpeed * deltaTime;

    Vector3 worldUp = { 0.0f, 1.0f, 0.0f };

    Vector3 right = Vector3CrossProduct(
        worldUp,
        travelDirection
    );

    // Protect against a nearly vertical travel direction.
    if (Vector3LengthSqr(right) < 0.001f)
    {
        right = { 1.0f, 0.0f, 0.0f };
    }
    else
    {
        right = Vector3Normalize(right);
    }

    Vector3 localUp = Vector3Normalize(
        Vector3CrossProduct(travelDirection, right)
    );

    // Tangential velocity around the central travel direction.
    Vector3 corkscrewVelocity = Vector3Add(
        Vector3Scale(
            right,
            -sinf(corkscrewPhase)
        ),
        Vector3Scale(
            localUp,
            cosf(corkscrewPhase)
        )
    );

    corkscrewVelocity = Vector3Scale(
        corkscrewVelocity,
        corkscrewRadius * corkscrewAngularSpeed
    );

    Vector3 forwardVelocity = Vector3Scale(
        travelDirection,
        speed
    );

    Vector3 movementVelocity = Vector3Add(
        forwardVelocity,
        corkscrewVelocity
    );

    Vector3 nextPosition = Vector3Add(
        position,
        Vector3Scale(movementVelocity, deltaTime)
    );

    // Calculate this frame's movement after steering.
    // Vector3 nextPosition = Vector3Add(
    //     position,
    //     Vector3Scale(travelDirection, speed * deltaTime)
    // );

    if (HandleEnemyCollision())
    {
        return;
    }

    if (HandleWorldCollision(position, nextPosition))
    {
        return;
    }

    // Check the entire movement segment before moving.
    if (!DDAHasLineOfSightWorld(position, nextPosition))
    {
        particleSystem.EmitBurst(
            position,
            25,
            ParticleType::MagicBurst
        );

        active = false;
        return;
    }

    // Commit the movement once.
    position = nextPosition;

    if (wooshId != 0)
    {
        SoundManager::GetInstance().MovePositionalSound(wooshId, position, player.position);
    }

    // Emit the trail at the accepted new position.
    particleSystem.EmitTrail(
        position,
        deltaTime,
        50.0f,
        particleAccumulator,
        ParticleType::Missile
    );

    lifetime -= deltaTime;

    if (lifetime <= 0.0f)
    {
        particleSystem.EmitBurst(
            position,
            25,
            ParticleType::Missile
        );

        active = false;
    }
}