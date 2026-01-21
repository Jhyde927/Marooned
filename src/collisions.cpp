#include "raylib.h"
#include "bullet.h"
#include "world.h"
#include "dungeonGeneration.h"
#include "sound_manager.h"
#include "resourceManager.h"
#include "raymath.h"
#include "pathfinding.h"
#include "spiderEgg.h"
#include "collisions.h"
#include "miniMap.h"
#include "lighting.h"




bool CheckCollisionPointBox(Vector3 point, BoundingBox box) {
    return (
        point.x >= box.min.x && point.x <= box.max.x &&
        point.y >= box.min.y && point.y <= box.max.y &&
        point.z >= box.min.z && point.z <= box.max.z
    );
}

// Returns true if we resolved a collision (i.e., boxes overlapped)
bool ResolveBoxBoxCollisionXZ(const BoundingBox& enemyBox, const BoundingBox& wallBox, Vector3& enemyPos)
{
    // Check overlap first
    if (!CheckCollisionBoxes(enemyBox, wallBox))
        return false;

    // Compute centers
    Vector3 eCenter = {
        (enemyBox.min.x + enemyBox.max.x) * 0.5f,
        (enemyBox.min.y + enemyBox.max.y) * 0.5f,
        (enemyBox.min.z + enemyBox.max.z) * 0.5f
    };
    Vector3 wCenter = {
        (wallBox.min.x + wallBox.max.x) * 0.5f,
        (wallBox.min.y + wallBox.max.y) * 0.5f,
        (wallBox.min.z + wallBox.max.z) * 0.5f
    };

    // Half-sizes
    float eHalfX = (enemyBox.max.x - enemyBox.min.x) * 0.5f;
    float eHalfZ = (enemyBox.max.z - enemyBox.min.z) * 0.5f;

    float wHalfX = (wallBox.max.x - wallBox.min.x) * 0.5f;
    float wHalfZ = (wallBox.max.z - wallBox.min.z) * 0.5f;

    // Delta between centers
    float dx = eCenter.x - wCenter.x;
    float dz = eCenter.z - wCenter.z;

    // Penetration depths on X and Z
    float penX = (eHalfX + wHalfX) - fabsf(dx);
    float penZ = (eHalfZ + wHalfZ) - fabsf(dz);

    // Push out along the smaller penetration axis
    if (penX < penZ)
    {
        float signX = (dx < 0) ? -1.0f : 1.0f;
        enemyPos.x += signX * penX;
    }
    else
    {
        float signZ = (dz < 0) ? -1.0f : 1.0f;
        enemyPos.z += signZ * penZ;
    }

    return true;
}


void ResolveBoxSphereCollision(const BoundingBox& box, Vector3& position, float radius) {
    // Clamp player position to the inside of the box
    float closestX = Clamp(position.x, box.min.x, box.max.x);
    float closestY = Clamp(position.y, box.min.y, box.max.y);
    float closestZ = Clamp(position.z, box.min.z, box.max.z);

    Vector3 closestPoint = { closestX, closestY, closestZ };
    Vector3 pushDir = Vector3Subtract(position, closestPoint);
    float distance = Vector3Length(pushDir);

    if (distance == 0.0f) {
        // If player is exactly on the box surface, push arbitrarily
        pushDir = {1.0f, 0.0f, 0.0f};
        distance = 0.001f;
    }

    float overlap = radius - distance;
    if (overlap > 0.0f) {
        Vector3 correction = Vector3Scale(Vector3Normalize(pushDir), overlap);
        position = Vector3Add(position, correction);
    }
}

Vector3 ComputeOverlapVector(BoundingBox a, BoundingBox b) {
    float xOverlap = fmin(a.max.x, b.max.x) - fmax(a.min.x, b.min.x);
    float yOverlap = fmin(a.max.y, b.max.y) - fmax(a.min.y, b.min.y);
    float zOverlap = fmin(a.max.z, b.max.z) - fmax(a.min.z, b.min.z);

    if (xOverlap <= 0 || yOverlap <= 0 || zOverlap <= 0) {
        return {0, 0, 0}; // No collision
    }

    float minOverlap = xOverlap;
    Vector3 axis = {1, 0, 0};
    if (yOverlap < minOverlap) {
        minOverlap = yOverlap;
        axis = {0, 1, 0};
    }
    if (zOverlap < minOverlap) {
        minOverlap = zOverlap;
        axis = {0, 0, 1};
    }

    // Get direction from a to b to know which way to push
    Vector3 direction = Vector3Normalize(Vector3Subtract(b.min, a.min));
    float sign = (Vector3DotProduct(axis, direction) >= 0) ? 1.0f : -1.0f;

    return Vector3Scale(axis, minOverlap * sign);
}


void ResolvePlayerEnemyMutualCollision(Character* enemy, Player* player) {
    BoundingBox enemyBox = enemy->GetBoundingBox();
    BoundingBox playerBox = player->GetBoundingBox();

    Vector3 overlap = ComputeOverlapVector(enemyBox, playerBox);
    if (Vector3Length(overlap) > 0) {
        Vector3 correction = Vector3Scale(overlap, 0.5f);
        enemy->position = Vector3Subtract(enemy->position, correction);
        player->position = Vector3Add(player->position, correction);
    }
}

void AgroAllSkeletons(){
    for (Character& e : enemies){
        if (e.type == CharacterType::Skeleton){
            e.playerVisible = true;
            e.canSee = true;
            e.ChangeState(CharacterState::Chase);
        }
    }

}

