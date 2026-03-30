#include "character.h"
#include "raylib.h"
#include "raymath.h"
#include "dungeonGeneration.h"
#include "world.h"
#include "pathfinding.h"
#include "sound_manager.h"
#include "resourceManager.h"
#include "utilities.h"
#include "spiderEgg.h"

void Character::UpdateAI(float deltaTime, Player& player) {
    switch (type) {
        case CharacterType::Raptor:
            UpdateRaptorAI(deltaTime, player); //distSq check

            break;

        case CharacterType::Pterodactyl:
            UpdateDactylAI(deltaTime, player); //distsq check
            break;

        case CharacterType::Trex:
            UpdateTrexAI(deltaTime, player); //distsq check
            break;

        case CharacterType::Zombie:
            UpdateZombieAI(deltaTime, player);//distsq check
            break;

        case CharacterType::Ghost: //ghost, spider, skeleton all use the same AI
        case CharacterType::Spider:
        case CharacterType::Skeleton:
            UpdateSkeletonAI(deltaTime, player); //distsq check
            break;

        case CharacterType::Bat:
            UpdateBatAI(deltaTime, player);//distsq check
            break;
            
        case CharacterType::Pirate:
            UpdatePirateAI(deltaTime, player); //distsq check  
            break;

        case CharacterType::Wizard:
            UpdateWizardAI(deltaTime, player); //distsq check 
            break;

        case CharacterType::GiantSpider:
            UpdateGiantSpiderAI(deltaTime, player);//distsq check 
            break;



    }
}

static bool IsValidTarget(const Character* t)
{
    return t && !t->isDead; 
}


static Character* FindClosestOfType(
    const Vector3& from,
    const std::vector<Character*>& enemies,
    CharacterType wantType,
    const Character* self,
    float maxDist)
{
    Character* best = nullptr;
    float bestSqr = maxDist * maxDist;

    for (Character* c : enemies)
    {
        if (!c || c == self) continue;
        if (c->isDead) continue;
        if (c->type != wantType) continue;

        float dSqr = Vector3DistanceSqr(from, c->position);
        if (dSqr < bestSqr)
        {
            bestSqr = dSqr;
            best = c;
        }
    }

    return best;
}

void Character::UpdateTargeting(float dt, Player& player, const std::vector<Character*>& enemyPtrs)
{

    if (!IsValidTarget(target)) target = nullptr;
    targetRefreshTimer = std::max(0.0f, targetRefreshTimer - dt);
    if (targetRefreshTimer > 0.0f) return;


    targetRefreshTimer = 0.25f; // refresh 4x/sec; cheap but responsive

    // Default: no target
    target = nullptr;

    if (type == CharacterType::Pirate)
    {
        // Pirates pick closest zombie within some radius
        target = FindClosestOfType(position, enemyPtrs, CharacterType::Zombie, this, 6000.0f);
    }

    if (type == CharacterType::Zombie)
    {
        target = FindClosestOfType(position, enemyPtrs, CharacterType::Pirate, this, 6000.0f);
    }

    // If you want pirates to still prefer player when close, do:
    // (optional) <-needed
    float playerSqr = Vector3DistanceSqr(position, player.position);
    if (playerSqr < 3500.0f * 3500.0f && HasWorldLineOfSight(position, player.position, 0.1f)) target = nullptr; // meaning "use player"
    
    if (target)
    {
        targetDist = Vector3DistanceSqr(position, target->position);
        // Use your existing LOS checker; whichever one you use for pirates now
        targetCanSee = HasWorldLineOfSight(position, target->position, 0.1);//WorldLineOfSight(position, target->position); // placeholder
    }
    else
    {
        targetDist = 999999.0f;
        targetCanSee = false;
    }
}


void Character::UpdateChaseSound(float deltaTime, Player& player){
    if (state != CharacterState::Chase) return;
    chaseSoundTimer -= deltaTime;

    if (chaseSoundTimer <= 0){
        chaseSoundTimer = RandomFloat(3.5, 8.5);
        const char* sn = nullptr;

        switch (type)
        {
            case CharacterType::Skeleton:
                sn = (GetRandomValue(0,1) > 0) ? "skeletonGrunt" : "skeletonGrunt2";
                break;

            case CharacterType::Pirate:
                sn = (GetRandomValue(0,1) > 0) ? "pirateYell1" : "pirateYell2";
                break;

            case CharacterType::Zombie:
                sn = (GetRandomValue(0,1) > 0) ? "zombieMoan1" : "zombieMoan2";
                break;

            default:
                break;
        }

        if (sn)
        {
            SoundManager::GetInstance().PlaySoundAtPosition(sn, position, player.position, 0.0f, 3000);
        }
    }
}

void Character::UpdatePlayerVisibility(const Vector3& playerPos, float deltaTime, float epsilon) {
    if (player.godMode){
        canSee = false;
        playerVisible = false; 
        return;
    }


    canSee = HasWorldLineOfSight(position, playerPos, epsilon);
    if (canSee) {
        lastKnownPlayerPos = playerPos;
        playerVisible = true;
        timeSinceLastSeen = 0.0f;
    } else {
        timeSinceLastSeen += deltaTime;
        if (timeSinceLastSeen > forgetTime) {
            playerVisible = false;
        }
    }
}




void Character::UpdateGiantSpiderAI(float deltaTime, Player& player) {
    float distanceSq = Vector3DistanceSqr(position, player.position);
    Vector2 start = WorldToImageCoords(position);
    float visionEnter = 3000.0f * 3000.0f;
    float attackEnter = 200.0f * 200.0f;
    UpdatePlayerVisibility(player.position, deltaTime, 0.0f);
    UpdateLeavingFlag(player.position, player.previousPosition);

    if (currentHealth <= 0.0f){
        state = CharacterState::Death;
    }

    if (!spiderAgro){
        spiderAgroTimer += deltaTime;
        if (spiderAgroTimer >= 10.0f){ //dont runaway forever
            spiderAgro = true;
            canLayEgg = true; // too much? 
            spiderAgroTimer = 0.0f;
            ChangeState(CharacterState::Chase); //doesn't need to see player to chase, allows him to sneak up on you. 
        }
    }

    switch (state){
        case CharacterState::Idle: {
            stateTimer += deltaTime;
 
            Vector2 start = WorldToImageCoords(position);

            // Transition to chase if player detected
            if (distanceSq < visionEnter && stateTimer > 1.0f && playerVisible) {

                if (type == CharacterType::GiantSpider && levelIndex == 8){ //lock the door when spider sees player
                    if (doors[5].isOpen) doors[5].isOpen = false;
                    if (!doors[5].eventLocked) doors[5].eventLocked = true; //event lock the door behind the player until the spider dies. 
                }

                if (!spiderAgro){
                    if (TrySetRetreatPath(start, WorldToImageCoords(player.position), this, currentWorldPath, 12, 3, 100, 30, 3)){
                        
                        ChangeState(CharacterState::RunAway);
                        break;

                    }
                }

                AlertNearbySkeletons(position, 3000.0f);
                ChangeState(CharacterState::Chase);
                currentWorldPath.clear();

                SetPath(start);


            }

            //dont wander

            break;
        }

        case CharacterState::Chase: {

            stateTimer += deltaTime;
            pathCooldownTimer = std::max(0.0f, pathCooldownTimer - deltaTime);


            if (isLeaving && rowIndex != 3){
                SetAnimation(3, 4, 0.25, true);
            }else if (!isLeaving && rowIndex != 1){
                SetAnimation(1, 5, 0.2, true);
            }

            if (!spiderAgro){
                if (TrySetRetreatPath(start, WorldToImageCoords(player.position), this, currentWorldPath, 12, 3, 100, 25, 3)){
                    ChangeState(CharacterState::RunAway);
                    break;

                }
            }
            if (distanceSq < attackEnter && canSee) {
                ChangeState(CharacterState::Attack);
                break;

            }
            else if (distanceSq > visionEnter) {
                ChangeState(CharacterState::Idle);
                break;

            }
            else {
                const Vector2 curTile = WorldToImageCoords(player.position);
                if (((int)curTile.x != (int)lastPlayerTile.x || (int)curTile.y != (int)lastPlayerTile.y)
                    && pathCooldownTimer <= 0.0f)
                {
                    lastPlayerTile = curTile;
                    pathCooldownTimer = 0.4f; // don’t spam BFS
                    const Vector2 start = WorldToImageCoords(position);
                    SetPath(start); 
                }

                // Move along current path
                MoveAlongPath(currentWorldPath, position, rotationY, skeleSpeed, deltaTime, 100.0f);
            }

        } break;

        case CharacterState::Attack: {
            

            if (distanceSq > (350.0f  * 350.0f)) { 
                if (spiderAgro){
                     ChangeState(CharacterState::Chase);
                     break;
                }

            }

            //dont stand on the same tile as another skele when attacking
            Vector2 myTile = WorldToImageCoords(position);
            Character* occupier = GetTileOccupier(myTile.x, myTile.y, enemyPtrs, this);

            if (occupier && occupier != this) {
                // Only the one with the "greater" pointer backs off
                if (this > occupier) {
                    ChangeState(CharacterState::Reposition);
                    break;
                } else {
                    // Let the other one reposition — wait
                    break;
                }
            }

            float horizDist = DistXZ(position, player.position);
            attackCooldown -= deltaTime;
            if (attackCooldown <= 0.0f && currentFrame == 1 && playerVisible && horizDist < 200.0f) { // make sure you can see what your attacking. 
                attackCooldown = 0.8f; // 0.2 * 4 frames on animation for skele attack. 

                // Play attack sound

                    // Blocked!
                if (CheckCollisionBoxes(GetBoundingBox(), player.blockHitbox) && player.blocking) {
                    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                    decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("blockSheet"), 4, 0.4f, 0.1f, 50.0f);
                    SoundManager::GetInstance().Play(rand() % 2 ? "swordBlock" : "swordBlock2");

      
                } else  {
                    // Player takes damage
                    player.TakeDamage(10);
                    
                    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                    SoundManager::GetInstance().Play(rand() % 2 ? "giantSpiderBite" : "giantSpiderBite");
                    decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("biteSheet"), 4, 0.4f, 0.1f, 50.0f);

                }
                
            }

            if (!spiderAgro){
                ChangeState(CharacterState::RunAway);
            }
            break;
        }


        case CharacterState::RunAway: {
            stateTimer += deltaTime;

            if (isLeaving && rowIndex != 3){
                SetAnimation(3, 4, 0.25, true); //runaway
            }else if (!isLeaving && rowIndex != 1){
                SetAnimation(1, 5, 0.2, true); //walk
            }

            if (distanceSq < attackEnter && canSee && spiderAgro) {
                ChangeState(CharacterState::Attack);
                break;

            }

            if (distanceSq < visionEnter && spiderAgro){
                ChangeState(CharacterState::Chase);
                AlertNearbySkeletons(position, 3000.0f);
                break;

            }

            if (!currentWorldPath.empty()) { 
                Vector3 targetPos = currentWorldPath[0];
                Vector3 dir = Vector3Normalize(Vector3Subtract(targetPos, position));
                Vector3 move = Vector3Scale(dir, 900 * deltaTime); // faster than chase
                position = Vector3Add(position, move);
                rotationY = RAD2DEG * atan2f(dir.x, dir.z);
                position.y = targetPos.y;

                if (Vector3Distance(position, targetPos) < 100.0f) {
                    currentWorldPath.erase(currentWorldPath.begin());
                }
            }
            else {
                if (canLayEgg){
                    canLayEgg = false;
                    SpawnSpiderEgg(position, R.GetTexture("spiderEggSheet"), 200, 200, 3, 0.5f);
                }

                ChangeState(CharacterState::Idle);
            }

            break;


        }

        case CharacterState::Harpooned: {
            stateTimer += deltaTime;

            if (currentHealth <= 0) {
                ChangeState(CharacterState::Death);
                break;
            }

            // Keep updating target so if player backpedals, it still pulls toward you
            Vector3 target = player.position;

            // Pull direction XZ only // try it on raptors on a hill. 
            Vector3 toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;

            float dist = Vector3Length(toTarget);

            // Stop distance so you don't overlap the player
            const float stopDist = harpoonMinDist;

            if (dist > stopDist && dist > 1.0f)
            {
                Vector3 dir = Vector3Scale(toTarget, 1.0f / dist);

                // Pull speed (world units / sec). Tune this.
                const float pullSpeed = 3000.0f;

                float step = pullSpeed * deltaTime;

                // Don't overshoot past stop distance
                float desiredMove = fminf(step, dist - stopDist);

                position = Vector3Add(position, Vector3Scale(dir, desiredMove));

            }

            // after position update
            toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;
            dist = Vector3Length(toTarget);
            
            // End condition: time AND close enough
            bool timeDone = (stateTimer >= harpoonDuration);
            bool closeEnough = (dist <= stopDist + 5.0f);

            if (timeDone && closeEnough)
            {
                currentWorldPath.clear();
                ChangeState(CharacterState::Stagger);
                break;
            }

            break;
        }

        case CharacterState::Freeze: {
            stateTimer += deltaTime;
            //do nothing
            if (currentHealth <= 0 && !isDead){ //hopefully prevents invincible skeles. 
                ChangeState(CharacterState::Death);
                break;
            }

            if (stateTimer > 5.0f && !isDead){
                ChangeState(CharacterState::Idle);
                break;
            }
            break;

        }



        case CharacterState::Stagger: {
            stateTimer += deltaTime;
            //do nothing

            if (stateTimer >= 1.0f) {
                canBleed = true;
                ChangeState(CharacterState::Chase);
                
            }
            break;
        }

        case CharacterState::Death: {
            if (!isDead) {
                isDead = true;
                deathTimer = 0.0f;         // Start counting

            }

            deathTimer += deltaTime;
            break;
        }
      
    }

}

