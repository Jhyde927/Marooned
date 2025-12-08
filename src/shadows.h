// ===== Shadows.h =====
#pragma once
#include <vector>
#include "raylib.h"
#include "vegetation.h"
#include "raymath.h"
#include "resourceManager.h"
#include "world.h"

//Holder for the rt
struct TreeShadowMask {
    RenderTexture2D rt = {};
    int width = 0, height = 0;
    Rectangle worldXZBounds = {}; // {x,y,w,h} => (minX, minZ, sizeX, sizeZ)
};

inline Vector2 WorldXZToUV(const Rectangle& worldXZBounds, const Vector3& p) {
    float u = (p.x - worldXZBounds.x) / worldXZBounds.width;
    float v = (p.z - worldXZBounds.y) / worldXZBounds.height;
    return { u, v };
}

// Create or resize the mask RT
inline void InitOrResizeTreeShadowMask(TreeShadowMask& mask, int w, int h, const Rectangle& worldXZBounds) {
    if (mask.rt.id == 0 || mask.width != w || mask.height != h) {
        if (mask.rt.id != 0) UnloadRenderTexture(mask.rt);
        mask.rt    = LoadRenderTexture(w, h);
        mask.width = w;
        mask.height= h;
        mask.worldXZBounds = worldXZBounds;
    }
}

struct ShadowParams {
    float minPx   = 64.0f;   // smallest canopy in pixels on the mask
    float maxPx   = 150.0f;   // largest canopy in pixels on the mask
    float curve   = 1.6f;    // >1 = bias small, <1 = bias large
    float jitter  = 0.08f;   // ±8% random break-up
};

// t.scale is ~20..30 in your setup
static inline float ScaleToDiameterPx(float scale, const ShadowParams& p) {
    float s = (scale - 20.0f) / 10.0f;        // 0..1
    s = fminf(fmaxf(s, 0.0f), 1.0f);
    // curve control (ease-in like powf); replace with smoothstep for cheaper curve
    float w = powf(s, p.curve);
    float px = p.minPx + (p.maxPx - p.minPx) * w;

    // tiny per-tree jitter so same-sized trees don’t look copy/pasted
    //float j = 1.0f + ((GetRandomValue(-1000, 1000) / 1000.0f) * p.jitter);
    return px;
}


static inline void StampCanopy(Texture2D tex,
                               int cx, int cy,       // center in mask pixels
                               float diameterPx,     // dest diameter in pixels
                               float rotationDeg,
                               float strength01)
{
    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };

    // Pattern A: dest.x/y = CENTER, origin = half size
    Rectangle dst   = { (float)cx, (float)cy, diameterPx, diameterPx };
    Vector2   origin= { diameterPx*0.5f, diameterPx*0.5f };

    unsigned char a = (unsigned char)Clamp(strength01 * 255.0f, 0.0f, 255.0f);
    Color tint = { 0, 0, 0, a };

    DrawTexturePro(tex, src, dst, origin, rotationDeg, tint);
}

inline void BuildTreeShadowMask_Tex(TreeShadowMask& mask,
                                    const std::vector<TreeInstance>& trees,
                                    Texture2D canopyTex, 
                                    float strength01 = 0.45f// overall darkness
                                    )
{
    BeginTextureMode(mask.rt);
    ClearBackground(WHITE);
    BeginBlendMode(BLEND_ALPHA);

    // const float metersPerTexel = mask.worldXZBounds.width / (float)mask.width;
    ShadowParams p;
    for (const auto& t : trees) {
        // Use the EXACT draw position (incl. your jitter offsets)
        Vector3 wp = t.position;
        wp.x += t.xOffset;
        wp.z += t.zOffset;

        // const float metersPerTexel = mask.worldXZBounds.width / (float)mask.width;

        // float rel = powf(t.scale / 25.0f, 2.0f); // >1.0 widens differences
        // float diameterM = baseDiameterMeters * rel;
        // float diameterPx = diameterM / metersPerTexel;

        float diameterPx = ScaleToDiameterPx(t.scale, p);//fmaxf(diameterPx, 12.0f); 


        // World → mask pixels
        Vector2 uv = WorldXZToUV(mask.worldXZBounds, wp);
        int cx = (int)(uv.x * mask.width);
        int cy = (int)(uv.y * mask.height);

        
        StampCanopy(canopyTex, cx, cy, diameterPx, t.rotationY, strength01);
        
    }

    EndBlendMode();
    EndTextureMode();
}