void AgroAllPirates(){
    for (Character& e : enemies){
        if (e.type == CharacterType::Pirate){
            e.playerVisible = true;
            e.canSee = true;
            e.ChangeState(CharacterState::Chase);
        }
    }

}

void AgroAllGiantSpiders(){
    for (Character& e : enemies){
        if (e.type == CharacterType::GiantSpider){
            e.playerVisible = true;
            e.canSee = true;
            e.ChangeState(CharacterState::Chase);
        }
    }

}

void SwitchCollision(){
    for (SwitchTile& st : switches){
        if (!st.triggered){
            if (CheckCollisionBoxSphere(st.box, player.position, player.radius)){
                st.triggered = true;

                if (st.lockType == LockType::Gold){
                    for (Door& door : doors){
                        if (door.requiredKey == KeyType::Gold){
                            door.isLocked = false;
                            door.isOpen = true;
                            walkable[door.tileX][door.tileY] = true;
                            walkableBat[door.tileX][door.tileY] = true;
                            AgroAllSkeletons();   
                        }
                    }
                }else if (st.lockType == LockType::Silver){
                    for (Door& door : doors){
                        if (door.requiredKey == KeyType::Silver){
                            door.isLocked = false;
                            door.isOpen = true;
                            walkable[door.tileX][door.tileY] = true;
                            walkableBat[door.tileX][door.tileY] = true;
                            AgroAllPirates();
                            
                        }
                    }

                }else if (st.lockType == LockType::Skeleton){
                    for (Door& door : doors){
                        if (door.requiredKey == KeyType::Skeleton){
                            door.isLocked = false;
                            door.isOpen = true;
                            walkable[door.tileX][door.tileY] = true;
                            walkableBat[door.tileX][door.tileY] = true;
                            AgroAllGiantSpiders();
  
                            
                        }
                    }

                }

                OpenEventLockedDoors(); //any switch opens all even locked door for now. 
            }
        }
    }
}



void launcherCollision(){
    for (LauncherTrap& launcher : launchers){
        if (CheckCollisionBoxSphere(launcher.bounds, player.position, player.radius)){
            ResolveBoxSphereCollision(launcher.bounds, player.position, player.radius);
        }
    }
}


void SpiderWebCollision(){
    for (SpiderWebInstance& web : spiderWebs){
        if (!web.destroyed && CheckCollisionBoxSphere(web.bounds, player.position, player.radius)){
            ResolveBoxSphereCollision(web.bounds, player.position, player.radius);
        }
    }
}

void SpiderEggCollision(){
    for (SpiderEgg& egg: eggs){
        if (egg.state != SpiderEggState::Destroyed && CheckCollisionBoxSphere(egg.collider, player.position, player.radius)){
            ResolveBoxSphereCollision(egg.collider, player.position, player.radius);
        }
    }
}


void DoorCollision(){
    for (Door& door : doors){//player collision
        if (!door.isOpen && CheckCollisionBoxSphere(door.collider, player.position, player.radius)){
           ResolveBoxSphereCollision(door.collider, player.position, player.radius);

        }

        for (Character* enemy : enemyPtrs){ //enemy collilsion 
            if (!door.isOpen && CheckCollisionBoxSphere(door.collider, enemy->position, enemy->radius)){
                ResolveBoxSphereCollision(door.collider, enemy->position, enemy->radius);
            }
        }
        
        //door side colliders
        for (BoundingBox& side : door.sideColliders){
            if (door.isOpen && CheckCollisionBoxSphere(side, player.position, 100)){
                ResolveBoxSphereCollision(side, player.position, 100);
            }

            for (Character* enemy : enemyPtrs){
                if ((door.isOpen || door.window) && CheckCollisionBoxSphere(side, enemy->position, enemy->radius)){
                    ResolveBoxSphereCollision(side, enemy->position, enemy->radius);
                }
            }
        }

    }
}

void WallCollision(){

    for (int i = 0; i < (int)wallRunColliders.size(); ++i) {
        if (!wallRunColliders[i].enabled) continue;
        // draw wallInstances[i] and use wallRunColliders[i].bounds for collision
        WallRun& run = wallRunColliders[i];
        if (CheckCollisionBoxSphere(run.bounds, player.position, player.radius)) { //player wall collision
            ResolveBoxSphereCollision(run.bounds, player.position, player.radius);
        }
    }

    for (WindowCollider& wc : windowColliders) {
        if (CheckCollisionBoxSphere(wc.bounds, player.position, player.radius)) {
            ResolveBoxSphereCollision(wc.bounds, player.position, player.radius);
        }
    }

    for (InvisibleWall& iw : invisibleWalls){ //not fully implemented. 
        //if (!iw.enabled) continue;

        if (CheckCollisionBoxSphere(iw.tileBounds, player.position, player.radius)) { //player wall collision
            ResolveBoxSphereCollision(iw.tileBounds, player.position, player.radius);
        }

    }


    for (const WallRun& run : wallRunColliders) { 

        for (Character* enemy : enemyPtrs){ //all enemies
            if (CheckCollisionBoxSphere(run.bounds, enemy->position, enemy->radius)){
                ResolveBoxSphereCollision(run.bounds, enemy->position, enemy->radius);
            }
        }



    }
}

