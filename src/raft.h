#pragma once
#include "raylib.h"

struct Raft
{
    Vector3 position = {5997.0f, 55.0f, -2610.0f};

    bool hasBody  = false;
    bool hasMast  = false;
    bool hasBoom  = false;
    bool hasSail  = false;

    void Update(float dt);
    void Draw();

    void AddBody() { hasBody = true; }
    void AddMast() { hasMast = true; }
    void AddBoom() { hasBoom = true; }
    void AddSail() { hasSail = true; }

    bool IsComplete() const
    {
        return hasBody && hasMast && hasBoom && hasSail;
    }
};