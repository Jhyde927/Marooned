#pragma once

#include "raylib.h"
#include "raymath.h"

struct ViewConeParams {
    Vector3 camPos;
    Vector3 camForward;
    float  cosHalfFov;
    float  maxDistSq;
    float  nearDistSq;
};

inline ViewConeParams MakeViewConeParams(const Camera3D& cam,
                                         float halfFovDeg,
                                         float maxDrawDist,
                                         float nearDist = 400.0f)
{
    ViewConeParams vp{};
    vp.camPos     = cam.position;
    vp.camForward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));

    float halfFovRad = halfFovDeg * PI / 180.0f;
    vp.cosHalfFov   = cosf(halfFovRad);
    vp.maxDistSq    = maxDrawDist * maxDrawDist;
    vp.nearDistSq   = nearDist * nearDist;

    return vp;
}

inline bool IsInViewCone(const ViewConeParams& vp,
                         const Vector3& point)
{
    Vector3 toP = Vector3Subtract(point, vp.camPos);
    float distSq = Vector3LengthSqr(toP);

    if (distSq > vp.maxDistSq)
        return false;

    // Always include very close tiles/chunks to avoid near clipping pop-in
    if (distSq < vp.nearDistSq || distSq < 0.0001f)
        return true;

    Vector3 dirToP = Vector3Normalize(toP);
    float dot = Vector3DotProduct(vp.camForward, dirToP);

    return (dot >= vp.cosHalfFov);
}