void pillarCollision() {
    for (const PillarInstance& pillar : pillars){
        ResolveBoxSphereCollision(pillar.bounds, player.position, player.radius);
        for (Character* enemy : enemyPtrs){
            ResolveBoxSphereCollision(pillar.bounds, enemy->position, enemy->radius);
        }
    }


}

void barrelCollision(){
    
    for (const BarrelInstance& barrel : barrelInstances) {
        if (!barrel.destroyed){ //walk through broke barrels
            ResolveBoxSphereCollision(barrel.bounds, player.position, player.radius);
            for (Character* enemy : enemyPtrs){
                ResolveBoxSphereCollision(barrel.bounds, enemy->position, enemy->radius);
            }
        }
        
    }

}

void ChestCollision(){
    for (const ChestInstance& chest : chestInstances){
        ResolveBoxSphereCollision(chest.bounds, player.position, player.radius);
        for (Character* enemy : enemyPtrs){
            ResolveBoxSphereCollision(chest.bounds, enemy->position, enemy->radius);
        }
    }
}

void HandleEnemyPlayerCollision(Player* player) {
    for (Character* enemy : enemyPtrs) {
        if (enemy->isDead) continue;
        if (CheckCollisionBoxes(enemy->GetBoundingBox(), player->GetBoundingBox())) {
            ResolvePlayerEnemyMutualCollision(enemy, player);
        }
    }
}

void EnemyWallCollision(){

    //NO enemy/wall collision, just makes them get stuck sometimes. Enemy natually can't move through walls because of pathfinding. 
    // for (Character* enemy : enemyPtrs) {
    //     if (enemy->isDead) continue;
    //     for (auto& wall : wallRunColliders){
    //         if (CheckCollisionBoxes(enemy->GetBoundingBox(), wall.bounds)){
    //             ResolveBoxSphereCollision(wall.bounds, enemy->position, 30);
    //         }
    //     }
    // }
}


void HandleMeleeHitboxCollision(Camera& camera) {
    //if (player.activeWeapon != WeaponType::Sword  && player.activeWeapon != WeaponType::MagicStaff) return;

    bool swordActive = (player.activeWeapon == WeaponType::Sword && meleeWeapon.hitboxActive);
    bool staffActive = (player.activeWeapon == WeaponType::MagicStaff && magicStaff.hitboxActive);
    if (!swordActive && !staffActive) return;
    //we never check if meleeHitbox is active, this could result in telefragging anything on top of playerpos, enemy bounding boxes prevent this
    for (BarrelInstance& barrel : barrelInstances){
        if (barrel.destroyed) continue;
        int tileX = GetDungeonImageX(barrel.position.x, tileSize, dungeonWidth);
        int tileY = GetDungeonImageY(barrel.position.z, tileSize, dungeonHeight);

        if (CheckCollisionBoxes(barrel.bounds, player.meleeHitbox)){
            PlayerSwipeDecal(camera); //swipe decal on hit. 
            barrel.destroyed = true;
            walkable[tileX][tileY] = true; //tile is now walkable for enemies
            walkableBat[tileX][tileY] = true;
            SoundManager::GetInstance().Play("barrelBreak");
            if (barrel.containsPotion) {
                Vector3 pos = {barrel.position.x, barrel.position.y + 100, barrel.position.z};
                collectables.push_back(Collectable(CollectableType::HealthPotion, pos, R.GetTexture("healthPotTexture"), 40));

            }

            if (barrel.containsGold) {
                Vector3 pos = {barrel.position.x, barrel.position.y + 100, barrel.position.z};
                int gvalue = GetRandomValue(1, 100);
                Collectable gold = Collectable(CollectableType::Gold, pos, R.GetTexture("coinTexture"), 40);
                gold.value = gvalue;
                collectables.push_back(gold);

            }

            if (barrel.containsMana) {
                Vector3 pos = {barrel.position.x, barrel.position.y + 100, barrel.position.z};
                Collectable manaPot = Collectable(CollectableType::ManaPotion, pos, R.GetTexture("manaPotion"), 40);
                collectables.push_back(manaPot);

            }

        }
    }

    for (Character* enemy : enemyPtrs){ //iterate all enemyPtrs
        if (enemy->isDead) continue;


        if (CheckCollisionBoxes(enemy->GetBoundingBox(), player.meleeHitbox) && enemy->lastAttackid != player.attackId){
            if (swordActive || staffActive){
                enemy->lastAttackid = player.attackId; //only apply damage once per swing. player.attackId is incremented every swing
                enemy->TakeDamage(50); //staff and sword both do 50. maybe staff should do less. 
                
                if (enemy->type != CharacterType::Skeleton && enemy->type != CharacterType::Ghost){ //skeles and ghosts dont bleed.  
                    if (enemy->currentHealth <= 0){
                        meleeWeapon.model.materials[3].maps[MATERIAL_MAP_DIFFUSE].texture = R.GetTexture("swordBloody");
                    } 
                }

                if (player.activeWeapon == WeaponType::Sword) SoundManager::GetInstance().Play("swordHit");
                if (player.activeWeapon == WeaponType::MagicStaff) SoundManager::GetInstance().Play("staffHit");
            }

        }
    }

    for (SpiderWebInstance& web : spiderWebs){
        if (!web.destroyed && CheckCollisionBoxes(web.bounds, player.meleeHitbox)){
            web.destroyed = true;
            PlayerSwipeDecal(camera);
            //play a sound
        }
    }

    for (SpiderEgg& egg : eggs){
        if (CheckCollisionBoxes(egg.collider, player.meleeHitbox) && egg.state != SpiderEggState::Destroyed){
            if (egg.lastAttackId != player.attackId){ //each melee attack as a unique id incremented each swing. 
                egg.lastAttackId = player.attackId;// a more robust way a limiting damage once per swing. Consider using this for enemies
                DamageSpiderEgg(egg, 50.0f, player.position);
            } 
        }
    }


}