void Character::UpdateBatAI(float deltaTime, Player& player){

    float distanceSq = Vector3DistanceSqr(position, player.position);
    float visionEnter = 4000.0f * 4000.0f;
    Vector2 start = WorldToImageCoords(position);

    UpdatePlayerVisibility(player.position, deltaTime, 0.0f);

    stateTimer += deltaTime;
 
    switch (state){
        case CharacterState::Idle: {

            Vector2 start = WorldToImageCoords(position);

            // Transition to chase if player detected
            if (distanceSq < visionEnter && stateTimer > 1.0f && playerVisible) {
                AlertNearbySkeletons(position, 3000.0f);
                ChangeState(CharacterState::Chase);
                currentWorldPath.clear();

                SetPath(start);
            }

            // Wander if idle too long
            else if (stateTimer > 10.0f) {
                Vector2 randomTile = GetRandomReachableTile(start, this);

                if (IsWalkable(randomTile.x, randomTile.y, dungeonImg)) {
                    if (TrySetRandomPatrolPath(start, this, currentWorldPath)) {
                        ChangeState(CharacterState::Patrol);
                        
                    }
                }
            }

            break;
        }

        
        case CharacterState::Chase: {
            float attackDistance = 200.0f * 200.0f;
            pathCooldownTimer = std::max(0.0f, pathCooldownTimer - deltaTime);

            if (bloatBat){
                if (distanceSq < attackDistance && canSee){
                    TakeDamage(999); //trigger death/explosion
                    return; //bat is exploding make sure we don't change state later on.
                
                }
            }
            if (distanceSq < attackDistance && canSee) {
                ChangeState(CharacterState::Attack);

            }
            else if (distanceSq > visionEnter) {
                ChangeState(CharacterState::Idle);

            }
            else {
                const Vector2 curTile = WorldToImageCoords(player.position);
                if (pathCooldownTimer <= 0.0f){
                
                    lastPlayerTile = curTile;
                    pathCooldownTimer = 0.4f; // don’t spam BFS
                    const Vector2 start = WorldToImageCoords(position);
                    SetPath(start); 
                
                }
                Vector3 repel = ComputeRepulsionForce(enemyPtrs, 300, 500); // your existing call
                // Move along current path
                MoveAlongPath(currentWorldPath, position, rotationY, skeleSpeed, deltaTime, 100, repel);
            }
        } break;

        case CharacterState::Attack: {
            //dont stand on the same tile as another skele when attacking
            Vector2 myTile = WorldToImageCoords(position);
            Character* occupier = GetTileOccupier(myTile.x, myTile.y, enemyPtrs, this);

            if (stateTimer > 2.5){
                if (TrySetRandomPatrolPath(start, this, currentWorldPath) && canSee) { //shoot then move to a random tile and shoot again.
                    ChangeState(CharacterState::Patrol);
                }else{
                    ChangeState(CharacterState::Attack);
                }
            }

            if (occupier && occupier != this) {
                // Only the one with the "greater" pointer backs off
                if (this > occupier) {
                    ChangeState(CharacterState::Reposition);
                    break;
                } else {
                    // Let the other one reposition — wait
                    break;
                }
            }

            if (distanceSq > (210.0f * 210.0f)) { 
                ChangeState(CharacterState::Chase);

            }

            float horizDist = DistXZ(position, player.position); //this should probably be vec3 distance for bats

            attackCooldown -= deltaTime;
            if (attackCooldown <= 0.0f && currentFrame == 1 && playerVisible && horizDist < 200.0f) { // make sure you can see what your attacking. 
                attackCooldown = 0.8f; // 0.2 * 4 frames on animation for skele attack. 

                // Play attack sound
                SoundManager::GetInstance().Play(rand() % 2 ? "swipe2" : "swipe3"); //sword swipes for bats for now

                    // Blocked!
                if (CheckCollisionBoxes(GetBoundingBox(), player.blockHitbox) && player.blocking) {
                    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                    decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("blockSheet"), 4, 0.4f, 0.1f, 50.0f);
                    SoundManager::GetInstance().Play(rand() % 2 ? "swordBlock" : "swordBlock2");

      
                } else  {
                    // Player takes damage
                    player.TakeDamage(10);
                    
                    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                    //SoundManager::GetInstance().Play(rand() % 2 ? "spiderBite2" : "spiderBite1");
                    decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("biteSheet"), 4, 0.4f, 0.1f, 50.0f);

                }
                
            }
            break;
        }
        case CharacterState::Reposition: {
            //surround the player


            Vector2 playerTile = WorldToImageCoords(player.position);
            Vector3 target = position; // fallback

            bool foundSpot = FindRepositionTarget(player, position, target);

            if (foundSpot) {
                Vector3 dir = Vector3Normalize(Vector3Subtract(target, position));
                Vector3 move = Vector3Scale(dir, skeleSpeed * deltaTime);
                position = Vector3Add(position, move);

                rotationY = RAD2DEG * atan2f(dir.x, dir.z);
                position.y = target.y;

             

                if (distanceSq < (300.0f * 300.0f) && stateTimer > 1.0f) {
                    ChangeState(CharacterState::Attack);
                } else if (distanceSq > (300.0f * 300.0f) && stateTimer > 1.0f) {
                    ChangeState(CharacterState::Chase);
                }
            }

            break;
        }



        case CharacterState::Patrol: {

            if (!currentWorldPath.empty()) { 
                Vector3 targetPos = currentWorldPath[0];
                Vector3 dir = Vector3Normalize(Vector3Subtract(targetPos, position));
                Vector3 move = Vector3Scale(dir, 500 * deltaTime); // slower than chase
                position = Vector3Add(position, move);
                rotationY = RAD2DEG * atan2f(dir.x, dir.z);
                position.y = targetPos.y;

                if (Vector3Distance(position, targetPos) < 100.0f) {
                    currentWorldPath.erase(currentWorldPath.begin());
                }
            }
            else {
                ChangeState(CharacterState::Idle);
            }

            break;
        }

        case CharacterState::Freeze: {

            //do nothing
            if (currentHealth <= 0 && !isDead){ //hopefully prevents invincible skeles. 
                ChangeState(CharacterState::Death);
                break;
            }

            if (stateTimer > 5.0f && !isDead){
                ChangeState(CharacterState::Idle);
                break;
            }
            break;

        }

        case CharacterState::Harpooned: {
            if (currentHealth <= 0) {
                ChangeState(CharacterState::Death);
                break;
            }

            // Keep updating target so if player backpedals, it still pulls toward you
            Vector3 target = player.position;

            // Pull direction XZ only // try it on raptors on a hill. 
            Vector3 toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;

            float dist = Vector3Length(toTarget);

            // Stop distance so you don't overlap the player
            const float stopDist = harpoonMinDist;

            if (dist > stopDist && dist > 1.0f)
            {
                Vector3 dir = Vector3Scale(toTarget, 1.0f / dist);

                // Pull speed (world units / sec). Tune this.
                const float pullSpeed = 3000.0f;

                float step = pullSpeed * deltaTime;

                // Don't overshoot past stop distance
                float desiredMove = fminf(step, dist - stopDist);

                position = Vector3Add(position, Vector3Scale(dir, desiredMove));

            }

            // after position update
            toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;
            dist = Vector3Length(toTarget);
            
            // End condition: time AND close enough
            bool timeDone = (stateTimer >= harpoonDuration);
            bool closeEnough = (dist <= stopDist + 5.0f);

            if (timeDone && closeEnough)
            {
                currentWorldPath.clear();
                ChangeState(CharacterState::Stagger);
                break;
            }

            break;
        }



        case CharacterState::Stagger: {
            //do nothing

            if (stateTimer >= 1.0f) {
                canBleed = true; //for spiders
                ChangeState(CharacterState::Chase);
                
            }
            break;
        }

        case CharacterState::Death:
            if (!isDead) {
                SetAnimation(4, 3, 0.5f, false); 
                
                isDead = true;
                deathTimer = 0.0f;         // Start counting
            }
            deathTimer += deltaTime;
            if (deathTimer >= 5.0) isDead = true;
            break;
        }
}

void Character::UpdateZombieAI(float deltaTime, Player& player) {
    float distanceSq = Vector3DistanceSqr(position, player.position);
    Vector2 start = WorldToImageCoords(position);

    UpdatePlayerVisibility(player.position, deltaTime, 0.0f);
    UpdateTargeting(deltaTime, player, enemyPtrs);
    if (state != CharacterState::Chase) chaseSoundTimer = 0.0f;

    stateTimer += deltaTime; //add to state timer every frame. 

    const float visionEnter = 4000.0f * 4000.0f;
    const float attackEnter = 200.0f * 200.0f;
 
    switch (state){
        case CharacterState::Idle: {
            Vector2 start = WorldToImageCoords(position);

            // Transition to chase if player detected
            if (distanceSq < visionEnter && stateTimer > 1.0f && playerVisible) {
                AlertNearbySkeletons(position, 3000.0f);
                ChangeState(CharacterState::Chase);
                currentWorldPath.clear();

                SetPath(start);
            }

            // Wander if idle too long
            else if (stateTimer > 10.0f) {
                Vector2 randomTile = GetRandomReachableTile(start, this);

                if (IsWalkable(randomTile.x, randomTile.y, dungeonImg)) {
                    if (TrySetRandomPatrolPath(start, this, currentWorldPath)) {
                        state = CharacterState::Patrol;
                        SetAnimation(1, 4, 0.2f); // walk anim

                    }
                }
            }

            break;
        }

        case CharacterState::Chase:
        {

            pathCooldownTimer = std::max(0.0f, pathCooldownTimer - deltaTime);

            UpdateChaseSound(deltaTime, player);
            // Decide who we're chasing this frame
            Vector3 chasePos    = player.position;
            float   chaseDist   = distanceSq;   // your existing distance-to-player
            bool    chaseCanSee = canSee;     // your existing LOS-to-player
            bool    chasingPirate = false;

            if (type == CharacterType::Zombie && target && !target->isDead)
            {
                chasingPirate = (target->type == CharacterType::Pirate);
                chasePos      = target->position;
                chaseDist     = targetDist;
                chaseCanSee   = targetCanSee;
            }

            // Enter attack when close + LOS
            if (chaseDist < attackEnter && chaseCanSee)
            {
                ChangeState(CharacterState::Attack);
                break;
            }
            else if (chaseDist > visionEnter)
            {
                ChangeState(CharacterState::Idle);
                break;
            }
            else
            {
                if (pathCooldownTimer <= 0.0f)
                {
                    pathCooldownTimer = RandomFloat(0.3, 0.9); //dont all act at the same time. 

                    // Repath to pirate target if we have one, otherwise to player (your old behavior)
                    if (chasingPirate)
                    {
                        SetPathTo(chasePos);
                    }
                    else
                    {
                        const Vector2 start = WorldToImageCoords(position);
                        SetPath(start); // your existing path-to-player BFS behavior
                    }
                }

                Vector3 repel = ComputeRepulsionForce(enemyPtrs, 300, 500);
                float speed = 5.0f;
                MoveAlongPath(currentWorldPath, position, rotationY, skeleSpeed, deltaTime, speed, repel);
            }

        } break;

        case CharacterState::Attack:
        {

            // Pick victim for this attack
            Vector3 victimPos = player.position;
            float   victimDist = distanceSq;
            bool    victimVisible = playerVisible; // for your current “must be visible” gating
            bool    attackingPirate = false;
            Character* victim = nullptr;

            if (type == CharacterType::Zombie && target && !target->isDead && target->type == CharacterType::Pirate)
            {
                attackingPirate = true;
                victim = target;

                victimPos  = target->position;
                victimDist = targetDist;

                // If you track visibility for pirates differently, use targetCanSee here.
                // If not, you can simply use targetCanSee as "visible enough to attack".
                victimVisible = targetCanSee;
            }

            // Don't stack on same tile while attacking (same as your code)
            Vector2 myTile = WorldToImageCoords(position);
            Character* occupier = GetTileOccupier(myTile.x, myTile.y, enemyPtrs, this);
            if (occupier && occupier != this)
            {
                if (this > occupier) { ChangeState(CharacterState::Reposition); break; }
                else { break; }
            }

            // Leave attack if out of range
            if (victimDist > (250.0f * 250.0f))
            {
                ChangeState(CharacterState::Chase);
                break;
            }

            // Melee distance check (you were using XZ for player)
            float horizDist = DistXZ(position, victimPos);

            attackCooldown -= deltaTime;

            if (attackCooldown <= 0.0f && currentFrame == 1 && victimVisible && horizDist < 250.0f)
            {
                attackCooldown = 0.8;

                //SoundManager::GetInstance().Play(rand() % 2 ? "swipe2" : "swipe3");

                if (attackingPirate && victim)
                {
                    SoundManager::GetInstance().PlaySoundAtPosition("zombieStab", position, player.position, 0.0f, 3000.0f);
                    // --- HIT PIRATE ---
                    victim->TakeDamage(50); 

                    // Optional: spawn blood decal at pirate like you do for player
                    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, victimPos));
                    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));
                    decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("biteSheet"), 4, 0.4f, 0.1f, 50.0f);
                }
                else
                {
                    // --- HIT PLAYER (your existing logic) ---
                    if (CheckCollisionBoxes(GetBoundingBox(), player.blockHitbox) && player.blocking)
                    {
                        Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                        Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                        decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("blockSheet"), 4, 0.4f, 0.1f, 50.0f);
                        SoundManager::GetInstance().Play(rand() % 2 ? "swordBlock" : "swordBlock2");
                    }
                    else
                    {
                        player.TakeDamage(10);

                        Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                        Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));
                        decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("biteSheet"), 4, 0.4f, 0.1f, 50.0f);
                    }
                }
            }

            break;
        }

        case CharacterState::Reposition: {
            //surround the player
            Vector2 playerTile = WorldToImageCoords(player.position);
            Vector3 target = position; // fallback

            bool foundSpot = FindRepositionTarget(player, position, target);

            if (foundSpot) {
                Vector3 dir = Vector3Normalize(Vector3Subtract(target, position));
                Vector3 move = Vector3Scale(dir, skeleSpeed * deltaTime);
                position = Vector3Add(position, move);

                rotationY = RAD2DEG * atan2f(dir.x, dir.z);
                position.y = target.y;

               

                if (distanceSq < attackEnter  && stateTimer > 1.0f) {
                    ChangeState(CharacterState::Attack);
                } else if (distanceSq > (250.0f * 250.0f) && stateTimer > 1.0f) {
                    ChangeState(CharacterState::Chase);
                }
            }

            break;
        }



        case CharacterState::Patrol: {

            if (distanceSq < visionEnter && playerVisible){
                ChangeState(CharacterState::Chase);
                AlertNearbySkeletons(position, 3000.0f);

            }

            if (!currentWorldPath.empty()) { 
                Vector3 targetPos = currentWorldPath[0];
                Vector3 dir = Vector3Normalize(Vector3Subtract(targetPos, position));
                Vector3 move = Vector3Scale(dir, 500 * deltaTime); // slower than chase
                position = Vector3Add(position, move);
                rotationY = RAD2DEG * atan2f(dir.x, dir.z);
                position.y = targetPos.y;

                if (Vector3Distance(position, targetPos) < 100.0f) {
                    currentWorldPath.erase(currentWorldPath.begin());
                }
            }
            else {
                ChangeState(CharacterState::Idle);
            }

            break;
        }

        case CharacterState::Freeze: {
            //do nothing
            if (currentHealth <= 0 && !isDead){ 
                ChangeState(CharacterState::Death);
                break;
            }

            if (stateTimer > 5.0f && !isDead){
                ChangeState(CharacterState::Idle);
                break;
            }
            break;

        }

        case CharacterState::Harpooned: {

            if (currentHealth <= 0) {
                ChangeState(CharacterState::Death);
                break;
            }

            // Keep updating target so if player backpedals, it still pulls toward you
            Vector3 target = player.position;

            // Pull direction XZ only // try it on raptors on a hill. 
            Vector3 toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;

            float dist = Vector3Length(toTarget);

            // Stop distance so you don't overlap the player
            const float stopDist = harpoonMinDist;

            if (dist > stopDist && dist > 1.0f)
            {
                Vector3 dir = Vector3Scale(toTarget, 1.0f / dist);

                // Pull speed (world units / sec). Tune this.
                const float pullSpeed = 3000.0f;

                float step = pullSpeed * deltaTime;

                // Don't overshoot past stop distance
                float desiredMove = fminf(step, dist - stopDist);

                position = Vector3Add(position, Vector3Scale(dir, desiredMove));

            }

            // after position update
            toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;
            dist = Vector3Length(toTarget);
            
            // End condition: time AND close enough
            bool timeDone = (stateTimer >= harpoonDuration);
            bool closeEnough = (dist <= stopDist + 5.0f);

            if (timeDone && closeEnough)
            {
                currentWorldPath.clear();
                ChangeState(CharacterState::Stagger); //stun the target after grapple
                break;
            }

            break;
        }



        case CharacterState::Stagger: {
            //do nothing

            if (stateTimer >= 1.0f) {
                canBleed = true; //for spiders
                ChangeState(CharacterState::Chase);
                
            }
            break;
        }
 
        case CharacterState::Death:
            if (!isDead) {
                //SetAnimation(4, 3, 0.5f, false); 
                isDead = true;
                deathTimer = 0.0f;         // Start counting
            }


            if (deathTimer >= 4.5f && canRes && !lostLimb && GetRandomValue(0,1) > 0){ //zombies can get back up. half the time
                canRes = false; //zombies can only ressurect once.
                deathTimer = 0.0;
                isDead = false;
                currentHealth = 50; // one sword swipe will kill ressurected zombies. 
                ChangeState(CharacterState::Idle);

                //A headless zombie can't get back up. It might look cool if they could. Maybe a chance to loose a limb on get up.
            }

            deathTimer += deltaTime;
            break;
        }
       
}



