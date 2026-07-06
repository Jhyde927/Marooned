#include "spawn_manager.h"
#include "world.h"
#include "resourceManager.h"
#include "sound_manager.h"
#include "shaderSetup.h"

namespace SpawnManager
{
    std::vector<Spawner> spawners;
    bool startSpawning = false;
    bool startCutscene = false;
    bool cutSceneFinished = false;
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
        if (!CurrentLevelIs("Ship")) return;
        if (gKraken.isDead) return;


        //first play the cut scene
        if (doors[4].isOpen && !startCutscene) //there is a better way.
        {
            startCutscene = true;
            Cutscenes::StartKrakenScene();
            gKraken.Rise();
            for (Tentacle t : tentacles){
                t.Rise();
            }

        }


        // gKraken.trigger is set by door[2] opening. This gets rid of the anti chamber we had where the player could safely respawn. 
        if (cutSceneFinished && !startSpawning){  
            startSpawning = true;
            ShaderSetup::StartSkyTransition(0.95, 30.0f); //30 seconds too long? 
            EventLockAllDoors(true);
            Vector3 playerPosAfterCutscene = Vector3{3585.0f, 220.0f, 3062.0f};
            player.position = playerPosAfterCutscene;
            player.startPosition = playerPosAfterCutscene;//move respawn location to just outside of boss arena. 
            
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
            }


            if (total >= maxAlive) continue;

            spawner.timer += dt;

            // later:
            if (spawner.timer >= spawner.cooldown)
            {
                
                spawner.timer = 0.0f;
                SoundManager::GetInstance().PlaySoundAtPosition("portal", spawner.position, player.position, 0.0f, 3000.0f);

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