// Pick nearest face normal of an AABB at the impact point.
// Assumes p is inside or very near the box.
static inline Vector3 AABBHitNormal(const BoundingBox& box, const Vector3& p)
{
    float dxMin = fabsf(p.x - box.min.x);
    float dxMax = fabsf(box.max.x - p.x);
    float dyMin = fabsf(p.y - box.min.y);
    float dyMax = fabsf(box.max.y - p.y);
    float dzMin = fabsf(p.z - box.min.z);
    float dzMax = fabsf(box.max.z - p.z);

    // Start with +X face as best
    float best = dxMin;
    Vector3 n = { -1, 0, 0 };

    if (dxMax < best) { best = dxMax; n = {  1, 0, 0 }; }
    if (dyMin < best) { best = dyMin; n = {  0,-1, 0 }; }
    if (dyMax < best) { best = dyMax; n = {  0, 1, 0 }; }
    if (dzMin < best) { best = dzMin; n = {  0, 0,-1 }; }
    if (dzMax < best) { /*best = dzMax;*/ n = {  0, 0, 1 }; }

    return n; // already unit for axis-aligned faces
}

void BulletRicochetPuff(Bullet& b, Vector3 dir, Color c)
{
    b.fireEmitter.SetParticleSize(6.0f);
    b.fireEmitter.SetColor(c);

    // Scale dir down so puff is subtle
    Vector3 base = Vector3Scale(dir, 0.15f);

    // Random jitter (same vibe you already like)
    float r = 50;
    Vector3 jitter = {
        (float)GetRandomValue(-r, r),
        (float)GetRandomValue(-r, r),
        (float)GetRandomValue(-r, r)
    };

    Vector3 puffVel = Vector3Add(base, jitter);

    b.fireEmitter.SetVelocity(puffVel);
    b.fireEmitter.EmitBurst(b.position, 2, ParticleType::Impact);
}


void BulletParticleRicochetNormal(Bullet& b, Vector3 n, Color c)
{
    b.fireEmitter.SetParticleSize(6.0f);
    b.fireEmitter.SetColor(c);

    n = Vector3Normalize(n);   // just in case

    Vector3 v = b.velocity;

    // Reflect velocity: v' = v - 2*dot(v,n)*n
    float d = Vector3DotProduct(v, n);
    Vector3 reflected = Vector3Subtract(v, Vector3Scale(n, 2.0f * d));

    // Scale down
    Vector3 base = Vector3Scale(reflected, 0.15f);

    // Random jitter
    float r = 50.0f;
    Vector3 jitter = {
        (float)GetRandomValue(-r, r),
        (float)GetRandomValue(-r, r),
        (float)GetRandomValue(-r, r)
    };

    Vector3 smokeVel = Vector3Add(base, jitter);

    b.fireEmitter.SetVelocity(smokeVel);
    //b.fireEmitter.EmitBurst(b.position, 2, ParticleType::Impact);

    b.exploded = true;
    b.alive = false;
}


void BulletParticleBounce(Bullet& b, Color c){
    b.fireEmitter.SetParticleSize(6.0);
    b.fireEmitter.SetColor(c);
    Vector3 base = Vector3Negate(b.velocity);

    // Scale down so it drifts slowly instead of blasting away
    base = Vector3Scale(base, 0.15f);   // 20% of bullet speed 

    // Add randomness
    float r = 50.0f; // magnitude of random jitter
    Vector3 jitter = {
        (float)GetRandomValue(-r, r),
        (float)GetRandomValue(-r, r),
        (float)GetRandomValue(-r, r)
    };

    // Final smoke velocity
    Vector3 smokeVel = Vector3Add(base, jitter);

    b.fireEmitter.SetVelocity(smokeVel);
    b.fireEmitter.EmitBurst(b.position, 2, ParticleType::Impact);

    b.exploded = true;
    b.alive = false;
}

bool TryBulletRicochet(Bullet& b, Vector3 n, float damp, float minSpeed, float headOnCosThreshold)
{
    //returns true if ricochet was succesful and bullet survived. 

    // Normalize inputs
    n = Vector3Normalize(n);

    Vector3 v = b.velocity;
    float speed = Vector3Length(v);
    if (speed < minSpeed) return false; // too slow to matter

    Vector3 vNorm = Vector3Scale(v, 1.0f / speed);

    // If hit is too head-on, don't ricochet
    float cosAngle = fabsf(Vector3DotProduct(vNorm, n)); // 1=head-on, 0=grazing
    if (cosAngle > headOnCosThreshold){
        return false;
    } 

    // Apply probability: e.g. only 50% of eligible hits actually bounce.
    if (GetRandomValue(1, 100) > 50) {
        return false; // 50% just die
    }


    // Reflect velocity: v' = v - 2*dot(v,n)*n
    float d = Vector3DotProduct(v, n);
    Vector3 reflected = Vector3Subtract(v, Vector3Scale(n, 2.0f * d));

    // Damp energy
    reflected = Vector3Scale(reflected, damp);

    // Apply new velocity
    b.velocity = reflected;

    // Push bullet out of the wall a bit to avoid immediate re-collision
    b.position = Vector3Add(b.position, Vector3Scale(n, 2.0f));

    return true;
}