void Character::UpdateSkeletonAI(float deltaTime, Player& player) {
    float distanceSq = Vector3DistanceSqr(position, player.position);
    float visionEnter = 4000.0f * 4000.0f;
    Vector2 start = WorldToImageCoords(position);

    UpdatePlayerVisibility(player.position, deltaTime, 0.0f);
    if (state != CharacterState::Chase) chaseSoundTimer = 0.0f;
    stateTimer += deltaTime;
 
    switch (state){
        case CharacterState::Idle: {
            Vector2 start = WorldToImageCoords(position);

            // Transition to chase if player detected
            if (distanceSq < visionEnter && stateTimer > 1.0f && playerVisible) {
                AlertNearbySkeletons(position, 3000.0f);
                ChangeState(CharacterState::Chase);
                currentWorldPath.clear();

                SetPath(start);
            }

            // Wander if idle too long
            else if (stateTimer > 10.0f) {
                Vector2 randomTile = GetRandomReachableTile(start, this);

                if (IsWalkable(randomTile.x, randomTile.y, dungeonImg)) {
                    if (TrySetRandomPatrolPath(start, this, currentWorldPath)) {
                        state = CharacterState::Patrol;
                        SetAnimation(1, 4, 0.2f); // walk anim
                        if (type == CharacterType::Ghost) SetAnimation(0, 7, 0.2, true);
                    }
                }
            }

            break;
        }

        
        case CharacterState::Chase: {
            pathCooldownTimer = std::max(0.0f, pathCooldownTimer - deltaTime);

            UpdateChaseSound(deltaTime, player);

            if (distanceSq < (200.0f * 200.0f) && canSee) {
                ChangeState(CharacterState::Attack);

            }
            else if (distanceSq > visionEnter) {
                ChangeState(CharacterState::Idle);

            }
            else {
                const Vector2 curTile = WorldToImageCoords(player.position);
                if (pathCooldownTimer <= 0.0f){
                
                    lastPlayerTile = curTile;
                    pathCooldownTimer = RandomFloat(0.3, 0.9);
                    const Vector2 start = WorldToImageCoords(position);
                    SetPath(start); 
                
                }
                Vector3 repel = ComputeRepulsionForce(enemyPtrs, 300, 500); // your existing call
                float speed = (type == CharacterType::Zombie) ? 50.0f : 100.0f;
                // Move along current path
                MoveAlongPath(currentWorldPath, position, rotationY, skeleSpeed, deltaTime, speed, repel);
            }
        } break;

        case CharacterState::Attack: {
            //dont stand on the same tile as another skele when attacking
            Vector2 myTile = WorldToImageCoords(position);
            Character* occupier = GetTileOccupier(myTile.x, myTile.y, enemyPtrs, this);

            if (occupier && occupier != this) {
                // Only the one with the "greater" pointer backs off
                if (this > occupier) {
                    ChangeState(CharacterState::Reposition);
                    break;
                } else {
                    // Let the other one reposition — wait
                    break;
                }
            }

            if (distanceSq > (250.0f * 250.0f) && stateTimer > 0.5f) { //wait a bit before you switch back to chasing. 

                ChangeState(CharacterState::Chase);

            }

            float horizDist = DistXZ(position, player.position); //XZ dist allow you to jump enemy attacks. 

            attackCooldown -= deltaTime;
            if (attackCooldown <= 0.0f && currentFrame == 1 && playerVisible && horizDist < 200.0f) { // make sure you can see what your attacking. 
                attackCooldown = 0.8f; // 0.2 * 4 frames on animation for skele attack. 

                // Play attack sound
                if (type == CharacterType::Skeleton) SoundManager::GetInstance().Play(rand() % 2 ? "swipe2" : "swipe3");

                    // Blocked!
                if (CheckCollisionBoxes(GetBoundingBox(), player.blockHitbox) && player.blocking) {
                    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                    decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("blockSheet"), 4, 0.4f, 0.1f, 50.0f);
                    SoundManager::GetInstance().Play(rand() % 2 ? "swordBlock" : "swordBlock2");

      
                } else  {
                    // Player takes damage
                    player.TakeDamage(10);
                    
                    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));
                    if (type == CharacterType::Spider){
                        SoundManager::GetInstance().Play(rand() % 2 ? "spiderBite2" : "spiderBite1");
                        decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("biteSheet"), 4, 0.4f, 0.1f, 50.0f);
                    }else if (type == CharacterType::Skeleton){
                        SoundManager::GetInstance().Play("slice");
                        Decal decal = {offsetPos, DecalType::MeleeSwipe, R.GetTexture("slashSheet"), 5, 0.5f, 0.1f, 80.0f};

                        Vector3 vel = Vector3Scale(camDir, 0.0f); //no velocity looks better, keep this technology for later
                        decal.velocity = vel;
                        decals.emplace_back(decal);
                        //decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("slashSheet"), 5, 0.5f, 0.1f, 80.0f);
                    }else if (type == CharacterType::Ghost){
                        Vector3 mid = Vector3Lerp(position, player.position, 0.5f); //in between ghost and player
                        decals.emplace_back(mid, DecalType::MagicAttack, R.GetTexture("magicAttackSheet"), 8, 1.0f, 0.1f, 60.0f);
                        // siphon heal
                        int healOnHit = 20; 
                        currentHealth = std::min(maxHealth, currentHealth + healOnHit);
                    }


                }
                
            }
            break;
        }
        case CharacterState::Reposition: {
            //surround the player

            Vector2 playerTile = WorldToImageCoords(player.position);
            Vector3 target = position; // fallback

            bool foundSpot = FindRepositionTarget(player, position, target);

            if (foundSpot) {
                Vector3 dir = Vector3Normalize(Vector3Subtract(target, position));
                Vector3 move = Vector3Scale(dir, skeleSpeed * deltaTime);
                position = Vector3Add(position, move);

                rotationY = RAD2DEG * atan2f(dir.x, dir.z);
                position.y = target.y;

                //float dist = Vector3Distance(position, target);

                if (distanceSq < (300.0f * 300.0f) && stateTimer > 1.0f) {
                    ChangeState(CharacterState::Attack);
                } else if (distanceSq > (350.0f * 350.0f) && stateTimer > 1.0f) {
                    ChangeState(CharacterState::Chase);
                }
            }

            break;
        }



        case CharacterState::Patrol: {

            if (distanceSq < visionEnter && playerVisible){
                ChangeState(CharacterState::Chase);
                AlertNearbySkeletons(position, 3000.0f);

            }

            if (!currentWorldPath.empty()) { 
                Vector3 targetPos = currentWorldPath[0];
                Vector3 dir = Vector3Normalize(Vector3Subtract(targetPos, position));
                Vector3 move = Vector3Scale(dir, 500 * deltaTime); // slower than chase
                position = Vector3Add(position, move);
                rotationY = RAD2DEG * atan2f(dir.x, dir.z);
                position.y = targetPos.y;

                if (Vector3Distance(position, targetPos) < 100.0f) {
                    currentWorldPath.erase(currentWorldPath.begin());
                }
            }
            else {
                ChangeState(CharacterState::Idle);
            }

            break;
        }

        case CharacterState::Freeze: {
            //do nothing
            if (currentHealth <= 0 && !isDead){ 
                ChangeState(CharacterState::Death);
                break;
            }

            if (stateTimer > 5.0f && !isDead){
                ChangeState(CharacterState::Idle);
                break;
            }
            break;

        }

        case CharacterState::Harpooned: {

            if (currentHealth <= 0) {
                ChangeState(CharacterState::Death);
                break;
            }

            // Keep updating target so if player backpedals, it still pulls toward you
            Vector3 target = player.position;

            // Pull direction XZ only // try it on raptors on a hill. 
            Vector3 toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;

            float dist = Vector3Length(toTarget);

            // Stop distance so you don't overlap the player
            const float stopDist = harpoonMinDist;

            if (dist > stopDist && dist > 1.0f)
            {
                Vector3 dir = Vector3Scale(toTarget, 1.0f / dist);

                // Pull speed (world units / sec). Tune this.
                const float pullSpeed = 3000.0f;

                float step = pullSpeed * deltaTime;

                // Don't overshoot past stop distance
                float desiredMove = fminf(step, dist - stopDist);

                position = Vector3Add(position, Vector3Scale(dir, desiredMove));

            }

            // after position update
            toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;
            dist = Vector3Length(toTarget);
            
            // End condition: time AND close enough
            bool timeDone = (stateTimer >= harpoonDuration);
            bool closeEnough = (dist <= stopDist + 5.0f);

            if (timeDone && closeEnough)
            {
                currentWorldPath.clear();
                ChangeState(CharacterState::Stagger); //stun the target after grapple
                break;
            }

            break;
        }



        case CharacterState::Stagger: {

            //do nothing

            if (stateTimer >= 1.0f) {
                canBleed = true; //for spiders
                ChangeState(CharacterState::Chase);
                
            }
            break;
        }

        case CharacterState::Death:
            if (!isDead) {
                SetAnimation(4, 3, 0.5f, false); 
                if (type == CharacterType::Ghost) SetAnimation(1, 7, 0.2); 
                isDead = true;
                deathTimer = 0.0f;         // Start counting
            }

            deathTimer += deltaTime;
            break;
        }
       
}

void Character::UpdateTrexAI(float deltaTime, Player& player){
    stateTimer += deltaTime;
    attackCooldown  = std::max(0.0f, attackCooldown - deltaTime);

    const float distanceSq = Vector3DistanceSqr(position, player.position);
    UpdateRaptorVisibility(player, deltaTime);

    // --- Simple deadbands ---
    const float STALK_ENTER   = 2000.0f * 2000.0f;  // engage if closer than this
    const float ATTACK_EXIT   = 800.0f * 2000.0f;   // leave attack if beyond this 
    const float FLEE_ENTER    = 100.0f * 100.0f;   // too close -> run away


    switch (state)
    {

        case CharacterState::Idle:
        {

            const float IDLE_TO_PATROL_TIME = 10.0f; // tweak
            if (stateTimer >= IDLE_TO_PATROL_TIME) {
                // seed a patrol target around the current position
                patrolTarget = RandomPointOnRingXZ(position, /*minR*/800.0f, /*maxR*/2200.0f);

                hasPatrolTarget  = true;
                ChangeState(CharacterState::Patrol);
                SoundManager::GetInstance().PlaySoundAtPosition((GetRandomValue(0, 1) == 0 ? "TrexRoar" : "TrexRoar2"), position, player.position, 0.0, 6000);
                break;
            }

            if (distanceSq < STALK_ENTER && playerVisible) {
                if (canSee){
                    ChangeState(CharacterState::Chase);
                    SoundManager::GetInstance().PlaySoundAtPosition((GetRandomValue(0, 1) == 0 ? "TrexRoar" : "TrexRoar2"), position, player.position, 0.0, 6000);
                } 
                
                
                break;
            }
        } break;

        case CharacterState::Patrol:
        {
            UpdatePatrol(deltaTime);
            break;
        }

        case CharacterState::Chase:
        {
            UpdateTrexStepSFX(deltaTime);
            UpdateChase(deltaTime);
            break;
        }

        case CharacterState::Attack:
        {
            if (distanceSq > ATTACK_EXIT) {
                if (canSee) ChangeState(CharacterState::Chase);
                break;
            }

            if (distanceSq < FLEE_ENTER) { ChangeState(CharacterState::RunAway); break;}
            if (attackCooldown <= 0.0f && distanceSq < ATTACK_EXIT && canSee){
                attackCooldown = 3.0f;
                player.TakeDamage(20);
                //add bite decal
                Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));
                decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("biteSheet"), 4, 0.4f, 0.1f, 100.0f);

                SoundManager::GetInstance().Play(GetRandomValue(0, 1) == 0 ? "TrexBite" : "TrexBite2");
                break;
            }
            
            

        } break;

        case CharacterState::RunAway:
        {
            UpdateRunaway(deltaTime);
            break;
        }


        case CharacterState::Freeze:
        {

            //do nothing
            if (currentHealth <= 0 && !isDead){
                ChangeState(CharacterState::Death); //check for death to freeze
                break;
            }

            if (stateTimer > 5.0f && !isDead){
                ChangeState(CharacterState::Idle);
                break;
            }
        } break;

        case CharacterState::Stagger:
        {
            //do nothing
            
            if (stateTimer >= 1.0f) {
                canBleed = true;
                ChangeState(CharacterState::Chase);
                
                
                //chaseDuration = GetRandomValue(4, 8); 
            }
        } break;

        case CharacterState::Death:
        {
            if (!isDead) {
                SoundManager::GetInstance().Play(GetRandomValue(0, 1) == 0 ? "TrexHurt" : "TrexHurt2");
                isDead = true;
                deathTimer = 0.0f;// Start counting
            }

            deathTimer += deltaTime;
        } break;


    }

}


