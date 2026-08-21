#include "raft.h"
#include "resourceManager.h"
#include "rlgl.h"
#include "cmath"


bool Raft::PlayerInRange(Vector3 playerPosition, float dist){
    float range = dist;
    float rangeSq = range * range;

    Vector3 d = Vector3Subtract(playerPosition, position);
    d.y = 0.0f;

    return Vector3LengthSqr(d) <= rangeSq;

}

void Raft::Update(const Player& player, float dt)
{
    (void)dt;
    // Optional future bobbing
    position.y += sin(GetTime()) * 0.1f;
    if (PlayerInRange(player.position, 500.0f)){
        showMessage = true; 
    }else{
        showMessage = false;
    }
}

void Raft::Draw()
{
    //positions of the peices 
    const float scale = 100.0f;

    //rlDisableBackfaceCulling();
    DrawModelEx(R.GetModel("raft"), position, Vector3{0, 0, 0}, 0.0f, Vector3{scale, scale, scale}, WHITE); //draw the ghost raft. 

    if (hasBody)
        DrawModel(R.GetModel("raftBody"), position, scale, GRAY); //darken the boards.

    if (hasMast)
        DrawModel(R.GetModel("raftMast"), position, scale, WHITE);

    if (hasBoom)
        DrawModel(R.GetModel("raftBoom"), position, scale, WHITE);

    if (hasSail)
        DrawModel(R.GetModel("raftSail"), position, scale, WHITE);
}