void CheckBulletHits(Camera& camera) {
    
    for (Bullet& b : activeBullets) {
        if (!b.IsAlive()) continue;

        Vector3 pos = b.GetPosition();

        // 🔹 1. Hit player
        if (CheckCollisionBoxSphere(player.GetBoundingBox(), b.GetPosition(), b.GetRadius())) { //use CollisionBoxSphere and use bullet radius
            if (b.type == BulletType::Fireball){ //this means wizards are immune to other wizard fireballs. or launchers
                b.Explode(camera);
                //damage delt elseware
                continue;
            }

            if (b.type == BulletType::Iceball){

                if (player.state != PlayerState::Frozen && player.canFreeze){
                    player.state = PlayerState::Frozen;
                    player.canFreeze = false;
                    b.Explode(camera);
                    b.alive = false; //kill the bullet
                    player.freezeTimer = 1.5f;

                }

                continue;
            }

            if (b.IsEnemy()) {

                b.BulletHole(camera); //show bullet whole decal infront of player. 
                player.TakeDamage(25);
                continue;
            }
        }

        // 🔹 2. Hit enemy
        for (Character* enemy : enemyPtrs) {
            if (enemy->isDead) continue;
            bool isSkeleton = (enemy->type == CharacterType::Skeleton);
            bool isGhost = (enemy->type == CharacterType::Ghost);
            if (CheckCollisionBoxSphere(enemy->GetBoundingBox(), b.GetPosition(), b.GetRadius())) {
                if (!b.IsEnemy() && (b.type == BulletType::Default)) {


                    if (b.hermit){
                        enemy->TakeDamage(999);
                        b.alive = false;
                        b.exploded = true;
                        break;
                    }

                    int extraD = 0;
                    if (enemy->state == CharacterState::Harpooned) extraD = 20;//enemies take more damage when harpooned. 
                    enemy->TakeDamage((int)b.ComputeDamage() + extraD); //damage based on bullets velocity, base 10 damage for blunderbuss
                    BoundingBox box = enemy->GetBoundingBox(); //we leave an aweful lot up to chance, but it plays well. 
                    Vector3 n = AABBHitNormal(box, b.position);
                    TryBulletRicochet(b, n, 0.6f, 500, 0.99); //0.9 cosign threshhold makes headon bullets get absorbed by enemy. 
                    break;

                }else if (b.type == BulletType::Bolt){
                    if (b.id != enemy->lastBulletIDHit){
 
                        enemy->TakeDamage(75);
                        enemy->lastBulletIDHit = b.id;
                        break;
                        //penetration, bullet stays alive for now. 

                    }

                }else if (b.type == BulletType::Harpoon){
                    if (b.id != enemy->lastBulletIDHit) {
                        
                        enemy->TakeDamage(75);
                        enemy->lastBulletIDHit = b.id;  //only hook one enemy at a time

                        // Stick this harpoon to the enemy
                        b.stuck = true;
                        b.stuckEnemyId = enemy->id;   
                        b.stuckOffset = Vector3Subtract(b.position, enemy->position);

                        // Stop the bullet moving
                        b.velocity = {0,0,0};
                        b.age = 0.0f;
                        b.maxLifetime = 9999.0f;

                        //GRAPPLE TO ENEMY
                        // player.state = PlayerState::Grappling;
                        // player.grappleTarget = enemy->position;
                        // player.grappleSpeed = 2000.0f;          // or gp.pullSpeed
                        // player.grappleStopDist = 200.0f;        // or gp.stopDistance
                        // player.grappleBulletId = b.id;          // optional, for rope rendering/cleanup
                        // player.harpoonLifeTimer = 3.0f; //start life timer to prevent grappling to an area you can't reach and getting stuck in grapple state
                        // SoundManager::GetInstance().Play("ratchet");
                        // enemy->ChangeState(CharacterState::Stagger); 

                        // PULL ENEMY 
                        if (enemy->type != CharacterType::Trex){
                            enemy->harpoonTarget = player.position;
                            enemy->ChangeState(CharacterState::Harpooned);
                            SoundManager::GetInstance().Play("ratchet");
                        }

                        b.alive = false;
                        b.exploded = true;
                        break;
                    }
                }
                
                else if (b.type == BulletType::Fireball){ //dont check if b.isEnemy, all fireballs hit enemies. 
                    if (enemy->type != CharacterType::Wizard){ //wizards are immune to fire balls. That's a rule I just made. 
                        enemy->TakeDamage(25);
                        b.pendingExplosion = true;
                        b.explosionTimer = 0.04f; // short delay //so it blows up inside the enemy not on the top of their head. 
                        // Don't call b.Explode() yet //called in updateFireball
                        break;
                        
                    }



                }else if (b.type == BulletType::Iceball){
 
                    enemy->ChangeState(CharacterState::Freeze);
                    b.pendingExplosion = true;
                    b.explosionTimer = 0.04f;
                    break;

                    
                } else if (b.IsEnemy() && isSkeleton) { // friendly fire vs skeletons
                    enemy->TakeDamage(150); //higher damage for higher chance of death by enemy bullet. 1 sword swipe plus friendly fire = death
                    BulletParticleBounce(b, LIGHTGRAY);
                    break;
                }

            }
        }


        for (SpiderEgg& egg : eggs){ //should you be able to harpoon eggs?
            if (CheckCollisionBoxSphere(egg.collider, b.position, b.radius) && egg.state != SpiderEggState::Destroyed){
                if (b.type == BulletType::Fireball || b.type == BulletType::Iceball){
                    DamageSpiderEgg(egg, 100, player.position);
                    b.Explode(camera);
                    break;
                }else{
                    DamageSpiderEgg(egg, 25, player.position);
                    Vector3 n = AABBHitNormal(egg.collider, b.position);
                    TryBulletRicochet(b, n, 0.6f, 500, 0.9); //0.9 cosign makes headon bullets get absorbed by enemy. 
                    break;

                }

            }
        }

        for (GrapplePoint& gp : grapplePoints){
            if (CheckCollisionBoxSphere(gp.box, b.position, b.radius) && b.age > 0.1f) { 
                //only detect the collision if the age of the bullet is over 0.1 seconds. As to not grapple the gp your standing on.
                if (b.type == BulletType::Harpoon && gp.enabled) {
                    // Stick bullet to grapple point
                    b.stuck = true;
                    b.stuckToGrapple = true;
                    b.stuckWorldPos = gp.position;

                    b.velocity = {0,0,0};
                    b.maxLifetime = 9999.0f;
                    
                    // Trigger player grapple (only if not already grappling)
                    if (player.state != PlayerState::Grappling) { //dont grapple if the hook is too close. 
                        player.state = PlayerState::Grappling;
                        player.grappleTarget = gp.position;
                        player.grappleSpeed = 3500.0f;          // or gp.pullSpeed
                        player.grappleStopDist = 70.0f;        // or gp.stopDistance
                        player.grappleBulletId = b.id;          // optional, for rope rendering/cleanup
                        player.harpoonLifeTimer = 3.0f; //start life timer to prevent grappling to an area you can't reach and getting stuck in grapple state
                        SoundManager::GetInstance().Play("ratchet");
                        
                    }
                    //dont erase bullet, it needs to remain alive for the retraction. otherwise it grapples at a wonky angle. 
                    break;
                }
            }

        }

        for (Collectable& c : collectables)
{
            // Optional: don't harpoon the harpoon pickup itself
            //if (c.type == CollectableType::Harpoon) continue;

            // Prevent repeated hits by the same bullet id
            if (c.lastHarpoonBulletId == b.id) continue;

            if (CheckCollisionBoxSphere(c.hitBox, b.position, b.radius))
            {
                if (b.type == BulletType::Harpoon)
                {
                    c.isHarpooned = true;
                    c.lastHarpoonBulletId = b.id;

                    // Optional: stick the harpoon bullet to the collectable for rope visuals
                    b.lifeTime = 0.0f;
                    b.stuck = false;
                    b.stuckEnemyId = -1; // means "not enemy"
                    b.stuckOffset = Vector3Subtract(b.position, c.position);
                    b.velocity = {0,0,0};
                    b.maxLifetime = 9999.0f;

                    SoundManager::GetInstance().Play("ratchet");
                }
            }
        }


        // 🔹 3. Hit walls
        for (WallRun& w : wallRunColliders) {
            if (CheckCollisionPointBox(pos, w.bounds)) {

                if (b.type == BulletType::Fireball || b.type == BulletType::Iceball) {
                    b.Explode(camera);
                    break;
                }

                // Default bullets: try ricochet
                Vector3 n = AABBHitNormal(w.bounds, b.position);
                TryBulletRicochet(b, n, 0.6f, 80.0f, 0.999f);

                break;
            }
        }

        // 🔹 4. Hit doors
        for (Door& d : doors) {
            if (!d.isOpen && CheckCollisionPointBox(pos, d.collider)) {
                if (b.type == BulletType::Fireball || b.type == BulletType::Iceball){
                    b.Explode(camera);
                    break;
                }else{
                    Vector3 n = AABBHitNormal(d.collider, b.position);
                    //BulletParticleRicochetNormal(b, n, GRAY);
                    TryBulletRicochet(b, n, 0.6f, 80.0f, 0.999f);
                    break;

                }
            }
            //archway side colliders
            for (BoundingBox& side : d.sideColliders){
                if (d.isOpen && CheckCollisionPointBox(pos, side)){
                    if (b.type == BulletType::Fireball || b.type == BulletType::Iceball){
                        b.Explode(camera);
                        break;
                    }else{
                        Vector3 n = AABBHitNormal(side, b.position);
                        TryBulletRicochet(b, n, 0.6f, 80.0f, 1.0f); //always bounce off side colliders to avoid them tunneling through
                        //BulletParticleRicochetNormal(b, n, GRAY);
                        break;

                    }
                }
            }
        }



        // 🔹 6. Hit pillars
        for (PillarInstance& pillar : pillars) {
            if (CheckCollisionPointBox(pos, pillar.bounds)) {
                if (b.type == BulletType::Fireball || b.type == BulletType::Iceball){
                    b.Explode(camera);
                    break;
                }else{
                    Vector3 n = AABBHitNormal(pillar.bounds, b.position);
                    TryBulletRicochet(b, n, 0.6f, 80.0f, 0.999f);
                    break;

                }
            }
        }
        //Hit spiderweb
        for (SpiderWebInstance& web : spiderWebs){
            if (!web.destroyed && CheckCollisionBoxSphere(web.bounds, b.GetPosition(), b.GetRadius())){
                web.destroyed = true;
                if (b.type == BulletType::Fireball || b.type == BulletType::Iceball){
                    b.Explode(camera);
                    break;
                }else{
                    Vector3 n = AABBHitNormal(web.bounds, b.position);
                    TryBulletRicochet(b, n, 0.6f, 80.0f, 0.9f);
                    break;

                }
            }
        }

                            // bullet hits barrel
        if (HandleBarrelHitsForBullet(b, camera)){
            break; //check bullets last
        }

        


    }
}