void Character::UpdateDactylAI(float deltaTime, Player& player)
{
    // --- Perception & timers ---
    stateTimer     += deltaTime;
    attackCooldown  = std::max(0.0f, attackCooldown - deltaTime);

    const float distanceSq = Vector3DistanceSqr(position, player.position);
    //UpdateRaptorVisibility(player, deltaTime);
    UpdateLeavingFlag(player.position, player.previousPosition);

    // --- Simple deadbands ---
    float STALK_ENTER = 3000.0f * 3000.0f; //birds can't see player when in free cam. 
    float VISION_ENTER = 4000.0f * 4000.0f;
    const float STALK_EXIT    = 3400.0f * 3400.0f;  // drop back to idle 
    const float ATTACK_ENTER  = 200.0f * 200.0f;   // start attack if closer than this
    const float ATTACK_EXIT   = 300.0f * 200.0f;   // leave attack if beyond this 
    const float FLEE_ENTER    = 100.0f * 100.0f;   // too close -> run away
    const float FLEE_EXIT     = 2000.0f * 2000.0f;   // far enough -> stop fleeing


    switch (state)
    {

        case CharacterState::Idle:
        {

            const float IDLE_TO_PATROL_TIME = 1.0f; // tweak
            if (stateTimer >= IDLE_TO_PATROL_TIME) {

                patrolTarget = RandomPointOnHeightmapRingXZ(
                    position,
                    /*minR*/ 3000.0f,
                    /*maxR*/ 8200.0f,
                    heightmap.width,   // pixels
                    terrainScale.x,     // world units per pixel
                    /*edgeMargin*/ 400.0f
                );
                //pick a random altitude before patrolling
                float altitudes[3] = {800.0f, 1200.0f, 1600.0f};
                int idx = GetRandomValue(0, 2);
                patrolAlt = altitudes[idx];

                hasPatrolTarget = true;
                ChangeState(CharacterState::Patrol);
                break;
            }

            if (distanceSq < STALK_ENTER) {
                ChangeState(CharacterState::Chase);
                
                break;
            }
        } break;

        case CharacterState::Patrol:
        {

            UpdateMovementAnim();

            UpdatePatrolSteering(deltaTime);
            break;   
        }

        case CharacterState::Chase:
        {
            UpdateMovementAnim();
            UpdateChaseSteering(deltaTime);
          
        } break;


        case CharacterState::Attack:

        {
            if (stateTimer > 3.0f){
                ChangeState(CharacterState::RunAway);
            }
            if (distanceSq > ATTACK_EXIT) {
                if (canSee) ChangeState(CharacterState::Chase);
                break;
            }
            
            if (distanceSq < FLEE_ENTER) { ChangeState(CharacterState::RunAway); break;}


            //float horizDist = DistXZ(position, player.position);
            //float distance = Vector3Distance(position, player.position);
            if (attackCooldown <= 0.0f && distanceSq < ATTACK_EXIT && distanceSq < ATTACK_ENTER) {
                attackCooldown = 1.0f; // seconds between attacks

                // Play attack sound
                SoundManager::GetInstance().Play("dinoBite");
                // Damage the player
                if (CheckCollisionBoxes(GetBoundingBox(), player.blockHitbox) && player.blocking) {
                    // Blocked!

                    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                    decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("blockSheet"), 4, 0.4f, 0.1f, 50.0f);
                    
                    SoundManager::GetInstance().Play(rand()%2 ? "swordBlock" : "swordBlock2");
        
                } else  {
                    // Player takes damage
                    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                    decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("biteSheet"), 4, 0.4f, 0.1f, 50.0f);
                    
                    player.TakeDamage(10);
                }
            }

        } break;

        case CharacterState::RunAway:
        {

            UpdateMovementAnim();
            UpdateRunawaySteering(deltaTime);

            
        } break;

        case CharacterState::Harpooned: {
            //stateTimer += deltaTime;  //we were adding to stateTimer twice. So harpoon timer would only run for 1 second instead of 2. 
            //never noticed. 

            if (currentHealth <= 0) {
                ChangeState(CharacterState::Death);
                break;
            }

            // Keep updating target so if player backpedals, it still pulls toward you
            Vector3 target = player.position; 

            // Pull direction XZ only // try it on raptors on a hill. 
            Vector3 toTarget = Vector3Subtract(target, position);
            //toTarget.y = 0.0f;

            float dist = Vector3Length(toTarget);

            // Stop distance so you don't overlap the player
            const float stopDist = harpoonMinDist;

            if (dist > stopDist && dist > 1.0f)
            {
                Vector3 dir = Vector3Scale(toTarget, 1.0f / dist);

                // Pull speed (world units / sec). Tune this.
                const float pullSpeed = 3000.0f;

                float step = pullSpeed * deltaTime;

                // Don't overshoot past stop distance
                float desiredMove = fminf(step, dist - stopDist);

                position = Vector3Add(position, Vector3Scale(dir, desiredMove));

            }

            // after position update
            toTarget = Vector3Subtract(target, position);
            //toTarget.y = 0.0f;
            dist = Vector3Length(toTarget);
            
            // End condition: time AND close enough
            bool timeDone = (stateTimer >= harpoonDuration);
            bool closeEnough = (dist <= stopDist + 5.0f);

            if (timeDone && closeEnough)
            {
                currentWorldPath.clear();
                ChangeState(CharacterState::Stagger);
                break;
            }

            break;
        }


        case CharacterState::Freeze:
        {

            //do nothing
            if (currentHealth <= 0 && !isDead){
                ChangeState(CharacterState::Death); //check for death to freeze, freeze damage doesn't call takeDamage
                break;
            }

            if (stateTimer > 5.0f && !isDead){
                ChangeState(CharacterState::Idle);
                break;
            }
        } break;

        case CharacterState::Stagger:
        {
            //do nothing
 
            if (stateTimer >= 1.0f) {
                canBleed = true;
                ChangeState(CharacterState::Chase);
                
                chaseDuration = GetRandomValue(4, 8); 
            }
        } break;

        case CharacterState::Death:
        {
            if (!isDead) {
                isDead = true;
                deathTimer = 0.0f;// Start counting
            }

            deathTimer += deltaTime;
        } break;
    }

}

void Character::UpdateRaptorAI(float deltaTime, Player& player)
{
    // --- Perception & timers ---
    stateTimer     += deltaTime;
    attackCooldown  = std::max(0.0f, attackCooldown - deltaTime);

    const float distanceSq = Vector3DistanceSqr(position, player.position);
    UpdateRaptorVisibility(player, deltaTime);
    UpdateLeavingFlag(player.position, player.previousPosition);

    //UpdateLeavingFlag(player.position);

    // --- Simple deadbands ---
    const float STALK_ENTER   = 3000.0f * 3000.0f;  // engage if closer than this
    const float STALK_EXIT    = 3400.0f * 3400.0f;  // drop back to idle 
    const float ATTACK_ENTER  = 200.0f * 200.0f;   // start attack if closer than this
    const float ATTACK_EXIT   = 300.0f * 300.0f;   // leave attack if beyond this 
    const float FLEE_ENTER    = 100.0f * 100.0f;   // too close -> run away
    const float FLEE_EXIT     = 2000.0f * 2000.0f;   // far enough -> stop fleeing
    const float VISION_ENTER = 4000.0f * 4000.0f;


    switch (state)
    {

        case CharacterState::Idle:
        {

            const float IDLE_TO_PATROL_TIME = 10.0f; // tweak
            if (stateTimer >= IDLE_TO_PATROL_TIME) {
                // seed a patrol target around the current position
                patrolTarget = RandomPointOnRingXZ(position, /*minR*/800.0f, /*maxR*/2200.0f);

                hasPatrolTarget  = true;
                ChangeState(CharacterState::Patrol);
                break;
            }

            if (distanceSq < STALK_ENTER && playerVisible) {
                if (canSee) ChangeState(CharacterState::Chase);
                
                break;
            }
        } break;

        case CharacterState::Patrol:
        {

            UpdateMovementAnim();

            UpdatePatrol(deltaTime);
            break;   
        }

        case CharacterState::Chase:
        {
            UpdateMovementAnim();
            UpdateChase(deltaTime);
          
        } break;


        case CharacterState::Attack:

        {
            if (stateTimer > 3.0f){
                ChangeState(CharacterState::RunAway);
            }
            if (distanceSq > ATTACK_EXIT) {
                if (canSee) ChangeState(CharacterState::Chase);
                break;
            }
            
            if (distanceSq < FLEE_ENTER) { ChangeState(CharacterState::RunAway); break;}


            float horizDist = DistXZ(position, player.position);
            if (attackCooldown <= 0.0f && distanceSq < ATTACK_EXIT && horizDist < ATTACK_ENTER) {
                attackCooldown = 1.0f; // seconds between attacks

                // Play attack sound
                SoundManager::GetInstance().Play("dinoBite");
                // Damage the player
                if (CheckCollisionBoxes(GetBoundingBox(), player.blockHitbox) && player.blocking) {
                    // Blocked!

                    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                    decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("blockSheet"), 4, 0.4f, 0.1f, 50.0f);
                    
                    SoundManager::GetInstance().Play(rand()%2 ? "swordBlock" : "swordBlock2");
        
                } else  {
                    // Player takes damage
                    Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                    Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                    decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("biteSheet"), 4, 0.4f, 0.1f, 50.0f);
                    
                    player.TakeDamage(10);
                }
            }

        } break;

        case CharacterState::RunAway:
        {

            UpdateMovementAnim();
            UpdateRunaway(deltaTime);

            
        } break;

        case CharacterState::Harpooned: {
            //stateTimer += deltaTime;  //we were adding to stateTimer twice. So harpoon timer would only run for 1 second instead of 2. 
            //never noticed. 

            if (currentHealth <= 0) {
                ChangeState(CharacterState::Death);
                break;
            }

            // Keep updating target so if player backpedals, it still pulls toward you
            Vector3 target = player.position; 

            // Pull direction XZ only // try it on raptors on a hill. 
            Vector3 toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;

            float dist = Vector3Length(toTarget);

            // Stop distance so you don't overlap the player
            const float stopDist = harpoonMinDist;

            if (dist > stopDist && dist > 1.0f)
            {
                Vector3 dir = Vector3Scale(toTarget, 1.0f / dist);

                // Pull speed (world units / sec). Tune this.
                const float pullSpeed = 3000.0f;

                float step = pullSpeed * deltaTime;

                // Don't overshoot past stop distance
                float desiredMove = fminf(step, dist - stopDist);

                position = Vector3Add(position, Vector3Scale(dir, desiredMove));

            }

            // after position update
            toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;
            dist = Vector3Length(toTarget);
            
            // End condition: time AND close enough
            bool timeDone = (stateTimer >= harpoonDuration);
            bool closeEnough = (dist <= stopDist + 5.0f);

            if (timeDone && closeEnough)
            {
                currentWorldPath.clear();
                ChangeState(CharacterState::Stagger);
                break;
            }

            break;
        }


        case CharacterState::Freeze:
        {
            stateTimer += deltaTime;
            //do nothing
            if (currentHealth <= 0 && !isDead){
                ChangeState(CharacterState::Death); //check for death to freeze, freeze damage doesn't call takeDamage
                break;
            }

            if (stateTimer > 5.0f && !isDead){
                ChangeState(CharacterState::Idle);
                break;
            }
            
            
        } break;

        case CharacterState::Stagger:
        {
            //do nothing
 
            if (stateTimer >= 1.0f) {
                canBleed = true;
                ChangeState(CharacterState::Chase);
                
                chaseDuration = GetRandomValue(4, 8); 
            }
        } break;

        case CharacterState::Death:
        {
            deathTimer += deltaTime;
            if (!isDead) {
                isDead = true;
                deathTimer = 0.0f;// Start counting
            }

     
        } break;
    }

}

