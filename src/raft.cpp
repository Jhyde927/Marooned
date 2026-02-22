#include "raft.h"
#include "resourceManager.h"
#include "rlgl.h"

void Raft::Update(float dt)
{
    // Optional future bobbing
    // position.y += sinf(GetTime()) * 0.5f;
}

void Raft::Draw()
{
    const float scale = 100.0f;

    rlDisableBackfaceCulling();
    DrawModelEx(R.GetModel("raft"), position, Vector3{0}, 0.0f, Vector3{scale, scale, scale}, WHITE);

    if (hasBody)
        DrawModel(R.GetModel("raftBody"), position, scale, GRAY);

    if (hasMast)
        DrawModel(R.GetModel("raftMast"), position, scale, WHITE);

    if (hasBoom)
        DrawModel(R.GetModel("raftBoom"), position, scale, WHITE);

    if (hasSail)
        DrawModel(R.GetModel("raftSail"), position, scale, WHITE);
}