bool HandleBarrelHitsForBullet(Bullet& b, Camera& camera)
{
    bool hitSomething = false;
    for (BarrelInstance& barrel : barrelInstances)
    {
        if (barrel.destroyed) continue;
        if (CheckCollisionBoxSphere(barrel.bounds, b.GetPosition(), b.GetRadius()))
        {
            hitSomething = true;
            if (b.type == BulletType::Fireball || b.type == BulletType::Iceball)
            {
                b.Explode(camera);
            }
            else
            {
                Vector3 n = AABBHitNormal(barrel.bounds, b.position);
                TryBulletRicochet(b, n, 0.6f, 80.0f, 0.999f);

            }

            // Destroy barrel + drop loot
            barrel.destroyed = true;

            int tileX = GetDungeonImageX(barrel.position.x, tileSize, dungeonWidth);
            int tileY = GetDungeonImageY(barrel.position.z, tileSize, dungeonHeight);

            if (tileX >= 0 && tileX < dungeonWidth &&
                tileY >= 0 && tileY < dungeonHeight)
            {
                walkable[tileX][tileY] = true;
                walkableBat[tileX][tileY] = true;
            }

            SoundManager::GetInstance().Play("barrelBreak");

            Vector3 dropPos{ barrel.position.x, barrel.position.y + 100.0f, barrel.position.z };
            if (barrel.containsPotion)
            {
                collectables.emplace_back(CollectableType::HealthPotion, dropPos, 
                                          R.GetTexture("healthPotTexture"), 40);
            }
            else if (barrel.containsMana)
            {
                collectables.emplace_back(CollectableType::ManaPotion, dropPos, 
                                          R.GetTexture("manaPotion"), 40);
            }
            else if (barrel.containsGold)
            {
                Collectable gold(CollectableType::Gold, dropPos, R.GetTexture("coinTexture"), 40);
                gold.value = GetRandomValue(1, 100);
                collectables.push_back(gold);
            }

            if (!b.alive) 
                return true; // stop processing this bullet
        }
    }

    // No more logic needed — if the bullet is alive, caller continues. 
    return hitSomething; 
}