void Character::UpdateWizardAI(float deltaTime, Player& player) {
    if (isLoadingLevel) return;

    // (EXIT must be > ENTER)

    stateTimer += deltaTime; //state timer should be outside of states, change this for pirates as well. 
    float distanceSq = Vector3DistanceSqr(position, player.position);


    constexpr float VISION_ENTER = 4000.0f * 4000.0f;
    constexpr float WIZARD_ATTACK_ENTER = 1600.0f * 1600.0f; // start attacking when closer than this
    constexpr float WIZARD_ATTACK_EXIT  = 2500.0f * 2500.0f; // stop attacking when farther than this
    constexpr float WIZARD_MELEE_ENTER  = 250.0f * 250.0f; // stop attacking when farther than this
    Vector2 start = WorldToImageCoords(position);
    //Vector2 goal = WorldToImageCoords(player.position);

    UpdatePlayerVisibility(player.position, deltaTime, 0.0f);
    UpdateLeavingFlag(player.position, player.previousPosition);
 
    switch (state){
        case CharacterState::Idle: {


            Vector2 start = WorldToImageCoords(position);

            if (distanceSq < WIZARD_MELEE_ENTER && canSee){
                ChangeState(CharacterState::MeleeAttack);
                break;
            }

            // Transition to chase if player detected
            if (distanceSq < VISION_ENTER && stateTimer > 1.0f && (playerVisible)) {
                AlertNearbySkeletons(position, 3000.0f);
                ChangeState(CharacterState::Chase);
                SetPath(start);

            }
            // Wander if idle too long
            else if (stateTimer > 10.0f) {

                
                if (TrySetRandomPatrolPath(start, this, currentWorldPath)) {

                    ChangeState(CharacterState::Patrol);
                    
                }
            }

            break;
        }

        case CharacterState::Chase: {

            pathCooldownTimer = std::max(0.0f, pathCooldownTimer - deltaTime);

            UpdateMovementAnim();
            if (distanceSq < WIZARD_MELEE_ENTER && canSee){
                ChangeState(CharacterState::MeleeAttack);
                break;
            }

            // 1) Try to attack when close AND we have instant LOS
            if (distanceSq < WIZARD_ATTACK_ENTER && canSee) {
                ChangeState(CharacterState::Attack);
                break;
            }

            // 2) Leash out if too far
            if (distanceSq > VISION_ENTER) {
                ChangeState(CharacterState::Idle);
                playerVisible = false;         // drop memory when giving up
                currentWorldPath.clear();
                break;
            }

            // 3) Plan path toward current target when cooldown allows
            if (pathCooldownTimer <= 0.0f) {
                if (canSee) {
                    SetPath(start);
                    //SetPathTo(player.position);
                    pathCooldownTimer = 0.4f;
                } else if (playerVisible) {       // still within memory window
                    SetPathTo(lastKnownPlayerPos);
                    
                    pathCooldownTimer = 0.4f;
                }
            }


            // 4) Advance along path (with repulsion)
            if (!currentWorldPath.empty() && state != CharacterState::Stagger) {
                Vector3 repel = ComputeRepulsionForce(enemyPtrs, 300, 500); // your existing call
                MoveAlongPath(currentWorldPath, position, rotationY, skeleSpeed, deltaTime, 100.0f, repel);

                // Reached the end but still no LOS? stop chasing
                if (currentWorldPath.empty() && !canSee) {
                    playerVisible = false;          // memory expires now that we arrived
                    ChangeState(CharacterState::Idle);
                }
            }
        } break;

        case CharacterState::Attack: { //wizard attacks with fireball
            
            Vector2 myTile = WorldToImageCoords(position);
            Character* occupier = GetTileOccupier(myTile.x, myTile.y, enemyPtrs, this);
            //pirates won't occupy the same tile while shooting. 
            if (occupier && occupier != this) {
                // Only the one with the "greater" pointer backs off
                if (this > occupier) {
                    ChangeState(CharacterState::Reposition);
                    break;
                } else {
                    // Let the other one reposition — wait
                    break;
                }
            }

            // Hysteresis: only leave attack if we're clearly out of range or lost LOS
            if (distanceSq > WIZARD_ATTACK_ENTER || !canSee) {
                ChangeState(CharacterState::Chase);
                break;
            }

            attackCooldown -= deltaTime;
            if (distanceSq < WIZARD_ATTACK_ENTER && distanceSq > WIZARD_MELEE_ENTER){
                
                if (canSee && attackCooldown <= 0.0f && currentFrame == 1 && !hasFired && type == CharacterType::Wizard) {

                    FireFireball(position, player.position, 1350.0f, 2.0f, true, true, true);
                    hasFired = true;
                    attackCooldown = 5.0f;
                    SoundManager::GetInstance().PlaySoundAtPosition("flame1", position, player.position, 1.0, 2000);
                }

            }else if (distanceSq < WIZARD_MELEE_ENTER){ //was 280
                ChangeState(CharacterState::MeleeAttack);
                break;
            }

            // Wait for next attack opportunity
            if (hasFired && stateTimer > 1.5f) {
                hasFired = false;
                attackCooldown = 3.5f;
                currentFrame = 0;
                stateTimer = 0;

                if (TrySetRandomPatrolPath(start, this, currentWorldPath) && canSee) { //shoot then move to a random tile and shoot again.
                    ChangeState(CharacterState::Patrol);
                }else{
                    ChangeState(CharacterState::Attack);
                }
               
            }

            break;
        }

        case CharacterState::MeleeAttack: {
            stateTimer += deltaTime;
            float horizDist = DistXZ(position, player.position);

            // Wait until animation is done to apply damage
            if (canMelee && currentFrame == 2 && horizDist < 280.0f) { // only apply damage on frame 2. Consider enemy attack ID
                canMelee = false;

                if (distanceSq < (300.0f * 300.0f)) {
                    if (CheckCollisionBoxes(GetBoundingBox(), player.blockHitbox) && player.blocking) {
                        // Blocked!

                        Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                        Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                        decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("blockSheet"), 4, 0.4f, 0.1f, 50.0f);
                        SoundManager::GetInstance().Play(rand()%2 ? "swordBlock" : "swordBlock2");

                    } else {
                        // Direct hit
                        player.TakeDamage(10);
                                               
                        SoundManager::GetInstance().Play("slice");

                        Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                        Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                        decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("slashSheetLeft"), 5, 0.5f, 0.1f, 50.0f);
                    }
                }
            }

            // Exit state after full animation plays
            if (stateTimer >= 0.6f) {
                if (distanceSq > WIZARD_MELEE_ENTER) {
                    Vector2 start = WorldToImageCoords(position);
                    if (TrySetRandomPatrolPath(start, this, currentWorldPath)){
                        ChangeState(CharacterState::Patrol);
                    }else{
                        ChangeState(CharacterState::Idle);

                    }

                } else {
                    ChangeState(CharacterState::MeleeAttack);
                }
                canMelee = true;
                stateTimer = 0.0f;
            }

            break;
        }


        case CharacterState::Reposition: {
            //skeletons and pirates and spiders, when close, surround the player. Instead of all standing on the same tile. 
            stateTimer += deltaTime;

            Vector2 playerTile = WorldToImageCoords(player.position);
            Vector3 target = position; // fallback

            bool foundSpot = FindRepositionTarget(player, position, target);


            if (foundSpot) {
                foundSpot = false;
                Vector3 dir = Vector3Normalize(Vector3Subtract(target, position));
                Vector3 move = Vector3Scale(dir, skeleSpeed * deltaTime);
                position = Vector3Add(position, move);

                rotationY = RAD2DEG * atan2f(dir.x, dir.z);

                if (distanceSq < WIZARD_MELEE_ENTER && stateTimer > 2.0f) {
                    ChangeState(CharacterState::MeleeAttack);
                } else if (distanceSq > (350.0f * 350.0f) && stateTimer > 2.0f) {
                    ChangeState(CharacterState::Chase);
                }
            }

            break;
        }

        case CharacterState::Patrol: { //Pirate Patrol after every shot. 
            stateTimer += deltaTime;

            UpdateMovementAnim();
            // Advance along path (with repulsion)
            if (!currentWorldPath.empty() && state != CharacterState::Stagger) {
                Vector3 repel = ComputeRepulsionForce(enemyPtrs, 300, 500); // your existing call
                MoveAlongPath(currentWorldPath, position, rotationY, skeleSpeed, deltaTime, 100.0f, repel);

                // Reached the end but still no LOS? stop chasing
                if (currentWorldPath.empty() && !canSee) {
                    playerVisible = false;          // memory expires now that we arrived
                    ChangeState(CharacterState::Idle);
                }
            }
            
            else {
                ChangeState(CharacterState::Idle);
            }

            break;
        }

        case CharacterState::Harpooned: {
            stateTimer += deltaTime;

            if (currentHealth <= 0) {
                ChangeState(CharacterState::Death);
                break;
            }

            // Keep updating target so if player backpedals, it still pulls toward you
            Vector3 target = player.position;

            // Pull direction XZ only // try it on raptors on a hill. 
            Vector3 toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;

            float dist = Vector3Length(toTarget);

            // Stop distance so you don't overlap the player
            const float stopDist = harpoonMinDist;

            if (dist > stopDist && dist > 1.0f)
            {
                Vector3 dir = Vector3Scale(toTarget, 1.0f / dist);

                // Pull speed (world units / sec). Tune this.
                const float pullSpeed = 3000.0f;

                float step = pullSpeed * deltaTime;

                // Don't overshoot past stop distance
                float desiredMove = fminf(step, dist - stopDist);

                position = Vector3Add(position, Vector3Scale(dir, desiredMove));

            }

            // after position update
            toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;
            dist = Vector3Length(toTarget);
            
            // End condition: time AND close enough
            bool timeDone = (stateTimer >= harpoonDuration);
            bool closeEnough = (dist <= stopDist + 5.0f);

            if (timeDone && closeEnough)
            {
                currentWorldPath.clear();
                ChangeState(CharacterState::Stagger);
                break;
            }

            break;
        }


        case CharacterState::Freeze: {
            stateTimer += deltaTime;
            //do nothing
            if (currentHealth <= 0 && !isDead){
                ChangeState(CharacterState::Death); //check for death to freeze, freeze damage doesn't call takeDamage
                break;
            }

            if (stateTimer > 5.0f && !isDead){
                ChangeState(CharacterState::Idle);
                break;
            }
            
            break;
        }


        case CharacterState::Stagger: {
            stateTimer += deltaTime;
            //do nothing
           
            if (stateTimer > 0.9f) {
                canBleed = true;
                if (distanceSq > WIZARD_MELEE_ENTER){
                    ChangeState(CharacterState::Chase);
                }else{
                    ChangeState(CharacterState::MeleeAttack);
                }
                
                
            }
            break;
        }

        case CharacterState::Death: {
            if (!isDead) {
                isDead = true;
                deathTimer = 0.0f;         // Start counting
            }

            deathTimer += deltaTime;
            break;
        }

    }
}

