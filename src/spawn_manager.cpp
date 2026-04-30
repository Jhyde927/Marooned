#include "spawn_manager.h"
#include "world.h"
#include "resourceManager.h"

namespace SpawnManager
{
    std::vector<Spawner> spawners;
    bool startSpawning = false;
    int maxAlive = 4;
    int CountAliveSpawnedByType(CharacterType type)
    {
        int count = 0;

        for (Character* enemy : enemyPtrs)
        {
            if (!enemy) continue;
            if (enemy->isDead) continue;
            if (enemy->spawnSource != SpawnSource::Spawner) continue;
            if (enemy->type != type) continue;

            count++;
        }

        return count;
    }

    void Clear()
    {
        spawners.clear();
    }

    

    void Update(float dt)
    {   
        if (gKraken.isDead) return;



        // if player is in range and has LOS of kraken head, start spawning and lock the doors.
        if (gKraken.playerInRange && !startSpawning){  
            startSpawning = true;
            EventLockAllDoors(true);
            player.startPosition = Vector3{3585.0f, 220.0f, 3062.0f}; //move respawn location to just outside of boss arena. 
        } 


        if (!startSpawning) return;

        int pirates   = CountAliveSpawnedByType(CharacterType::Pirate);
        int skeletons = CountAliveSpawnedByType(CharacterType::Skeleton);
        int total     = pirates + skeletons;
        for (Spawner& spawner : spawners)
        {
            if (!spawner.enabled) continue;


            if (spawner.portalTimer > 0.0f)
            {
                spawner.portalTimer -= dt;
                std::cout << "portalTimer = " << spawner.portalTimer << "\n";
            }


            if (total >= maxAlive) continue;

            spawner.timer += dt;

            // later:
            if (spawner.timer >= spawner.cooldown)
            {
                
                spawner.timer = 0.0f;
                switch (spawner.type)
                {
                    case SpawnerType::Skeleton:
                    {     

                        Character skeleton(
                            spawner.position,
                            R.GetTexture("skeletonSheet"), 
                            200, 200,         // frame width, height
                            1,                // max frames
                            0.8f, 0.5f,       // scale, speed
                            0,                // initial animation frame
                            CharacterType::Skeleton
                        );
                        skeleton.spawnSource = SpawnSource::Spawner;
                        skeleton.maxHealth = 200;
                        skeleton.currentHealth = 200; //at least 2 shots. 4 sword swings 
                        skeleton.id = gEnemyCounter++;
                        enemies.push_back(skeleton);
                        spawner.portalTimer = 2.0f; // show portal 2 seconds after spawning. 
                        break;
                    }

                    case SpawnerType::Pirate:
                    {                        
                        Character pirate(
                            spawner.position,
                            R.GetTexture("pirateSheet"), 
                            200, 200,         // frame width, height 
                            1,                // max frames, set when setting animations
                            0.5f, 0.5f,       // scale, speed
                            0,                // initial animation frame
                            CharacterType::Pirate
                        );
                        
                        pirate.spawnSource = SpawnSource::Spawner;
                        pirate.maxHealth = 400; // twice as tough as skeletons, at least 3 shots. 8 slices.
                        pirate.currentHealth = 400;
                        pirate.id = gEnemyCounter++;
                        enemies.push_back(pirate);
                        spawner.portalTimer = 2.0f; // show portal 2 seconds after spawning. 
                    }

                    default:
                        break;
                    }



            }
        }
    }
}