bool CheckBulletHitsTree(const TreeInstance& tree, const Vector3& bulletPos) {
    Vector3 treeBase = {
        tree.position.x + tree.xOffset,
        tree.position.y + tree.yOffset,
        tree.position.z + tree.zOffset
    };

    // Check vertical overlap
    if (bulletPos.y < treeBase.y || bulletPos.y > treeBase.y + tree.colliderHeight) {
        return false;
    }

    // Check horizontal distance from tree trunk center
    float dx = bulletPos.x - treeBase.x;
    float dz = bulletPos.z - treeBase.z;
    float horizontalDistSq = dx * dx + dz * dz;

    return horizontalDistSq <= tree.colliderRadius * tree.colliderRadius;
}

bool CheckTreeCollision(const TreeInstance& tree, const Vector3& playerPos) {
    //Tree player collision
    Vector3 treeBase = {
        tree.position.x + tree.xOffset,
        tree.position.y + tree.yOffset,
        tree.position.z + tree.zOffset
    };

    float dx = playerPos.x - treeBase.x;
    float dz = playerPos.z - treeBase.z;
    float horizontalDistSq = dx * dx + dz * dz;

    if (horizontalDistSq < tree.colliderRadius * tree.colliderRadius &&
        playerPos.y >= treeBase.y &&
        playerPos.y <= treeBase.y + tree.colliderHeight) {
        return true;
    }

    return false;
}

void ResolveTreeCollision(const TreeInstance& tree, Vector3& playerPos) {
    Vector3 treeBase = {
        tree.position.x + tree.xOffset,
        tree.position.y + tree.yOffset,
        tree.position.z + tree.zOffset
    };

    float dx = playerPos.x - treeBase.x;
    float dz = playerPos.z - treeBase.z;
    float distSq = dx * dx + dz * dz;

    float radius = tree.colliderRadius;
    if (distSq < radius * radius) {
        float dist = sqrtf(distSq);
        float overlap = radius - dist;

        if (dist > 0.01f) {
            dx /= dist;
            dz /= dist;
            playerPos.x += dx * overlap;
            playerPos.z += dz * overlap;
        }
    }
}



void CheckBulletsAgainstTrees(std::vector<TreeInstance>& trees,
                              Camera& camera)
{
    constexpr float CULL_RADIUS = 500.0f;
    const float cullDistSq = CULL_RADIUS * CULL_RADIUS;

    for (TreeInstance& tree : trees) {
        // use the same center the collision routine uses
        Vector3 treeBase = {
            tree.position.x + tree.xOffset,
            tree.position.y + tree.yOffset,
            tree.position.z + tree.zOffset
        };

        

        for (Bullet& bullet : activeBullets) {
            if (!bullet.IsAlive()) continue;

            const Vector3 bp = bullet.GetPosition();

            // cheap early-out around the correct center
            if (Vector3DistanceSqr(treeBase, bp) < cullDistSq) {
                if (CheckBulletHitsTree(tree, bp)) {
                    if (bullet.type == BulletType::Fireball){
                        bullet.Explode(camera);

                    }else{
                        Vector3 n = AABBHitNormal(GetTreeAABB(tree), bullet.position);
                        TryBulletRicochet(bullet, n, 0.6f, 80.0f, 0.9f);
                    } 

                    break; // stop checking this tree for this frame
                }
            }
        }
    }
}