void Character::UpdatePirateAI(float deltaTime, Player& player) {
    if (isLoadingLevel) return;

    stateTimer += deltaTime; // is always counting
    
    float distanceSq = Vector3DistanceSqr(position, player.position);

    Vector2 start = WorldToImageCoords(position);
    constexpr float VISION_ENTER = 4000.0f * 4000.0f;
    constexpr float PIRATE_ATTACK_ENTER = 800.0f * 800.0f; // start attacking when closer than this
    constexpr float PIRATE_ATTACK_EXIT  = 900.0f * 900.0f; // stop attacking when farther than this
    constexpr float PIRATE_MELEE_ENTER = 250.0f * 250.0f;

    UpdatePlayerVisibility(player.position, deltaTime, 0.0f);
    UpdateLeavingFlag(player.position, player.previousPosition);
    UpdateTargeting(deltaTime, player, enemyPtrs);
   
    switch (state){
        case CharacterState::Idle: {
            

            Vector2 start = WorldToImageCoords(position);
 
            if (distanceSq < PIRATE_MELEE_ENTER && canSee){
                ChangeState(CharacterState::MeleeAttack);
                break;
            }

            // Transition to chase if player detected
            if (distanceSq < VISION_ENTER && stateTimer > 1.0f && (playerVisible)) {
                AlertNearbySkeletons(position, 3000.0f);
                ChangeState(CharacterState::Chase);
                SetPath(start);

            }
            // Wander if idle too long
            else if (stateTimer > 10.0f) {

                
                if (TrySetRandomPatrolPath(start, this, currentWorldPath)) {

                    ChangeState(CharacterState::Patrol);
                    
                }
            }

            break;
        }

        case CharacterState::Chase:
        {
            pathCooldownTimer = std::max(0.0f, pathCooldownTimer - deltaTime);

            UpdateChaseSound(deltaTime, player);
            UpdateMovementAnim();

            // --- Update pirate target selection (zombies) ---
            // This sets: target, targetDist, targetCanSee
            //UpdateTargeting(deltaTime, player, enemyPtrs);

            // --- Pick who we're chasing/attacking this tick ---
            // Default = player
            Vector3 chasePos      = player.position;
            float   chaseDist     = distanceSq;   // your existing distance-to-player
            bool    chaseCanSee   = canSee;     // your existing LOS-to-player
            bool    chasingZombie = false;

            if (type == CharacterType::Pirate && target && !target->isDead)
            {
                chasingZombie = true;
                chasePos    = target->position;
                chaseDist   = targetDist;
                chaseCanSee = targetCanSee;

            }

            // --- MELEE: super close -> melee (same rule, but uses chaseDist now) ---
            if (chaseDist < PIRATE_MELEE_ENTER && chaseCanSee)
            {
                ChangeState(CharacterState::MeleeAttack);
                break;
            }

            // --- GUN ATTACK: within enter range + LOS ---
            if (chaseDist < PIRATE_ATTACK_ENTER && chaseCanSee)
            {
                ChangeState(CharacterState::Attack);
                break;
            }

            // --- LEASH: too far -> idle (same behavior) ---
            if (chaseDist > VISION_ENTER)
            {
                ChangeState(CharacterState::Idle);

                // Only drop player memory when we were actually chasing the player
                if (!chasingZombie)
                    playerVisible = false;

                currentWorldPath.clear();
                break;
            }

            // --- PATHING ---
            if (pathCooldownTimer <= 0.0f)
            {
                // If chasing a zombie, just go directly to it (no "memory" logic)
                if (chasingZombie)
                {
                    SetPathTo(chasePos);
                    pathCooldownTimer = 0.4f;
                }
                else
                {
                    // Original player memory behavior
                    if (canSee)
                    {
                        SetPath(start); // your existing start->player BFS call
                        // or: SetPathTo(player.position);
                        pathCooldownTimer = 0.4f;
                    }
                    else if (playerVisible)
                    {
                        SetPathTo(lastKnownPlayerPos);
                        pathCooldownTimer = 0.4f;
                    }
                }
            }

            // --- ADVANCE ---
            if (!currentWorldPath.empty() && state != CharacterState::Stagger)
            {
                Vector3 repel = ComputeRepulsionForce(enemyPtrs, 300, 500);
                MoveAlongPath(currentWorldPath, position, rotationY, skeleSpeed, deltaTime, 100.0f, repel);

                // Reached the end but still no LOS? stop chasing (player-only rule)
                if (currentWorldPath.empty() && !chaseCanSee)
                {
                    if (!chasingZombie)
                    {
                        playerVisible = false;
                        ChangeState(CharacterState::Idle);
                    }
                    else
                    {
                        // Zombie target lost / unreachable -> just idle or repick next tick
                        ChangeState(CharacterState::Idle);
                    }
                }
            }

        } break;
        
        case CharacterState::Attack:
        {
            if (hasFired && stateTimer < 1.5f){
                //wait before switching states after firing. 
                break;
            }

            //UpdateTargeting(deltaTime, player, enemyPtrs);

            Vector3 aimPos = player.position;
            float   dist   = distanceSq; // existing distance to player
            bool    los    = canSee;

            if (type == CharacterType::Pirate && target)
            {
                aimPos = target->position;
                dist   = targetDist;
                los    = targetCanSee;
            }

            // Tile occupier logic stays the same (prevents stacking while shooting)
            Vector2 myTile = WorldToImageCoords(position);
            Character* occupier = GetTileOccupier(myTile.x, myTile.y, enemyPtrs, this);
            if (occupier && occupier != this)
            {
                if (this > occupier) { ChangeState(CharacterState::Reposition); break; }
                else { break; }
            }

            if (dist > PIRATE_ATTACK_EXIT || !los)
            {
                ChangeState(CharacterState::Chase);
                break;
            }

            attackCooldown -= deltaTime;

            if (dist < PIRATE_ATTACK_ENTER && dist > PIRATE_MELEE_ENTER)
            {
                if (los && attackCooldown <= 0.0f && currentFrame == 1 && !hasFired)
                {
                    FireBullet(position, aimPos, 1500.0f, 3.0f, true, false);
                    hasFired = true;
                    attackCooldown = 1.5f;

                    // Sound: if shooting zombie, use *player.position* for attenuation listener as usual
                    SoundManager::GetInstance().PlaySoundAtPosition("musket", position, player.position, 1.0f, 2000);
                }
            }
            else if (dist < 280)
            {
                ChangeState(CharacterState::MeleeAttack);
                break;
            }

            if (hasFired && stateTimer > 1.5f)
            {
                hasFired = false;
                attackCooldown = 3.5f;
                currentFrame = 0;
                stateTimer = 0;

                // Patrol/reposition after shot — still works
                // If you want to reposition *relative to target*, you can bias it later.
                if (TrySetRandomPatrolPath(WorldToImageCoords(position), this, currentWorldPath) && los)
                    ChangeState(CharacterState::Patrol);
                else
                    ChangeState(CharacterState::Attack);
            }

        } break;

        case CharacterState::MeleeAttack:
        {
            // --- Pick victim (player by default) ---
            Vector3 victimPos    = player.position;
            float   victimDist   = distanceSq;  // your existing distance-to-player
            bool    victimCanSee = canSee;    // your existing LOS-to-player
            bool    attackingZombie = false;
            Character* victim = nullptr;

            if (type == CharacterType::Pirate && target && !target->isDead && target->type == CharacterType::Zombie)
            {
                attackingZombie = true;
                victim = target;

                victimPos    = target->position;
                victimDist   = targetDist;     // set by UpdateTargeting
                victimCanSee = targetCanSee;   // set by UpdateTargeting
            }

            float horizDist = DistXZ(position, victimPos);

            // Wait until animation frame to apply damage
            if (canMelee && currentFrame == 2 && horizDist < 280.0f)
            {
                canMelee = false;

                // (Optional) require LOS for melee too, similar to your gun logic
                // if (!victimCanSee) { /* skip damage this swing */ }

                if (victimDist < PIRATE_MELEE_ENTER)
                {
                    if (!attackingZombie)
                    {
                        // --- PLAYER HIT LOGIC (unchanged) ---
                        if (CheckCollisionBoxes(GetBoundingBox(), player.blockHitbox) && player.blocking)
                        {
                            Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                            Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                            decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("blockSheet"), 4, 0.4f, 0.1f, 50.0f);
                            SoundManager::GetInstance().Play(rand()%2 ? "swordBlock" : "swordBlock2");
                        }
                        else
                        {
                            player.TakeDamage(10);
                            SoundManager::GetInstance().Play("slice");

                            Vector3 camDir = Vector3Normalize(Vector3Subtract(position, player.position));
                            Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                            decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("slashSheetLeft"), 5, 0.5f, 0.1f, 50.0f);
                        }
                    }
                    else
                    {
                        // --- ZOMBIE HIT LOGIC ---
                        victim->TakeDamage(50);

                        SoundManager::GetInstance().PlaySoundAtPosition("slice", position, player.position, 0.0f, 3000.0f);

                        // Decal facing zombie
                        Vector3 camDir = Vector3Normalize(Vector3Subtract(position, victimPos));
                        Vector3 offsetPos = Vector3Add(position, Vector3Scale(camDir, -100.0f));

                        decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("slashSheetLeft"), 5, 0.5f, 0.1f, 50.0f);
                    }
                }
            }

            // Exit / loop after full animation
            if (stateTimer >= 0.6f)
            {
                if (victimDist > (280.0f * 280.0f))
                {
                    Vector2 start = WorldToImageCoords(position);
                    if (TrySetRandomPatrolPath(start, this, currentWorldPath))
                        ChangeState(CharacterState::Patrol);
                    else
                        ChangeState(CharacterState::Idle);
                }
                else
                {
                    if (victim){ //if attacking zombie, melee attack once and reposition
                        Vector2 start = WorldToImageCoords(position);
                        if (TrySetRandomPatrolPath(start, this, currentWorldPath))
                            ChangeState(CharacterState::Patrol);
                        else
                            ChangeState(CharacterState::Idle);
                       
                    }else{
                        ChangeState(CharacterState::MeleeAttack);
                    }
                    // Still close: keep swinging

                }

                canMelee = true;
                stateTimer = 0.0f;
            }

            break;
        }


        case CharacterState::Reposition: {
            //skeletons and pirates and spiders, when close, surround the player. Instead of all standing on the same tile. 

            Vector2 playerTile = WorldToImageCoords(player.position);
            Vector3 target = position; // fallback

            bool foundSpot = FindRepositionTarget(player, position, target);


            if (foundSpot) {
                foundSpot = false;
                Vector3 dir = Vector3Normalize(Vector3Subtract(target, position));
                Vector3 move = Vector3Scale(dir, skeleSpeed * deltaTime);
                position = Vector3Add(position, move);

                rotationY = RAD2DEG * atan2f(dir.x, dir.z);
                //position.y = pirateHeight;


                if (distanceSq < (300.0f * 300.0f) && stateTimer > 2.0f) {
                    ChangeState(CharacterState::MeleeAttack);
                } else if (distanceSq > (350.0f * 350.0f) && stateTimer > 2.0f) {
                    ChangeState(CharacterState::Chase);
                }
            }

            break;
        }

        case CharacterState::Patrol: { //Pirate Patrol after every shot. 
            //ignore player while patroling to new tile. This means they finish patrolling before attacking you

            UpdateMovementAnim();
            // Advance along path (with repulsion)
            if (!currentWorldPath.empty() && state != CharacterState::Stagger) {
                Vector3 repel = ComputeRepulsionForce(enemyPtrs, 300, 500); // your existing call
                MoveAlongPath(currentWorldPath, position, rotationY, skeleSpeed, deltaTime, 100.0f, repel);

                // Reached the end but still no LOS? stop chasing
                if (currentWorldPath.empty() && !canSee) {
                    playerVisible = false;          // memory expires now that we arrived
                    ChangeState(CharacterState::Idle);
                }
            }
            
            else {
                ChangeState(CharacterState::Idle);
            }

            break;
        }

        case CharacterState::Harpooned: {

            if (currentHealth <= 0) {
                ChangeState(CharacterState::Death);
                break;
            }

            // Keep updating target so if player backpedals, it still pulls toward you
            Vector3 target = player.position;

            // Pull direction XZ only // try it on raptors on a hill. 
            Vector3 toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;

            float dist = Vector3Length(toTarget);

            // Stop distance so you don't overlap the player
            const float stopDist = harpoonMinDist;

            if (dist > stopDist && dist > 1.0f)
            {
                Vector3 dir = Vector3Scale(toTarget, 1.0f / dist);

                // Pull speed (world units / sec). Tune this.
                const float pullSpeed = 3000.0f;

                float step = pullSpeed * deltaTime;

                // Don't overshoot past stop distance
                float desiredMove = fminf(step, dist - stopDist);

                position = Vector3Add(position, Vector3Scale(dir, desiredMove));

            }

            // after position update
            toTarget = Vector3Subtract(target, position);
            toTarget.y = 0.0f;
            dist = Vector3Length(toTarget);
            
            // End condition: time AND close enough
            bool timeDone = (stateTimer >= harpoonDuration);
            bool closeEnough = (dist <= stopDist + 5.0f);

            if (timeDone && closeEnough)
            {
                currentWorldPath.clear();
                ChangeState(CharacterState::Stagger);
                break;
            }

            break;
        }


        case CharacterState::Freeze: {
            //do nothing
            if (currentHealth <= 0 && !isDead){
                ChangeState(CharacterState::Death); //check for death to freeze, freeze damage doesn't call takeDamage
                break;
            }

            if (stateTimer > 5.0f && !isDead){
                ChangeState(CharacterState::Idle);
                break;
            }
            
            break;
        }


        case CharacterState::Stagger: {
            //do nothing
           
            if (stateTimer > 0.9f) {
                canBleed = true;
                if (distanceSq > PIRATE_MELEE_ENTER){
                    ChangeState(CharacterState::Chase);
                }else{
                    ChangeState(CharacterState::MeleeAttack);
                }
                
                
            }
            break;
        }

        case CharacterState::Death:
            if (!isDead) {
                isDead = true;
                deathTimer = 0.0f;         // Start counting
            }

            deathTimer += deltaTime;
            break;
        }
}

bool Character::FindRepositionTarget(const Player& player, const Vector3& selfPos, Vector3& outTarget) {
    //surround the player, don't all stand on the same tile. 
    Vector2 playerTile = WorldToImageCoords(player.position);

    // Get facing direction
    float playerYaw = player.rotation.y;
    Vector3 forward = Vector3Normalize({ sinf(DEG2RAD * playerYaw), 0.0f, cosf(DEG2RAD * playerYaw) });
    Vector3 right = Vector3Normalize(Vector3CrossProduct({ 0, 1, 0 }, forward));

    // Generate offsets
    Vector2 relativeOffsets[3];
    relativeOffsets[0] = WorldToImageCoords(Vector3Add(player.position, Vector3Scale(forward, tileSize))) - playerTile;
    relativeOffsets[1] = WorldToImageCoords(Vector3Add(player.position, Vector3Scale(right, tileSize))) - playerTile;
    relativeOffsets[2] = WorldToImageCoords(Vector3Add(player.position, Vector3Scale(Vector3Negate(right), tileSize))) - playerTile;
    //relativeOffsets[3] = WorldToImageCoords(Vector3Add(player.position, Vector3Scale(Vector3Negate(forward), tileSize))) - playerTile;
    //don't go behind the player theoretically. 

    for (int i = 0; i < 3; ++i) {
        int tx = (int)playerTile.x + (int)roundf(relativeOffsets[i].x);
        int ty = (int)playerTile.y + (int)roundf(relativeOffsets[i].y);

        if (tx < 0 || ty < 0 || tx >= dungeonWidth || ty >= dungeonHeight) continue;
        if (!IsWalkable(tx, ty, dungeonImg)) continue;
        if (IsTileOccupied(tx, ty, enemyPtrs, nullptr)) continue;

        outTarget = GetDungeonWorldPos(tx, ty, tileSize, dungeonPlayerHeight);
        outTarget.y += 80.0f;
        return true;
    }

    return false;
}

// General path builder to an arbitrary goal
bool Character::BuildPathTo(const Vector3& goalWorld)
{
    navPath.clear();
    navPathIndex = -1;
    navHasPath   = false;

    if (!hasIslandNav) return false;

    std::vector<Vector3> path;
    bool ok = HeightmapPathfinding::FindPathBFS(
        gIslandNav,
        position,      // start from our current position
        goalWorld,     // arbitrary goal (fleeTarget)
        terrainScale.y,
        path
    );

    if (!ok || path.empty())
    {
        return false;
    }

    navPath      = std::move(path);
    navPathIndex = 0;
    navHasPath   = true;
    navRepathTimer = 0.0f;
    return true;
}

// Pick a local patrol point around THIS raptor, snapped to nav grid
bool Character::ChoosePatrolTarget()
{
    hasPatrolTarget = false;
    navHasPath      = false;
    navPath.clear();
    navPathIndex    = -1;

    // If no nav grid (some debug mode), fall back to old behavior
    if (hasIslandNav)
    {
        patrolTarget    = RandomPointOnRingXZ(position, 800.0f, 2200.0f);
        patrolTarget.y  = position.y;
        hasPatrolTarget = true;
        return false; // no path
    }

    const int MAX_TRIES = 6;

    for (int i = 0; i < MAX_TRIES; ++i)
    {
        // Old logic: random ring around self
        Vector3 candidate = RandomPointOnRingXZ(position, 800.0f, 2200.0f);

        int gx, gz;
        gIslandNav.WorldToCell(candidate, gx, gz);
        if (!gIslandNav.IsWalkable(gx, gz))
            continue; // landed in water / cliff, try again

        // Snap to center of that nav cell
        Vector3 snapped = gIslandNav.CellToWorldCenter(gx, gz, terrainScale.y);
        snapped.y = position.y;  // keep visual height consistent

        // Try to build a path to that snapped patrol point
        if (BuildPathTo(snapped))
        {
            patrolTarget    = snapped;
            hasPatrolTarget = true;
            return true; // we have a path + target
        }
    }

    // Couldn’t find a pathable patrol spot: fall back to “dumb” ring point
    patrolTarget    = RandomPointOnRingXZ(position, 800.0f, 2200.0f);

    patrolTarget.y  = position.y;
    hasPatrolTarget = true;
    return false;
}

