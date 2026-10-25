#pragma once

#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>


// Include after raylib.h and raymath.h. No world/game dependencies.
namespace BulletCollisionGeometry
{
struct Hit
{
    float t = 1.0f;
    Vector3 normal = {0, 1, 0};
};

inline Vector3 Point(Vector3 a, Vector3 b, float t)
{
    return Vector3Add(a, Vector3Scale(Vector3Subtract(b, a), t));
}

inline Vector3 OpposingAxis(Vector3 delta)
{
    const float x = std::fabs(delta.x), y = std::fabs(delta.y), z = std::fabs(delta.z);
    if (x >= y && x >= z) return {delta.x >= 0 ? -1.0f : 1.0f, 0, 0};
    if (z >= y) return {0, 0, delta.z >= 0 ? -1.0f : 1.0f};
    return {0, delta.y >= 0 ? -1.0f : 1.0f, 0};
}

// Segment against a box expanded by radius. Conservative at rounded corners:
// this prevents tunnelling but can register a sphere hit slightly early there.
inline bool SweepBox(Vector3 a, Vector3 b, BoundingBox box, float radius, Hit& hit)
{
    const Vector3 d = Vector3Subtract(b, a);
    const float start[3] = {a.x, a.y, a.z};
    const float delta[3] = {d.x, d.y, d.z};
    const float lo[3] = {box.min.x-radius, box.min.y-radius, box.min.z-radius};
    const float hi[3] = {box.max.x+radius, box.max.y+radius, box.max.z+radius};
    float enter = 0.0f, leave = 1.0f;
    Vector3 normal = OpposingAxis(d);
    for (int axis = 0; axis < 3; ++axis)
    {
        if (std::fabs(delta[axis]) < 0.000001f)
        {
            if (start[axis] < lo[axis] || start[axis] > hi[axis]) return false;
            continue;
        }
        float nearT = (lo[axis] - start[axis]) / delta[axis];
        float farT = (hi[axis] - start[axis]) / delta[axis];
        float sign = -1.0f;
        if (nearT > farT) { std::swap(nearT, farT); sign = 1.0f; }
        if (nearT > enter)
        {
            enter = nearT;
            normal = {0, 0, 0};
            if (axis == 0) normal.x = sign;
            if (axis == 1) normal.y = sign;
            if (axis == 2) normal.z = sign;
        }
        leave = std::min(leave, farT);
        if (enter > leave) return false;
    }
    hit = {enter, normal};
    return true;
}
}