void TreeCollision(Camera& camera){

    for (TreeInstance& tree : trees) {
        if (Vector3DistanceSqr(tree.position, player.position) < 500 * 500) { //check a smaller area not the whole map. 
            if (CheckTreeCollision(tree, player.position)) {
                ResolveTreeCollision(tree, player.position);
            }
        }
    }

    for (Character* enemy : enemyPtrs){
        for (TreeInstance& tree : trees) {
            if (Vector3DistanceSqr(tree.position, enemy->position) < 500*500) {
                if (CheckTreeCollision(tree, enemy->position)) {
                    ResolveTreeCollision(tree, enemy->position);
                    
                }
            }
        }

    }


    CheckBulletsAgainstTrees(trees,  camera);


}



void HandleDoorInteraction(Camera& camera) {
    static bool isWaiting = false;
    static float openTimer = 0.0f;
    static int pendingDoorIndex = -1;

    float deltaTime = GetFrameTime();

    if (!isWaiting && IsKeyPressed(KEY_E) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) {
        for (size_t i = 0; i < doors.size(); ++i) {
            float distanceTo = Vector3Distance(doors[i].position, player.position);
            if (distanceTo < 250) {

                if (doors[i].eventLocked){ //no access until world event triggers OpenEventLockedDoors()
                    SoundManager::GetInstance().Play("lockedDoor");
                    return;
                }

                // If locked and no key, deny access
                if (doors[i].isLocked) {

                    bool hasKey = false;
                    if (doors[i].requiredKey == KeyType::Gold)   hasKey = player.hasGoldKey;
                    if (doors[i].requiredKey == KeyType::Silver) hasKey = player.hasSilverKey;
                    if (player.hasSkeletonKey) hasKey = true; //skeleton key opens all key locked doors

                    if (hasKey) {
                        doors[i].isLocked = false;
                        SoundManager::GetInstance().Play("unlock");
                    } else {
                        SoundManager::GetInstance().Play("lockedDoor");
                        return;
                    }
                }

                // Start door interaction (fade, open, etc)
                isWaiting = true;
                openTimer = 0.0f;
                pendingDoorIndex = i;

                std::string s = doors[i].isOpen ? "doorClose" : "doorOpen";
                SoundManager::GetInstance().Play(s);

                DoorType type = doors[i].doorType;
                if (type == DoorType::GoToNext || type == DoorType::ExitToPrevious) {
                    previousLevelIndex = levelIndex;
                    isWaiting = false;
                    openTimer = 0.0f;

                    pendingLevelIndex = doors[i].linkedLevelIndex;
                    StartFadeOutToLevel(pendingLevelIndex);
                    isLoadingLevel = true; //stops updating characters, prevents crash on level switch.
                    currentGameState = GameState::LoadingLevel; 
                    if (levelIndex == 4) unlockEntrances = true; //unlock dungeon entrances after seeing dungeon3 (index 4)

                }

                break; // done with this door
            }
        }
    }


    if (isWaiting) {
        openTimer += deltaTime;

        if (openTimer >= 0.5f && pendingDoorIndex != -1) {
            doors[pendingDoorIndex].isOpen = !doors[pendingDoorIndex].isOpen;
            doorways[pendingDoorIndex].isOpen = doors[pendingDoorIndex].isOpen;

            // Update walkable grid, open doors are walkable. 
            int tileX = GetDungeonImageX(doors[pendingDoorIndex].position.x, tileSize, dungeonWidth);
            int tileY = GetDungeonImageY(doors[pendingDoorIndex].position.z, tileSize, dungeonHeight);
            if (tileX >= 0 && tileY >= 0 && tileX < (int)walkable.size() && tileY < (int)walkable[0].size()) {
                walkable[tileX][tileY] = doors[pendingDoorIndex].isOpen;
                walkableBat[tileX][tileY] = doors[pendingDoorIndex].isOpen; //open doors for bats as well
                miniMap.RevealAroundPlayer(player.position);

                //Failed attempt at rebaking lights on door open. Runs too slow and causes problems with orange inner light. 
                // auto affected = GetStaticLightIndices(doors[pendingDoorIndex].position);
                // OnDoorToggled_RebakeStaticLights(doors[pendingDoorIndex].position, affected);

                
            }

            // Reset
            isWaiting = false;
            openTimer = 0.0f;
            pendingDoorIndex = -1;
        }
    }
}

void UpdateCollisions(Camera& camera){
    CheckBulletHits(camera); //bullet collision
    TreeCollision(camera); //player and raptor vs tree
    WallCollision();
    EnemyWallCollision();
    DoorCollision();
    SpiderWebCollision();
    barrelCollision();
    ChestCollision();
    HandleEnemyPlayerCollision(&player);
    pillarCollision();
    launcherCollision();
    HandleMeleeHitboxCollision(camera);
    SpiderEggCollision();
    SwitchCollision();
}


