#include "character.h"
#include "raylib.h"
#include "raymath.h"
#include "dungeonGeneration.h"
#include "world.h"
#include "pathfinding.h"
#include "sound_manager.h"
#include "resourceManager.h"
#include "utilities.h"

void Character::UpdateAI(float deltaTime, Player& player) {
    switch (type) {
        case CharacterType::Raptor:
            UpdateRaptorAI(deltaTime, player);
            break;

        case CharacterType::Trex:
            UpdateTrexAI(deltaTime, player);
            break;

        case CharacterType::Skeleton:
            UpdateSkeletonAI(deltaTime, player);
            break;

        case CharacterType::Pirate:
            UpdatePirateAI(deltaTime, player);
            
            break;

        case CharacterType::Spider:
            UpdateSkeletonAI(deltaTime, player); //spider uses same code as skeleton
            break;

        case CharacterType::GiantSpider:
            UpdateGiantSpiderAI(deltaTime, player);
            break;

        case CharacterType::Ghost:
            UpdateSkeletonAI(deltaTime, player);
            break;
    }
}

void Character::UpdatePlayerVisibility(const Vector3& playerPos, float deltaTime, float epsilon) {
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
    float distance = Vector3Distance(position, player.position);

    Vector2 start = WorldToImageCoords(position);
    //Vector2 goal = WorldToImageCoords(player.position);
    //changed vision to soley rely on world LOS. More forgiving 
    playerVisible = false;
    UpdatePlayerVisibility(player.position, deltaTime, 0.0f);

 
    switch (state){
        case CharacterState::Idle: {
            stateTimer += deltaTime;
 
            Vector2 start = WorldToImageCoords(position);

            // Transition to chase if player detected
            if (distance < 4000.0f && stateTimer > 1.0f && playerVisible) {
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

            stateTimer += deltaTime;
            pathCooldownTimer = std::max(0.0f, pathCooldownTimer - deltaTime);

            if (distance < 200.0f && canSee) {
                ChangeState(CharacterState::Attack);

            }
            else if (distance > 4000.0f) {
                ChangeState(CharacterState::Idle);

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

            if (distance > 350.0f) { 
                ChangeState(CharacterState::Chase);

            }

            attackCooldown -= deltaTime;
            if (attackCooldown <= 0.0f && currentFrame == 1 && playerVisible) { // make sure you can see what your attacking. 
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

                    SoundManager::GetInstance().Play(rand() % 2 ? "spiderBite2" : "spiderBite1");
                    decals.emplace_back(offsetPos, DecalType::Explosion, R.GetTexture("biteSheet"), 4, 0.4f, 0.1f, 50.0f);

                }
                
            }
            break;
        }


        


      
    }

}

void Character::UpdateSkeletonAI(float deltaTime, Player& player) {
    float distance = Vector3Distance(position, player.position);

    Vector2 start = WorldToImageCoords(position);
    //Vector2 goal = WorldToImageCoords(player.position);
    //changed vision to soley rely on world LOS. More forgiving 
    playerVisible = false;
    UpdatePlayerVisibility(player.position, deltaTime, 0.0f);

 
    switch (state){
        case CharacterState::Idle: {
            stateTimer += deltaTime;
 
            Vector2 start = WorldToImageCoords(position);

            // Transition to chase if player detected
            if (distance < 4000.0f && stateTimer > 1.0f && playerVisible) {
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
            stateTimer += deltaTime;
            pathCooldownTimer = std::max(0.0f, pathCooldownTimer - deltaTime);

            if (distance < 200.0f && canSee) {
                ChangeState(CharacterState::Attack);

            }
            else if (distance > 4000.0f) {
                ChangeState(CharacterState::Idle);

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

            if (distance > 350.0f) { 
                ChangeState(CharacterState::Chase);

            }

            attackCooldown -= deltaTime;
            if (attackCooldown <= 0.0f && currentFrame == 1 && playerVisible) { // make sure you can see what your attacking. 
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
            stateTimer += deltaTime;

            Vector2 playerTile = WorldToImageCoords(player.position);
            Vector3 target = position; // fallback

            bool foundSpot = FindRepositionTarget(player, position, target);

            if (foundSpot) {
                Vector3 dir = Vector3Normalize(Vector3Subtract(target, position));
                Vector3 move = Vector3Scale(dir, skeleSpeed * deltaTime);
                position = Vector3Add(position, move);

                rotationY = RAD2DEG * atan2f(dir.x, dir.z);
                position.y = target.y;

                float dist = Vector3Distance(position, target);

                if (dist < 300.0f && stateTimer > 1.0f) {
                    ChangeState(CharacterState::Attack);
                } else if (dist > 350.0f && stateTimer > 1.0f) {
                    ChangeState(CharacterState::Chase);
                }
            }

            break;
        }



        case CharacterState::Patrol: {
            stateTimer += deltaTime;

            if (distance < 4000.0f && playerVisible){
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
            stateTimer += deltaTime;
            //do nothing
            if (currentHealth <= 0){ //hopefully prevents invincible skeles. 
                ChangeState(CharacterState::Death);
            }

            if (stateTimer > 5.0f && !isDead){
                ChangeState(CharacterState::Idle);
            }
            break;

        }


        case CharacterState::Stagger: {
            stateTimer += deltaTime;
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

// Raptor = overworld, no grid pathing.
// Skeleton only (structure + thresholds). Fill TODOs as you add steering.

void Character::UpdateTrexAI(float deltaTime, Player& player){
    stateTimer += deltaTime;
    attackCooldown  = std::max(0.0f, attackCooldown - deltaTime);

    const float distance = Vector3Distance(position, player.position);
    UpdateRaptorVisibility(player, deltaTime);

    // --- Simple deadbands ---
    const float STALK_ENTER   = 2000.0f;  // engage if closer than this
    const float ATTACK_EXIT   = 800.0f;   // leave attack if beyond this 
    const float FLEE_ENTER    = 100.0f;   // too close -> run away


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

            if (distance < STALK_ENTER && playerVisible) {
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
            if (distance > ATTACK_EXIT) {
                if (canSee) ChangeState(CharacterState::Chase);
                break;
            }

            if (distance < FLEE_ENTER) { ChangeState(CharacterState::RunAway); break;}
            if (attackCooldown <= 0.0f && distance < ATTACK_EXIT && canSee){
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

void Character::UpdateRaptorAI(float deltaTime, Player& player)
{
    // --- Perception & timers ---
    stateTimer     += deltaTime;
    attackCooldown  = std::max(0.0f, attackCooldown - deltaTime);

    const float distance = Vector3Distance(position, player.position);
    UpdateRaptorVisibility(player, deltaTime);

    //UpdateLeavingFlag(player.position);

    // --- Simple deadbands ---
    const float STALK_ENTER   = 3000.0f;  // engage if closer than this
    const float STALK_EXIT    = 3400.0f;  // drop back to idle 
    const float ATTACK_ENTER  = 200.0f;   // start attack if closer than this
    const float ATTACK_EXIT   = 300.0f;   // leave attack if beyond this 
    const float FLEE_ENTER    = 100.0f;   // too close -> run away
    const float FLEE_EXIT     = 3000.0f;   // far enough -> stop fleeing
    const float VISION_ENTER = 4000.0f;

    //playerVisible = distance < VISION_ENTER;

    // --- Placeholder for steering output (fill later) ---
    //Vector3 desiredVel = {0,0,0};   // compute with Seek/Arrive/Orbit/Flee later

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

            if (distance < STALK_ENTER && playerVisible) {
                if (canSee) ChangeState(CharacterState::Chase);
                
                break;
            }
        } break;

        case CharacterState::Patrol:
        {
            UpdateLeavingFlag(player.position);
            UpdatePatrol(deltaTime);
            break;   
        }

        case CharacterState::Chase:
        {
            
            UpdateChase(deltaTime);
          
        } break;

        case CharacterState::Orbit:
{           //we never use this. 

        } break;


        case CharacterState::Attack:

        {

            if (distance > ATTACK_EXIT) {
                if (canSee) ChangeState(CharacterState::Chase);
                break;
            }
            if (distance < FLEE_ENTER) { ChangeState(CharacterState::RunAway); break;}
            if (attackCooldown <= 0.0f && distance < ATTACK_EXIT) {
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
            UpdateLeavingFlag(player.position);
            UpdateRunaway(deltaTime);

            
        } break;


        case CharacterState::Freeze:
        {
            // TODO: do nothing; exit after freezeDuration or on event
            
            if (stateTimer >= 5.0f && canSee && !isDead) ChangeState(CharacterState::Chase);
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


void Character::UpdatePirateAI(float deltaTime, Player& player) {
    if (isLoadingLevel) return;
    constexpr float PIRATE_ATTACK_ENTER = 800.0f; // start attacking when closer than this
    constexpr float PIRATE_ATTACK_EXIT  = 900.0f; // stop attacking when farther than this
    // (EXIT must be > ENTER)

    
    float distance = Vector3Distance(position, player.position);
    float pirateHeight = 160;
    playerVisible = false;
    Vector2 start = WorldToImageCoords(position);
    //Vector2 goal = WorldToImageCoords(player.position);

    UpdatePlayerVisibility(player.position, deltaTime, 0.0f);

 
    switch (state){
        case CharacterState::Idle: {
            stateTimer += deltaTime;

            Vector2 start = WorldToImageCoords(position);

            if (distance < 250 && canSee){
                ChangeState(CharacterState::MeleeAttack);
                break;
            }

            // Transition to chase if player detected
            if (distance < 4000.0f && stateTimer > 1.0f && (playerVisible)) {
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
            stateTimer += deltaTime;
            pathCooldownTimer = std::max(0.0f, pathCooldownTimer - deltaTime);

            if (distance < 250 && canSee){
                ChangeState(CharacterState::MeleeAttack);
                break;
            }

            // 1) Try to attack when close AND we have instant LOS
            if (distance < PIRATE_ATTACK_ENTER && canSee) {
                ChangeState(CharacterState::Attack);
                break;
            }

            // 2) Leash out if too far
            if (distance > 4000.0f) {
                ChangeState(CharacterState::Idle);
                playerVisible = false;         // drop memory when giving up
                currentWorldPath.clear();
                break;
            }

            // 3) Plan path toward current target when cooldown allows
            if (pathCooldownTimer <= 0.0f) {
                if (canSee) {
                    SetPathTo(player.position);
                    pathCooldownTimer = 0.4f;
                } else if (playerVisible) {       // still within memory window
                    SetPathTo(lastKnownPlayerPos);
                    pathCooldownTimer = 0.4f;
                }
            }


            // 4) Advance along path (with repulsion)
            if (!currentWorldPath.empty() && state != CharacterState::Stagger) {
                Vector3 repel = ComputeRepulsionForce(enemyPtrs, 50, 500); // your existing call
                MoveAlongPath(currentWorldPath, position, rotationY, skeleSpeed, deltaTime, 100.0f, repel);

                // Reached the end but still no LOS? stop chasing
                if (currentWorldPath.empty() && !canSee) {
                    playerVisible = false;          // memory expires now that we arrived
                    ChangeState(CharacterState::Idle);
                }
            }
        } break;


        case CharacterState::Attack: { //Pirate attack with gun
            stateTimer += deltaTime;
            
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
            if (distance > PIRATE_ATTACK_EXIT || !canSee) {
                ChangeState(CharacterState::Chase);
                break;
            }

            attackCooldown -= deltaTime;
            if (distance < 800 && distance > 300){
                
                if (canSee && attackCooldown <= 0.0f && currentFrame == 1 && !hasFired && type == CharacterType::Pirate) {
                    FireBullet(position, player.position, 1200.0f, 3.0f, true);
                    hasFired = true;
                    attackCooldown = 1.5f;
                    //SoundManager::GetInstance().Play("shotgun");
                    SoundManager::GetInstance().PlaySoundAtPosition("musket", position, player.position, 1.0, 2000);
                }

            }else if (distance < 250){
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

            // Wait until animation is done to apply damage
            if (stateTimer >= 0.5f && !hasFired) { // 5 frames * 0.12s = 0.6s make it 0.5
                hasFired = true; //reusing hasfired for sword attack. I think this is ok?

                if (distance < 280.0f && playerVisible) {
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
            if (stateTimer >= 1.5f) {
                if (distance > 300.0f) {
                    Vector2 start = WorldToImageCoords(position);
                    if (TrySetRandomPatrolPath(start, this, currentWorldPath)){
                        ChangeState(CharacterState::Patrol);
                    }else{
                        ChangeState(CharacterState::Idle);

                    }

                } else {
                    ChangeState(CharacterState::MeleeAttack);
                }
                hasFired= false;
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
                position.y = pirateHeight;

                float dist = Vector3Distance(position, target);

                if (dist < 300.0f && stateTimer > 2.0f) {
                    ChangeState(CharacterState::MeleeAttack);
                } else if (dist > 350.0f && stateTimer > 2.0f) {
                    ChangeState(CharacterState::Chase);
                }
            }

            break;
        }



        case CharacterState::Patrol: { //Pirate Patrol after every shot. 
            stateTimer += deltaTime;
            //ignore player while patroling to new tile. 
            
            // Advance along path (with repulsion)
            if (!currentWorldPath.empty() && state != CharacterState::Stagger) {
                Vector3 repel = ComputeRepulsionForce(enemyPtrs, 50, 500); // your existing call
                MoveAlongPath(currentWorldPath, position, rotationY, skeleSpeed, deltaTime, 100.0f, repel);
                UpdateLeavingFlag(player.position);
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

        case CharacterState::Freeze: {
            stateTimer += deltaTime;
            //do nothing
            if (currentHealth <= 0){
                ChangeState(CharacterState::Death); //check for death to freeze, freeze damage doesn't call takeDamage
            }

            if (stateTimer > 5.0f && !isDead){
                ChangeState(CharacterState::Idle);
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

Vector3 Character::ComputeRepulsionForce(const std::vector<Character*>& allRaptors, float repulsionRadius, float repulsionStrength) {
    Vector3 repulsion = { 0 };
    //prevent raptors overlapping 
    for (Character* other : allRaptors) {
        if (other == this) continue;

        float dist = Vector3Distance(position, other->position);
        if (dist < repulsionRadius && dist > 1.0f) {
            Vector3 away = Vector3Normalize(Vector3Subtract(position, other->position)); // you - other
            float falloff = (repulsionRadius - dist) / repulsionRadius;
            repulsion = Vector3Add(repulsion, Vector3Scale(away, falloff * repulsionStrength));
        }
    }

    return repulsion;
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
        other.ChangeState(CharacterState::Chase);

    }
}



void Character::SetPath(Vector2 start)
{
    // 1) Find tile path (same as before)
    Vector2 goal = WorldToImageCoords(player.position);
    std::vector<Vector2> tilePath = FindPath(start, goal);

    // 2) Convert tile centers to world points (y based on type)
    std::vector<Vector3> worldPath;
    worldPath.reserve(tilePath.size());
    for (const Vector2& tile : tilePath) {
        Vector3 wp = GetDungeonWorldPos(tile.x, tile.y, tileSize, dungeonPlayerHeight);
        wp.y = (type == CharacterType::Pirate) ? 160.0f : 180.0f; // pirate height
        worldPath.push_back(wp);
    }

    // 3) Smooth in world space using your LOS
    std::vector<Vector3> smoothed = SmoothWorldPath(worldPath);

    // 4) Store
    currentWorldPath = std::move(smoothed);
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

    // Snap feet to path height (keep your existing behavior)
    pos.y = target.y;

    return false;
}

void Character::SetPathTo(const Vector3& goalWorld) {
    Vector2 start = WorldToImageCoords(position);
    Vector2 goal  = WorldToImageCoords(goalWorld);

    std::vector<Vector2> tilePath = FindPath(start, goal);

    currentWorldPath.clear();
    currentWorldPath.reserve(tilePath.size());

    float feetY = (type == CharacterType::Pirate) ? 160.0f : 180.0f; // keep your heights

    for (const Vector2& tile : tilePath) {
        Vector3 worldPos = GetDungeonWorldPos(tile.x, tile.y, tileSize, dungeonPlayerHeight);
        worldPos.y = feetY;
        currentWorldPath.push_back(worldPos);
    }

    // 3) Smooth in world space using your LOS
    std::vector<Vector3> smoothed = SmoothWorldPath(currentWorldPath);

    // 4) Store
    currentWorldPath = std::move(smoothed);
}

// Call this for raptors/Trex (overworld)
void Character::UpdateRaptorVisibility(const Player& player, float deltaTime) {
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

void Character::UpdateChase(float deltaTime)
{
    //update raptor/trex chase state. 
    float ATTACK_ENTER  = 200.0f;   // start attack if closer than this
    if (type == CharacterType::Trex) ATTACK_ENTER = 600; //maybe to far

    const float VISION_ENTER = 4500.0f;
    float distance = Vector3Distance(position, player.position);
    if (!canSee) {ChangeState(CharacterState::Idle); return; }
    if (stateTimer > chaseDuration) { ChangeState(CharacterState::RunAway); return;}
            // (keep your existing enter/exit checks above or below as you like)
    if (distance < ATTACK_ENTER) { ChangeState(CharacterState::Attack); return; }
    if (distance > VISION_ENTER) { ChangeState(CharacterState::Idle); return; }

    const float MAX_SPEED   = raptorSpeed;  // per-type speed
    const float SLOW_RADIUS = 800.0f;       // ease-in so we don’t overshoot
    // Move straight toward the player (XZ only), easing inside SLOW_RADIUS
    Vector3 vel = ArriveXZ(position, player.position, MAX_SPEED, SLOW_RADIUS);
    bool blocked = StopAtWaterEdge(position, vel, 65, deltaTime);

    if (!blocked) position = Vector3Add(position, Vector3Scale(vel, deltaTime));
    //SoundManager::GetInstance().PlaySoundAtPosition("TrexStep", position, player.position, 0.0f, 4000.0f);

    
    if (blocked) ChangeState(CharacterState::RunAway);

    if (vel.x*vel.x + vel.z*vel.z > 1e-4f) {
        rotationY = RAD2DEG * atan2f(vel.x, vel.z);
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
    //update raptor/Trex runaway state
    float distance = Vector3Distance(position, player.position);
    // --- simple knobs ---
    const float MAX_SPEED     = raptorSpeed;   // same as chase or a bit higher
    const float FLEE_MIN_TIME = 2.0f;          // don’t instantly flip back
    const float FLEE_MAX_TIME = 5.0f;          // optional: cap flee bursts
    const float FLEE_EXIT     = 1000.0f;       
    const float SEP_CAP       = 200.0f;        // limit separation shove

    // Steering: flee + a touch of separation + tiny wander so it’s not laser-straight
    Vector3 vFlee   = FleeXZ(position, player.position, MAX_SPEED);

    
    Vector3 vSep    = ComputeRepulsionForce(enemyPtrs, /*radius*/200, /*strength*/600);
    vSep            = Limit(vSep, SEP_CAP);

    
    Vector3 vWander = WanderXZ(wanderAngle, /*turn*/4.0f, /*speed*/80.0f, deltaTime);

    Vector3 desired = Vector3Add(vFlee, Vector3Add(vSep, vWander));
    desired         = Limit(desired, MAX_SPEED);

    bool blocked = StopAtWaterEdge(position, desired, 65, deltaTime);
    if (blocked) ChangeState(CharacterState::Idle);

    // Integrate + face motion
    position = Vector3Add(position, Vector3Scale(desired, deltaTime));
    if (desired.x*desired.x + desired.z*desired.z > 1e-4f) {
        rotationY = RAD2DEG * atan2f(desired.x, desired.z);
    }

    // Exit conditions: far enough OR time window expired
    if ((distance > FLEE_EXIT && stateTimer >= FLEE_MIN_TIME) || stateTimer >= FLEE_MAX_TIME) {
        if (canSee){
            ChangeState(CharacterState::Chase);
            return;
        } 
        
    }
}


void Character::UpdatePatrol(float deltaTime)
{
    //update raptor/Trex patrol state
    // Acquire a patrol target if we don't have one
    if (!hasPatrolTarget) {
        patrolTarget    = RandomPointOnRingXZ(position, 800.0f, 2200.0f);
        hasPatrolTarget = true;
    }

    // Movement toward target
    const float PATROL_SPEED    = raptorSpeed * 0.6f;
    const float PATROL_SLOW_RAD = 400.0f;
    const float ARRIVE_EPS_XZ   = 150.0f;

    Vector3 vel = ArriveXZ(position, patrolTarget, PATROL_SPEED, PATROL_SLOW_RAD);
    position    = Vector3Add(position, Vector3Scale(vel, deltaTime));

    // Stop at water edge → flee
    if (StopAtWaterEdge(position, vel, 65, deltaTime)) {
        hasPatrolTarget = false;
        ChangeState(CharacterState::RunAway);
        return;
    }

    // Face travel direction
    if (vel.x*vel.x + vel.z*vel.z > 1e-4f) {
        rotationY = RAD2DEG * atan2f(vel.x, vel.z);
    }

    // Arrived → idle
    if (DistXZ(position, patrolTarget) <= ARRIVE_EPS_XZ) {
        hasPatrolTarget = false;
        ChangeState(CharacterState::Idle);
        return;
    }

    // Player seen → chase
    const float STALK_ENTER = 2000.0f;
    if (playerVisible && Vector3Distance(position, player.position) < STALK_ENTER) {
        hasPatrolTarget = false;
        if (canSee) ChangeState(CharacterState::Chase); //cant see if player is in water
        return;
    }
}