// Pick a flee target on a ring around the player, snapped to nav grid & pathable
bool Character::ChooseFleeTarget()
{
    hasFleeTarget = false;
    navHasPath    = false;
    navPath.clear();
    navPathIndex  = -1;

    if (!hasIslandNav)
    {
        // fall back to old behavior: just pick any ring point (no nav guarantee)
        fleeTarget = RandomPointOnRingXZ(player.position, 1500.0f, 4000.0f);
        fleeTarget.y = position.y;
        hasFleeTarget = true;
        return false; // no path
    }

    // Try a few random candidates on the ring
    const int MAX_TRIES = 10;

    for (int i = 0; i < MAX_TRIES; ++i)
    {
        Vector3 candidate = RandomPointOnRingXZ(player.position, 1500.0f, 4000.0f);

        // Map to grid cell
        int gx, gz;
        gIslandNav.WorldToCell(candidate, gx, gz);

        if (!gIslandNav.IsWalkable(gx, gz))
            continue; // land in water or cliff, skip

        // Snap to cell center so goal is *exactly* on nav grid
        Vector3 snapped = gIslandNav.CellToWorldCenter(gx, gz, terrainScale.y);

        // Keep Y near our current ground level for rendering consistency
        snapped.y = position.y;

        // Try to build a path to this snapped point
        if (BuildPathTo(snapped))
        {
            fleeTarget   = snapped;
            hasFleeTarget = true;
            return true; // we have a path
        }
    }

    // Could not find a pathable flee target; fall back to old style fleeTarget
    fleeTarget = RandomPointOnRingXZ(player.position, 1500.0f, 4000.0f);
    fleeTarget.y = position.y;
    hasFleeTarget = true;
    return false;
}

Vector3 Character::ComputeRepulsionForce(const std::vector<Character*>& allRaptors, float repulsionRadius, float repulsionStrength)
{
    //return only XZ repulsion. Y stays 0
    Vector3 repulsion = { 0.0f, 0.0f, 0.0f };

    const float radius   = repulsionRadius;
    const float radiusSq = radius * radius;
    const float minDist  = 1.0f;
    const float minDistSq = minDist * minDist;

    for (Character* other : allRaptors) {
        if (other == this) continue;
        if (other->isDead) continue;
        //if (type == CharacterType::Zombie && other->type == CharacterType::Pirate) continue; // zombies aren't repulsed by pirates.

        // Horizontal separation only (XZ plane)
        Vector3 delta = {
            position.x - other->position.x,
            0.0f,
            position.z - other->position.z
        };

        float distSq = delta.x * delta.x + delta.z * delta.z;
        if (distSq < minDistSq || distSq > radiusSq) {
            continue;
        }

        float dist = sqrtf(distSq);
        if (dist > 0.0001f) {
            // Normalized horizontal direction
            Vector3 away = {
                delta.x / dist,
                0.0f,
                delta.z / dist
            };

            // Stronger repulsion the closer they are
            float falloff = (radius - dist) / radius; // 0..1

            repulsion.x += away.x * (falloff * repulsionStrength);
            repulsion.z += away.z * (falloff * repulsionStrength);
        }
    }

    return repulsion; // Y is guaranteed 0
}


void Character::AlertNearbySkeletons(Vector3 alertOrigin, float radius) {
    //*nearby enemies
    Vector2 originTile = WorldToImageCoords(alertOrigin);

    for (Character& other : enemies) {
        if (&other == this) continue; // Don't alert yourself
        if (other.isDead || other.state == CharacterState::Chase) continue;

        float distSqr = Vector3DistanceSqr(alertOrigin, other.position);
        if (distSqr > radius * radius) continue;

        Vector2 targetTile = WorldToImageCoords(other.position);
        if (!LineOfSightRaycast(originTile, targetTile, dungeonImg, 60, 0.0f)) continue;

        // Passed all checks → alert the skeleton
        other.ChangeState(CharacterState::Chase); //chase player regardless of LOS

    }
}



void Character::SetPath(Vector2 start)
{
    // 1) Find tile path (same as before)
    Vector2 goal = WorldToImageCoords(player.position);

    std::vector<Vector2> tilePath = FindPath(start, goal);
    if (type == CharacterType::Bat){
        tilePath.clear();
        tilePath = FindPath(walkableBat, start, goal);
    } 
    std::vector<Vector2> smoothTiles = SmoothTilePath(tilePath, dungeonImg);
    // 2) Convert tile centers to world points (y based on type)
    std::vector<Vector3> worldPath;
    worldPath.reserve(smoothTiles.size());
    for (const Vector2& tile : smoothTiles) {
        Vector3 wp = GetDungeonWorldPos(tile.x, tile.y, tileSize, dungeonEnemyHeight);
        worldPath.push_back(wp);
    }

    // 3) Smooth in world space using your LOS
    //std::vector<Vector3> smoothed = SmoothWorldPath(worldPath);

    // 4) Store
    currentWorldPath = std::move(worldPath);
}

//move with repulsion
bool Character::MoveAlongPath(std::vector<Vector3>& path,
                              Vector3& pos, float& yawDeg,
                              float speed, float deltaTime,
                              float arriveEps,
                              Vector3 repulsion)   // <-- new, optional
{
    if (path.empty()) return false;

    const Vector3 target = path.front();
    Vector3 delta = Vector3Subtract(target, pos);
    const float dist = sqrtf(delta.x*delta.x + delta.y*delta.y + delta.z*delta.z);
    if (dist <= arriveEps) {
        path.erase(path.begin());
        return path.empty();
    }

    const float step = fminf(speed * deltaTime, dist);
    Vector3 dir = { delta.x / dist, delta.y / dist, delta.z / dist };

    // Forward move toward waypoint
    Vector3 moveFwd = Vector3Scale(dir, step);

    // repulsion (scaled by dt so it's “units per second”)
    Vector3 moveRep = Vector3Scale(repulsion, deltaTime);

    // Apply both
    Vector3 move = Vector3Add(moveFwd, moveRep);
    pos = Vector3Add(pos, move);

    // Yaw from actual motion if we had any; fallback to forward dir
    const float mxz2 = move.x*move.x + move.z*move.z;
    if (mxz2 > 1e-4f) {
        yawDeg = RAD2DEG * atan2f(move.x, move.z);
    } else {
        yawDeg = RAD2DEG * atan2f(dir.x, dir.z);
    }

    //Don't snap bats to target.y, they bob up and down in update
    if (type != CharacterType::Bat) pos.y = target.y;

    return false;
}

void Character::SetPathTo(const Vector3& goalWorld) {
    Vector2 start = WorldToImageCoords(position);
    Vector2 goal  = WorldToImageCoords(goalWorld);

    std::vector<Vector2> tilePath = SmoothTilePath(FindPath(start, goal), dungeonImg);

    currentWorldPath.clear();
    currentWorldPath.reserve(tilePath.size());

    for (const Vector2& tile : tilePath) {
        Vector3 worldPos = GetDungeonWorldPos(tile.x, tile.y, tileSize, dungeonEnemyHeight);
        currentWorldPath.push_back(worldPos);
    }

}

// Call this for raptors/Trex (overworld)
void Character::UpdateRaptorVisibility(const Player& player, float deltaTime) {
    if (player.godMode){
        canSee = false;
        playerVisible = false;
        return;
    }

    const float VISION_ENTER = 4000.0f; 
    const float VISION_EXIT  = 4200.0f;  // deadband so it doesn’t flicker at the edge
    const float WATER_LEVEL  = 65.0f;

    const float d = Vector3Distance(position, player.position);
    const bool playerInWater = IsWaterAtXZ(player.position.x, player.position.z, WATER_LEVEL);
    
    // Engageable == in range AND not in water
    bool inRange = (playerVisible ? d < VISION_EXIT : d < VISION_ENTER);
    canSee = inRange && !playerInWater;   // instantaneous gate for actions

    if (canSee) {
        playerVisible = true;
        lastKnownPlayerPos = player.position;
        timeSinceLastSeen = 0.0f;
    } else {
        timeSinceLastSeen += deltaTime;
        if (timeSinceLastSeen > forgetTime) playerVisible = false;
    }
}

const float HOVER_ALT   = 200.0f;
const float PATROL_ALT  = 1200.0f;
const float FLEE_ALT    = 2200.0f;
const float DIVE_ALT    = -500.0f;


void Character::UpdateChaseSteering(float dt)
{
    // --- tuning ---
    const float CHASE_SPEED        = raptorSpeed * 2.00f;  // faster than patrol
    const float MAX_ACCEL          = CHASE_SPEED * 10.0f;   // snappy “missile”
    const float SLOW_RADIUS        = 1000.0f;               // soften near player
    const float ATTACK_ENTER_XZ    = 200.0f;               // switch to attack here
    const float GIVE_UP_DISTANCE   = 4500.0f;              // optional: fall back

    float feetY = player.position.y - player.height / 2.0f;
    Vector3 playerFeet = {player.position.x, feetY, player.position.z};
    // XZ-only target (Y handled elsewhere)
    Vector3 target = playerFeet;
    target.y = position.y;

    // Distance to player on XZ
    float d = DistXZ(position, target);

    float desiredAlt = PATROL_ALT;
    if (d < 2500.0f) {
        float t = Clamp((d - 200.0f) / (2500.0f - 200.0f), 0.0f, 1.0f); // 0 close, 1 far
        desiredAlt = Lerp(DIVE_ALT, PATROL_ALT, t);
    }

    float groundY = GetHeightAtWorldPosition(position, heightmap, terrainScale);

    UpdateAltitude(dt, groundY, desiredAlt);
    float distance = Vector3Distance(position, player.position);
    // Close enough → attack
    if (distance <= ATTACK_ENTER_XZ)
    {
        velocity = {0,0,0}; // optional: prevents “slide-through” on state change
        ChangeState(CharacterState::Attack);
        return;
    }

    // Optional: if player is super far, give up
    if (d > GIVE_UP_DISTANCE)
    {
        ChangeState(CharacterState::Patrol);
        return;
    }


    // --- desired velocity (arrive-ish toward player) ---
    Vector3 toT = Vector3Subtract(target, position);
    toT.y = 0.0f;

    Vector3 desiredVel = {0,0,0};
    if (d > 1e-4f)
    {
        float speed = CHASE_SPEED;

        // slow down as we approach so we don’t orbit
        if (d < SLOW_RADIUS)
            speed *= (d / SLOW_RADIUS);

        desiredVel = Vector3Scale({toT.x / d, 0.0f, toT.z / d}, speed);
    }

    // --- steering accel toward desired velocity ---
    Vector3 accel = Vector3Subtract(desiredVel, velocity);
    accel.y = 0.0f;
    accel = ClampXZ(accel, MAX_ACCEL);

    // integrate
    velocity = Vector3Add(velocity, Vector3Scale(accel, dt));
    velocity = ClampXZ(velocity, CHASE_SPEED);

    position = Vector3Add(position, Vector3Scale(velocity, dt));

    // face travel direction
    if (velocity.x*velocity.x + velocity.z*velocity.z > 1e-4f)
        rotationY = RAD2DEG * atan2f(velocity.x, velocity.z);
}


void Character::UpdateChase(float deltaTime)
{
    // update raptor/trex chase state. 
    float ATTACK_ENTER  = 200.0f;   // start attack if closer than this
    if (type == CharacterType::Trex) ATTACK_ENTER = 500; // maybe too far

    const float VISION_ENTER = 5000.0f;
    float distance = Vector3Distance(position, player.position);

    if (!canSee) { ChangeState(CharacterState::Idle); return; }
    

    if (stateTimer > chaseDuration && type != CharacterType::Trex) { ChangeState(CharacterState::RunAway); return; }
    if (distance < ATTACK_ENTER) { ChangeState(CharacterState::Attack); return; }
    if (distance > VISION_ENTER) { ChangeState(CharacterState::Idle); return; }

    // --- NEW: build or occasionally rebuild path when entering/while in chase ---
    navRepathTimer += deltaTime;

    // First frame of chase, or we’ve lost our path
    if (!navHasPath && navRepathTimer <= deltaTime)
    {
        BuildPathToPlayer();
    }
    else if (navHasPath && navRepathTimer >= NAV_REPATH_INTERVAL)
    {
        // Optional: periodically refresh path toward the moving player
        BuildPathToPlayer();
    }

    float MAX_SPEED = 700.0f;  // per-type speed 700
    if (type == CharacterType::Raptor) MAX_SPEED = 1200.0f; // double fast raptors.

    const float SLOW_RADIUS = 400.0f;       // ease-in so we don’t overshoot

    Vector3 targetPos = player.position;

    // If we have a nav path, chase the current waypoint instead of the raw player pos
    if (navHasPath && navPathIndex >= 0 && navPathIndex < (int)navPath.size())
    {
        targetPos = navPath[navPathIndex];
        
        //Only check XZ distance not Y. T-Rex is taller by like 100, so it messed up the distance calculation.
        float waypointDist = DistXZ(position, targetPos); 
        const float WAYPOINT_REACH_RADIUS = 150.0f; 

        if (waypointDist < WAYPOINT_REACH_RADIUS)
        {
            
            navPathIndex++;
            
            if (navPathIndex >= (int)navPath.size())
            {
                
                // Reached end of path; switch to direct player chase
                navHasPath   = false;
                navPathIndex = -1;
                targetPos    = player.position;
            }
            else
            {
                targetPos = navPath[navPathIndex];
            }
        }
    }
    else
    {
        // Pathless fallback: direct chase as before
        navHasPath   = false;
        navPathIndex = -1;
        targetPos    = player.position;
        
    }

    // Move toward targetPos (either player or current waypoint), easing inside SLOW_RADIUS
    Vector3 vel = ArriveXZ(position, targetPos, MAX_SPEED, SLOW_RADIUS);

    // Safety: still obey water edge logic (nav grid *should* avoid water, but no harm)
    bool blocked = StopAtWaterEdge(position, vel, 65, deltaTime);

    Vector3 repel = ComputeRepulsionForce(enemyPtrs, 300, 800.0f);

    if (!blocked)
    {
        
        position = Vector3Add(position, Vector3Scale(vel + repel, deltaTime));
    }
    else
    {
        // You *could* trigger a repath here instead of running away if you want:
        // BuildPathToPlayer();
        
        ChangeState(CharacterState::RunAway);
    }

    if (vel.x*vel.x + vel.z*vel.z > 1e-4f) {
        rotationY = RAD2DEG * atan2f(vel.x, vel.z);
    }

}

static inline Vector3 PickFleeTargetXZ(
    const Vector3& enemyPos,
    const Vector3& playerPos,
    float minR,
    float maxR,
    const Image& heightmap,
    float terrainScale,
    float edgeMargin = 0.0f
){
    // Base direction away from player
    Vector3 away = Vector3Subtract(enemyPos, playerPos);
    away.y = 0.0f;

    float len = sqrtf(away.x*away.x + away.z*away.z);
    if (len < 1e-4f) {
        // degenerate: pick random direction
        float a = GetRandomValue(0, 360) * DEG2RAD;
        away = { cosf(a), 0.0f, sinf(a) };
    } else {
        away.x /= len; away.z /= len;
    }

    // Radius (uniform by area)
    float t = (float)GetRandomValue(0, 10000) / 10000.0f;
    float r = sqrtf(minR*minR + (maxR*maxR - minR*minR) * t);

    // Add some angular noise so they don't all flee in a straight line
    float jitter = ((float)GetRandomValue(-1000, 1000) / 1000.0f) * 0.75f; // radians-ish
    float s = sinf(jitter), c = cosf(jitter);

    // rotate away vector by jitter
    Vector3 dir = { away.x*c - away.z*s, 0.0f, away.x*s + away.z*c };

    Vector3 p = enemyPos;
    p.x += dir.x * r;
    p.z += dir.z * r;
    p.y = enemyPos.y;

    // Clamp to world bounds
    float worldW = (heightmap.width  - 1) * terrainScale;
    float worldD = (heightmap.height - 1) * terrainScale;

    p.x = Clamp(p.x, edgeMargin, worldW - edgeMargin);
    p.z = Clamp(p.z, edgeMargin, worldD - edgeMargin);

    return p;
}

void Character::UpdateRunawaySteering(float dt)
{
    // --- knobs ---
    const float FLEE_SPEED        = raptorSpeed * 2.0f; // a bit faster than chase
    const float MAX_ACCEL         = FLEE_SPEED * 5.0f;   // snappy getaway
    const float SLOW_RADIUS       = 200.0f;              // optional soften near target
    const float ARRIVE_EPS_XZ     = 250.0f;

    const float FLEE_MIN_TIME     = 5.0f;
    const float FLEE_MAX_TIME     = 10.0f;
    const float FLEE_EXIT_DIST    = 4000.0f;

    // --- choose / refresh flee target ---
    // Refresh if we don't have one, or we reached it, or target is too close to player.
    const float TARGET_REACHED = ARRIVE_EPS_XZ;
    const float TARGET_TOO_CLOSE_TO_PLAYER = 900.0f;

    if (!hasFleeTarget ||
        DistXZ(position, fleeTarget) < TARGET_REACHED ||
        DistXZ(player.position, fleeTarget) < TARGET_TOO_CLOSE_TO_PLAYER)
    {
        fleeTarget = PickFleeTargetXZ(
            position, player.position,
            /*minR*/ 2000.0f,
            /*maxR*/ 4500.0f,
            heightmap, terrainScale.x,
            /*edgeMargin*/ 400.0f
        );
        hasFleeTarget = true;
    }

    // --- steer toward fleeTarget (ignore player) ---
    Vector3 toT = Vector3Subtract(fleeTarget, position);
    toT.y = 0.0f;

    float d = sqrtf(toT.x*toT.x + toT.z*toT.z);

    Vector3 desiredVel = {0,0,0};
    if (d > 1e-4f)
    {
        float speed = FLEE_SPEED;
        if (d < SLOW_RADIUS) speed *= (d / SLOW_RADIUS); // optional

        desiredVel = Vector3Scale({toT.x / d, 0.0f, toT.z / d}, speed);
    }



    Vector3 accel = Vector3Subtract(desiredVel, velocity);
    accel.y = 0.0f;
    accel = ClampXZ(accel, MAX_ACCEL);

    velocity = Vector3Add(velocity, Vector3Scale(accel, dt));
    velocity = ClampXZ(velocity, FLEE_SPEED);

    Vector3 nextPos = Vector3Add(position, Vector3Scale(velocity, dt));

    position = nextPos;

    // Face travel direction
    if (velocity.x*velocity.x + velocity.z*velocity.z > 1e-4f)
        rotationY = RAD2DEG * atan2f(velocity.x, velocity.z);

    // --- exit conditions ---
    float distFromPlayer = DistXZ(position, player.position);

    //--- altitude ---
    float groundY = GetHeightAtWorldPosition(position, heightmap, terrainScale);
    UpdateAltitude(dt, groundY, FLEE_ALT);
    float altT = Clamp((position.y - groundY) / FLEE_ALT, 0.0f, 1.0f);
    float horizScale = (altT < 0.5f) ? 0.5f : 1.0f;
    position += velocity * dt * horizScale;

    if ((stateTimer >= FLEE_MIN_TIME && distFromPlayer >= FLEE_EXIT_DIST) ||
        stateTimer >= FLEE_MAX_TIME)
    {
        hasFleeTarget = false;
        velocity = {0,0,0};

        // After fleeing, decide what to do
        ChangeState(CharacterState::Idle);

        return;
    }
}





void Character::UpdateTrexStepSFX(float dt)
{
    if (state != CharacterState::Chase) {  // only step in Chase
        stepTimer = 0.0f;                   // reset when not chasing
        return;
    }

    stepTimer += dt;
    if (stepTimer >= 1.0f) {

        SoundManager::GetInstance().PlaySoundAtPosition("TrexStep", position, player.position, 0.0f, 8000.0f);
        stepTimer -= 1.0f; // use -= to survive occasional long frames
    }
}

void Character::UpdateRunaway(float deltaTime)
{
    // Distance from player (for exit logic)
    float distance = DistXZ(position, player.position);

    // --- simple knobs ---
    const float MAX_SPEED      = 1000.0f;
    const float FLEE_MIN_TIME  = 5.0f;
    const float FLEE_MAX_TIME  = 10.0f;
    const float FLEE_EXIT      = 4000.0f;
    const float SEP_CAP        = 200.0f;

    const float TARGET_REACHED_RADIUS = 300.0f;
    const float WAYPOINT_REACHED_RADIUS = 200.0f;

    // --- choose / refresh flee target ---

    if (!hasFleeTarget ||
        DistXZ(position, fleeTarget) < TARGET_REACHED_RADIUS ||
        DistXZ(player.position, fleeTarget) < 600.0f)
    {
        // This will also try to build a nav path to the flee target
        ChooseFleeTarget();
    }

    // --- movement toward flee target (possibly via nav path) ---

    Vector3 targetPos = fleeTarget;

    // If we have a nav path (best case), walk the waypoints
    if (navHasPath && navPathIndex >= 0 && navPathIndex < (int)navPath.size())
    {
        targetPos = navPath[navPathIndex];

        float dToWp = DistXZ(position, targetPos);
        if (dToWp < WAYPOINT_REACHED_RADIUS)
        {
            navPathIndex++;
            if (navPathIndex >= (int)navPath.size())
            {
                // Reached the end of the path; final target is fleeTarget
                navHasPath   = false;
                navPathIndex = -1;
                targetPos    = fleeTarget;
            }
            else
            {
                targetPos = navPath[navPathIndex];
            }
        }
    }
    else
    {
        // No nav path (either none built or failed): we still flee directly
        navHasPath   = false;
        navPathIndex = -1;
        targetPos    = fleeTarget;
    }

    // --- steering toward targetPos ---

    Vector3 desired = { 0, 0, 0 };

    // Seek targetPos on XZ
    {
        Vector3 toTarget = {
            targetPos.x - position.x,
            0.0f,
            targetPos.z - position.z
        };

        float dist = sqrtf(toTarget.x*toTarget.x + toTarget.z*toTarget.z);

        if (dist > 1.0f) {
            float inv = 1.0f / dist;
            toTarget.x *= inv;
            toTarget.z *= inv;

            desired.x = toTarget.x * MAX_SPEED;
            desired.z = toTarget.z * MAX_SPEED;
        }
    }

    // Separation: avoid stacking with other raptors
    Vector3 vSep = ComputeRepulsionForce(enemyPtrs, /*radius*/500.0f, /*strength*/600.0f);
    vSep = Limit(vSep, SEP_CAP);

    desired = Vector3Add(desired, vSep);
    desired = Limit(desired, MAX_SPEED);

    // Don’t run into water (belt-and-suspenders: nav should already avoid it)
    bool blocked = StopAtWaterEdge(position, desired, 65, deltaTime);
    if (blocked) {
        // water edge killed motion; drop flee target so we pick a new one next frame
        hasFleeTarget = false;
        navHasPath    = false;
        navPath.clear();
        navPathIndex = -1;

        ChangeState(CharacterState::Idle);
        return;
    }

    // Integrate motion
    position = Vector3Add(position, Vector3Scale(desired, deltaTime));

    // Face direction of travel (XZ only)
    if (desired.x*desired.x + desired.z*desired.z > 1e-4f) {
        rotationY = RAD2DEG * atan2f(desired.x, desired.z);
    }

    // --- exit conditions (same as before) ---
    if ((distance > FLEE_EXIT && stateTimer >= FLEE_MIN_TIME) ||
        stateTimer >= FLEE_MAX_TIME)
    {
        hasFleeTarget = false;  // so next time we enter Runaway we re-roll
        navHasPath    = false;
        navPath.clear();
        navPathIndex = -1;

        if (canSee) {
            ChangeState(CharacterState::Chase);
        } else {
            ChangeState(CharacterState::Idle);
        }
        return;
    }
}


void Character::UpdatePatrolSteering(float dt)
{
    // Pick a new target if we don’t have one
    if (!hasPatrolTarget)
    {
        // Use your new ring picker (no nav path)
        patrolTarget = RandomPointOnHeightmapRingXZ(position, 3000.0f, 8200.0f,
                                           heightmap.width, terrainScale.x, 400.0f);
        hasPatrolTarget = true;

        float maxDist = 10000;
        float distanceToOrigin = Vector3Length(position);
        if (distanceToOrigin > maxDist){
            patrolTarget = Vector3{0}; //if you go too far from center, return to center of the map. 
        }


    }

    const float MAX_SPEED    = raptorSpeed;
    const float MAX_ACCEL    = MAX_SPEED * 3.0f;   // snappy missile feel
    const float ARRIVE_EPS   = 150.0f;             // consider “arrived”
    const float SLOW_RADIUS  = 600.0f;             // optional soften near target
    const float VISION_ENTER = 4000.0f;

    // --- desired velocity (seek or arrive) ---
    float groundY = GetHeightAtWorldPosition(position, heightmap, terrainScale);


    UpdateAltitude(dt, groundY, patrolAlt);

    float currentAlt = position.y - groundY;

    // If still climbing, limit horizontal movement so it “takes off” first
    float altT = Clamp(currentAlt / PATROL_ALT, 0.0f, 1.0f);
    float horizScale = (altT < 0.6f) ? 0.25f : 1.0f;  // climb phase = slow XZ

    // XZ steering (same as you already have)
    Vector3 desiredVel = ArriveXZ(position, patrolTarget, /*maxSpeed*/raptorSpeed*0.6f, /*slow*/400.0f);
    Vector3 accel = Vector3Subtract(desiredVel, velocity);
    accel.y = 0.0f;
    accel = ClampXZ(accel, /*maxAccel*/(raptorSpeed*0.6f)*3.0f);

    velocity = Vector3Add(velocity, Vector3Scale(accel, dt));
    velocity = ClampXZ(velocity, raptorSpeed*0.6f);

    // Apply the gating scale
    position = Vector3Add(position, Vector3Scale(velocity, dt * horizScale));

    // Face travel direction
    if (velocity.x*velocity.x + velocity.z*velocity.z > 1e-4f)
        rotationY = RAD2DEG * atan2f(velocity.x, velocity.z);

    if (Vector3Distance(position, player.position) <= VISION_ENTER){
        ChangeState(CharacterState::Chase);
        return;
    }

    // Arrived → idle
    if (DistXZ(position, patrolTarget) <= ARRIVE_EPS)
    {
        hasPatrolTarget = false;
        velocity = {0,0,0};
        ChangeState(CharacterState::Idle);
        return;
    }


}



void Character::UpdatePatrol(float deltaTime)
{
    // --- choose / refresh patrol target ---
    if (!hasPatrolTarget)
    {
        // This will also try to build a nav path to that target
        ChoosePatrolTarget();
    }

    const float PATROL_SPEED        = raptorSpeed * 0.6f;
    const float PATROL_SLOW_RAD     = 400.0f;
    const float ARRIVE_EPS_XZ       = 150.0f;
    const float WAYPOINT_REACH_RAD  = 200.0f;

    // Decide what we’re actually steering toward this frame
    Vector3 targetPos = patrolTarget;

    if (navHasPath && navPathIndex >= 0 && navPathIndex < (int)navPath.size())
    {
        targetPos = navPath[navPathIndex];

        float dToWp = DistXZ(position, targetPos);
        if (dToWp < WAYPOINT_REACH_RAD)
        {
            navPathIndex++;
            if (navPathIndex >= (int)navPath.size())
            {
                // Done with path; final approach to patrolTarget
                navHasPath   = false;
                navPathIndex = -1;
                targetPos    = patrolTarget;
            }
            else
            {
                targetPos = navPath[navPathIndex];
            }
        }
    }
    else
    {
        // No path available: just walk straight toward patrolTarget (old behavior)
        navHasPath   = false;
        navPathIndex = -1;
        targetPos    = patrolTarget;
    }

    // Movement toward targetPos
    Vector3 vel = ArriveXZ(position, targetPos, PATROL_SPEED, PATROL_SLOW_RAD);
    position    = Vector3Add(position, Vector3Scale(vel, deltaTime));

    // Stop at water edge → flee (belt-and-suspenders on top of nav)
    if (StopAtWaterEdge(position, vel, 65, deltaTime)) {
        hasPatrolTarget = false;
        navHasPath      = false;
        navPath.clear();
        navPathIndex = -1;
        ChangeState(CharacterState::RunAway);
        return;
    }

    // Face travel direction
    if (vel.x*vel.x + vel.z*vel.z > 1e-4f) {
        rotationY = RAD2DEG * atan2f(vel.x, vel.z);
    }

    // Arrived at patrolTarget → idle
    if (DistXZ(position, patrolTarget) <= ARRIVE_EPS_XZ) {
        hasPatrolTarget = false;
        navHasPath      = false;
        navPath.clear();
        navPathIndex = -1;
        ChangeState(CharacterState::Idle);
        return;
    }

    // Player seen → chase
    const float STALK_ENTER = 2000.0f;
    if (playerVisible && DistXZ(position, player.position) < STALK_ENTER) {
        hasPatrolTarget = false;
        navHasPath      = false;
        navPath.clear();
        navPathIndex = -1;

        if (canSee) ChangeState(CharacterState::Chase); // cant see if player is in water
        return;
    }
}

