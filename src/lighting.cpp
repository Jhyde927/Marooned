#include "lighting.h"
#include "raylib.h"
#include "pathfinding.h"
#include "dungeonGeneration.h"
#include "raymath.h"
#include "cfloat"
#include "stdint.h"
#include "world.h"
#include "utilities.h"
#include "dungeonColors.h"


BakedLightmap gDynamic; 

static std::vector<Color> gStaticBase;   // same w*h as gDynamic
std::vector<int> StaticLightIndices;



//lighting control
LightingConfig lightConfig =
{
    0.15f, //0.15f // ambient //dark on youtube but anything higher is too orange. 
    0.8f,   // dynStrength

    2100.0f,  // staticRadius
    0.6f,     // staticIntensity
    {0.8f, 0.9f, 1.0f}, // staticColor

    400.0f,   // dynamicRange
    0.5f,     // dynamicIntensity
    {1.0f, 0.3f, 0.0f},   // dynamicFireColor
    {0.0f, 0.7f, 1.0f},   // dynamicIceColor

    {1.0f, 0.8f, 0.2f},   // playerColor
    200.0f,               // playerRadius
    0.5f,                 // playerIntensity

    7,        // losNumRays
    0.25f,    // losSpreadFrac
    120.0f,   // losOriginY
    120.0f,   // losTargetY
    0.005f,    // losEpsilonFrac

    //tonemap
    1.25, //dungeon exposure
    0.9 //outside exposure
};


LightSource MakeStaticTorch(Vector3 pos) { //init static light params
    LightSource L;
    L.position  = pos;
    L.colorTint = lightConfig.staticColor;
    L.intensity = lightConfig.staticIntensity;
    L.range     = lightConfig.staticRadius;
    L.lifeTime  = -1; // infinite
    L.type      = LightType::StaticFire;
    return L;
}

void GatherFrameLights() {
    //each bullet has a struct called light. Check bullet lights and add info to LightSample struct and add to framelight vector.
    frameLights.clear();
    frameLights.reserve(activeBullets.size()); // upper bound

    for (const Bullet& b : activeBullets) {
        if (!b.light.active) continue;
        if (b.light.age > 0.75) continue; //wait to stop lights 

        LightSample s;
        s.color     = b.light.color;
        s.range     = b.light.range;
        s.intensity = b.light.intensity;
        if (b.exploded) s.range = s.range * 2; //increase light range when exploding. 

        s.pos = b.light.detached ? b.light.posWhenDetached
                                 : b.GetPosition();

        // sanity checks (avoid NaN/zero range)
        if (s.range > 0.f && isfinite(s.range) &&
            isfinite(s.pos.x) && isfinite(s.pos.y) && isfinite(s.pos.z)) {
            frameLights.push_back(s);
        }
    }
}




static inline bool RectsIntersectInclusive(const RectI& a, const RectI& b)
{
    return !(a.x1 < b.x0 || a.x0 > b.x1 || a.y1 < b.y0 || a.y0 > b.y1);
}

static inline RectI IntersectInclusive(const RectI& a, const RectI& b)
{
    RectI r;
    r.x0 = std::max(a.x0, b.x0);
    r.y0 = std::max(a.y0, b.y0);
    r.x1 = std::min(a.x1, b.x1);
    r.y1 = std::min(a.y1, b.y1);
    return r;
}

static RectI ClampRectToMap(RectI r, int w, int h)
{
    if (r.x0 < 0) r.x0 = 0;
    if (r.y0 < 0) r.y0 = 0;
    if (r.x1 > w) r.x1 = w;
    if (r.y1 > h) r.y1 = h;
    if (r.x1 < r.x0) r.x1 = r.x0;
    if (r.y1 < r.y0) r.y1 = r.y0;
    return r;
}

static RectI ComputeDoorAffectedRegion_LightmapPixels_UsingStamperMapping(
    const Vector3& doorPos,
    int bufW, int bufH,
    float radius) // world units (e.g., 2200)
{
    const int tppX = bufW / dungeonWidth;
    const int tppZ = bufH / dungeonHeight;

    // Door world -> tile indices using SAME origin as stamper
    const int tx = (int)floorf((doorPos.x - gDynamic.minX) / tileSize);
    const int tz = (int)floorf((doorPos.z - gDynamic.minZ) / tileSize);

    // If door is outside dungeon bounds, return empty rect
    if (tx < 0 || tz < 0 || tx >= dungeonWidth || tz >= dungeonHeight) {
        return RectI{0,0,-1,-1}; // empty (x0 > x1)
    }

    // Door tile -> lightmap pixel center (roughly center of that tile in texels)
    const int cx = tx * tppX + (tppX / 2);
    const int cy = tz * tppZ + (tppZ / 2);

    // Radius in tiles -> radius in pixels
    const int Rtiles = (int)ceilf(radius / tileSize);
    const int RpxX   = Rtiles * tppX;
    const int RpxY   = Rtiles * tppZ;

    RectI r;
    r.x0 = cx - RpxX;
    r.y0 = cy - RpxY;
    r.x1 = cx + RpxX;
    r.y1 = cy + RpxY;

    // Clamp to buffer bounds (inclusive)
    if (r.x0 < 0) r.x0 = 0;
    if (r.y0 < 0) r.y0 = 0;
    if (r.x1 > bufW - 1) r.x1 = bufW - 1;
    if (r.y1 > bufH - 1) r.y1 = bufH - 1;

    return r;
}


static void ClearStaticBaseRegion(std::vector<Color>& buf, int w, int h, const RectI& r)
{
    if (r.x0 == r.x1 || r.y0 == r.y1) return;

    for (int y = r.y0; y < r.y1; ++y) {
        int row = y * w;
        for (int x = r.x0; x < r.x1; ++x) {
            buf[row + x] = (Color){0,0,0,255};
        }
    }


}

void StampLight_StaticBase_Subtile2x2_ToBuffer_Clipped(std::vector<Color>& outBuf, int bufW, int bufH,
                                                       const Vector3& lightPos, float radius, Color color,
                                                       const RectI& clip)
{

    if ((int)outBuf.size() != bufW * bufH)
        outBuf.assign((size_t)bufW * bufH, (Color){0,0,0,255});

    // Clamp clip to buffer (defensive)
    RectI clipC = clip;
    clipC.x0 = std::max(0, std::min(clipC.x0, bufW - 1));
    clipC.y0 = std::max(0, std::min(clipC.y0, bufH - 1));
    clipC.x1 = std::max(0, std::min(clipC.x1, bufW - 1));
    clipC.y1 = std::max(0, std::min(clipC.y1, bufH - 1));
    if (clipC.x0 > clipC.x1 || clipC.y0 > clipC.y1) return;

    const int tppX = bufW / dungeonWidth;   // texels-per-tile (X)
    const int tppZ = bufH / dungeonHeight;  // texels-per-tile (Z)

    const int lx = (int)floorf((lightPos.x - gDynamic.minX) / tileSize);
    const int lz = (int)floorf((lightPos.z - gDynamic.minZ) / tileSize);
    const int R  = (int)ceilf(radius / tileSize);
    const float r2 = radius * radius;

    const int tx0 = std::max(0,              lx - R), tx1 = std::min(dungeonWidth  - 1, lx + R);
    const int tz0 = std::max(0,              lz - R), tz1 = std::min(dungeonHeight - 1, lz + R);

    for (int tz = tz0; tz <= tz1; ++tz)
    {
        for (int tx = tx0; tx <= tx1; ++tx)
        {
            // Tile texel bounds (inclusive)
            const int tileX0 = tx * tppX;
            const int tileX1 = tileX0 + tppX - 1;
            const int tileY0 = tz * tppZ;
            const int tileY1 = tileY0 + tppZ - 1;

            // Early reject: tile doesn't overlap the clip region at all
            RectI tileRect{ tileX0, tileY0, tileX1, tileY1 };
            if (!RectsIntersectInclusive(tileRect, clipC))
                continue;

            const float cx = gDynamic.minX + (tx + 0.5f) * tileSize;
            const float cz = gDynamic.minZ + (tz + 0.5f) * tileSize;

            // quick circle cull (center)
            const float cdx = cx - lightPos.x, cdz = cz - lightPos.z;
            if (cdx*cdx + cdz*cdz > (radius + 0.75f*tileSize)*(radius + 0.75f*tileSize))
                continue;

            float visUniform = 1.0f;
            float vis2x2[2][2]; bool useSubtile = false;

            SubtileVis2x2(vis2x2, lightPos, cx, cz, tileSize, floorHeight);
            if (vis2x2[0][0]==0 && vis2x2[0][1]==0 && vis2x2[1][0]==0 && vis2x2[1][1]==0)
                continue;
            useSubtile = true;

            // Clamp the per-tile loops to the intersected rect
            RectI drawRect = IntersectInclusive(tileRect, clipC);

            for (int y = drawRect.y0; y <= drawRect.y1; ++y)
            {
                const float texV = (y + 0.5f) / bufH;
                const float wz   = gDynamic.minZ + texV * gDynamic.sizeZ;

                for (int x = drawRect.x0; x <= drawRect.x1; ++x)
                {
                    const float texU = (x + 0.5f) / bufW;
                    const float wx   = gDynamic.minX + texU * gDynamic.sizeX;

                    const float dx = wx - lightPos.x, dz = wz - lightPos.z;
                    const float d2 = dx*dx + dz*dz;
                    if (d2 > r2) continue;

                    float u = sqrtf(d2) / radius;   // 0..1
                    if (u > 1.0f) continue;

                    // base falloff
                    float t = 1.0f - u*u;
                    float base = t*t*(3.0f - 2.0f*t);

                    // ring params (your current defaults)
                    float ringPos   = 0.0f;
                    float ringWidth = 0.2f;
                    float ringAmp   = 0.7f;

                    float du   = (u - ringPos);
                    float ring = expf(-(du*du) / (2.0f*ringWidth*ringWidth));

                    float w = base + ringAmp * ring;

                    // apply visibility (2×2)
                    float vis = visUniform;
                    if (useSubtile)
                    {
                        // IMPORTANT: sx/sy must be computed relative to the tile's full texel area,
                        // not relative to drawRect (which is clipped).
                        int sx = ((x - tileX0) * 2) / tppX; if (sx < 0) sx = 0; else if (sx > 1) sx = 1;
                        int sy = ((y - tileY0) * 2) / tppZ; if (sy < 0) sy = 0; else if (sy > 1) sy = 1;
                        vis = vis2x2[sy][sx];
                        if (vis <= 0.0f) continue;
                    }

                    w *= vis;
                    if (w <= 0.0f) continue;

                    Color& p = outBuf[y * bufW + x];
                    int r = p.r + (int)(color.r * w);
                    int g = p.g + (int)(color.g * w);
                    int b = p.b + (int)(color.b * w);
                    p.r = (unsigned char)std::min(r, 255);
                    p.g = (unsigned char)std::min(g, 255);
                    p.b = (unsigned char)std::min(b, 255);
                }
            }
        }
    }
}





// Returns 0..1 visibility from light -> tileCenter using a small perpendicular ray fan.
// spreadMeters ~ 0.15f * tileSize; numRays 3..5; epsilonFrac ~ 0.02f
static float TileVisibilityWorldRay(const Vector3& lightPos,
                                    const Vector3& tileCenter,
                                    int numRays, float spreadMeters,
                                    float originYOffset, float targetYOffset,
                                    float epsilonFrac)
{
    // Direction in XZ, build a perpendicular (for fan offsets)
    Vector3 dirXZ = { tileCenter.x - lightPos.x, 0.0f, tileCenter.z - lightPos.z };
    float len = sqrtf(dirXZ.x*dirXZ.x + dirXZ.z*dirXZ.z);
    if (len < 1e-3f) return 1.0f; // same tile
    Vector3 perp = { -dirXZ.z/len, 0.0f, dirXZ.x/len };

    int visible = 0;
    int rays = (numRays < 1) ? 1 : numRays;
    float half = 0.5f * (float)(rays - 1);

    Vector3 from = lightPos; from.y += originYOffset;

    for (int i = 0; i < rays; ++i) {
        float t = ((float)i - half);                 // … -2,-1,0,1,2 …
        Vector3 to = tileCenter;
        to.x += perp.x * t * spreadMeters;
        to.z += perp.z * t * spreadMeters;
        to.y  = tileCenter.y + targetYOffset;

        if (HasWorldLineOfSight(from, to, epsilonFrac, LOSMode::Lighting)) ++visible;  //WARNING set to AI for doors blocking light. 
    }
    return (float)visible / (float)rays;
}

// --- 2×2 sub-tile visibility using your world-ray LOS fan ---
void SubtileVis2x2(float vis[2][2],
                          const Vector3& lightPos,
                          float cx, float cz, float tileSize, float floorY)
{
    // Offsets to centers of the 4 quarters: (-0.25,+0.25) * tileSize
    const float o = 0.25f * tileSize;
    const float offs[2] = {-o, +o};

    for (int sy=0; sy<2; ++sy) {
        for (int sx=0; sx<2; ++sx) {
            Vector3 subCenter = { cx + offs[sx], floorY, cz + offs[sy] };
            // reuse your TileVisibilityWorldRay(lightPos, tileCenter, numRays, spread, originY, targetY, eps)
            vis[sy][sx] = TileVisibilityWorldRay(lightPos, subCenter,
                                                 /*numRays=*/lightConfig.losNumRays,
                                                 /*spreadMeters=*/lightConfig.losSpreadFrac*tileSize,
                                                 /*originYOffset=*/lightConfig.losOriginY,
                                                 /*targetYOffset=*/lightConfig.losTargetY,
                                                 /*epsilonFrac=*/lightConfig.losEpsilonFrac);
        }
    }
}




// // --- Static bake: tile-first stamping with 2×2 sub-tile LOS near occluders ---
void StampLight_StaticBase_Subtile2x2_ToBuffer(std::vector<Color>& outBuf, int bufW, int bufH,
                                               const Vector3& lightPos, float radius, Color color)
{
    if ((int)outBuf.size() != bufW*bufH) outBuf.assign((size_t)bufW*bufH, (Color){0,0,0,255});

    const int tppX = bufW / dungeonWidth;            // texels-per-tile (X)
    const int tppZ = bufH / dungeonHeight;            // texels-per-tile (Z)

    const int lx = (int)floorf((lightPos.x - gDynamic.minX) / tileSize);
    const int lz = (int)floorf((lightPos.z - gDynamic.minZ) / tileSize);
    const int R  = (int)ceilf(radius / tileSize);
    const float r2 = radius*radius;

    const int tx0 = std::max(0,        lx - R), tx1 = std::min(dungeonWidth-1, lx + R);
    const int tz0 = std::max(0,        lz - R), tz1 = std::min(dungeonHeight-1, lz + R);

    for (int tz = tz0; tz <= tz1; ++tz) {
        for (int tx = tx0; tx <= tx1; ++tx) {
            const float cx = gDynamic.minX + (tx + 0.5f)*tileSize;
            const float cz = gDynamic.minZ + (tz + 0.5f)*tileSize;

            // quick circle cull (center)
            const float cdx = cx - lightPos.x, cdz = cz - lightPos.z;
            if (cdx*cdx + cdz*cdz > (radius + 0.75f*tileSize)*(radius + 0.75f*tileSize)) continue;

            // Decide visibility model for this tile:
            // default: uniform vis=1 (fast path)
            float visUniform = 1.0f;
            float vis2x2[2][2]; bool useSubtile = false;

            //if (TileNearSolid(tx, tz)) { //dont do this check for better results. 
                // compute 2×2 sub-tile vis only near occluders
            SubtileVis2x2(vis2x2, lightPos, cx, cz, tileSize, floorHeight);
            // if all zero, skip whole tile
            if (vis2x2[0][0]==0 && vis2x2[0][1]==0 && vis2x2[1][0]==0 && vis2x2[1][1]==0) continue;
            useSubtile = true;
            //}

            // Texel bounds for this tile
            const int x0 = tx*tppX, x1 = x0 + tppX - 1;
            const int y0 = tz*tppZ, y1 = y0 + tppZ - 1;

            for (int y = y0; y <= y1; ++y) {
                const float texV = (y + 0.5f)/bufH;
                const float wz   = gDynamic.minZ + texV * gDynamic.sizeZ;

                for (int x = x0; x <= x1; ++x) {
                    const float texU = (x + 0.5f)/bufW;
                    const float wx   = gDynamic.minX + texU * gDynamic.sizeX;

                    const float dx = wx - lightPos.x, dz = wz - lightPos.z;
                    const float d2 = dx*dx + dz*dz;
                    if (d2 > r2) continue;

                    // float t = 1.0f - (d2 / r2);
                    // float w = t*t*(3.0f - 2.0f*t);   // smoothstep
                    // w = powf(w, 0.5f);               // <--- NEW: boost center

                    float u = sqrtf(d2) / radius;   // 0..1
                    if (u > 1.0f) continue;

                    // base falloff (your current smoothstep)
                    float t = 1.0f - u*u;                 // same as 1 - d2/r2
                    float base = t*t*(3.0f - 2.0f*t);

                    // ring parameters (tweakables)
                    float ringPos   = 0.0f;   // where the ring sits (0..1)
                    float ringWidth = 0.2f;   // thickness (0..1)
                    float ringAmp   = 0.7f;    // how strong

                    // gaussian ring centered at ringPos
                    float du   = (u - ringPos);
                    float ring = expf(-(du*du) / (2.0f*ringWidth*ringWidth));

                    // combine
                    float w = base + ringAmp * ring;

                    // optional: keep inside 0..1ish
                    //w = fminf(w, 1.0f);

                    // apply visibility
                    float vis = visUniform;
                    if (useSubtile) {
                        // map texel to sub-tile index (2×2)
                        int sx = ((x - x0) * 2) / tppX; if (sx < 0) sx = 0; else if (sx > 1) sx = 1;
                        int sy = ((y - y0) * 2) / tppZ; if (sy < 0) sy = 0; else if (sy > 1) sy = 1;
                        vis = vis2x2[sy][sx];
                        if (vis <= 0.0f) continue;
                    }

                    w *= vis;
                    if (w <= 0.0f) continue;

                    Color& p = outBuf[y*bufW + x];
                    int r = p.r + (int)(color.r * w);
                    int g = p.g + (int)(color.g * w);
                    int b = p.b + (int)(color.b * w);
                    p.r = (unsigned char)std::min(r, 255);
                    p.g = (unsigned char)std::min(g, 255);
                    p.b = (unsigned char)std::min(b, 255);
                }
            }
        }
    }
}



// Compute world-space bounds from dungeon indices & tile size.
static void ComputeDungeonXZBounds(int dungeonWidth, int dungeonHeight, float tileSize,
                                   float baseY, float& outMinX, float& outMinZ,
                                   float& outSizeX, float& outSizeZ)
{
    Vector3 p00 = GetDungeonWorldPos(0, 0, dungeonWidth > 0 ? tileSize : 1.0f, baseY);
    Vector3 p11 = GetDungeonWorldPos(dungeonWidth - 1, dungeonHeight - 1, tileSize, baseY);

    float half = tileSize * 0.5f;

    float minX = std::min(p00.x, p11.x) - half;
    float maxX = std::max(p00.x, p11.x) + half;
    float minZ = std::min(p00.z, p11.z) - half;
    float maxZ = std::max(p00.z, p11.z) + half;

    outMinX = minX;
    outMinZ = minZ;
    outSizeX = (maxX - minX);
    outSizeZ = (maxZ - minZ);
}



void InitDynamicLightmap(int res)
{
    gStaticBase.clear();
    // If re-initting, free the old GPU texture to avoid leaks
    if (gDynamic.tex.id != 0){
        UnloadTexture(gDynamic.tex);
    } 

    // Resolution
    gDynamic.w = res;
    gDynamic.h = res;

    gStaticBase.assign((size_t)gDynamic.w * gDynamic.h, (Color){0,0,0,255});

    // World-space mapping for this level (XZ bounds)
    ComputeDungeonXZBounds(dungeonWidth, dungeonHeight, tileSize, floorHeight,
                           gDynamic.minX, gDynamic.minZ, gDynamic.sizeX, gDynamic.sizeZ);

    // CPU buffer (black = no light)
    gDynamic.pixels.assign(gDynamic.w * gDynamic.h, (Color){0,0,0,255});

    // GPU texture
    Image img = GenImageColor(gDynamic.w, gDynamic.h, BLACK);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8); // <- ensure RGBA8
    gDynamic.tex = LoadTextureFromImage(img);
    UnloadImage(img);

    SetTextureFilter(gDynamic.tex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(gDynamic.tex, TEXTURE_WRAP_CLAMP);

    TraceLog(LOG_INFO, "InitDynamicLightmap: w=%d h=%d tile=%.1f floorY=%.1f",
         dungeonWidth, dungeonHeight, tileSize, floorHeight);

    TraceLog(LOG_INFO, "Bounds: minX=%.1f minZ=%.1f sizeX=%.1f sizeZ=%.1f",
         gDynamic.minX, gDynamic.minZ, gDynamic.sizeX, gDynamic.sizeZ);
}

std::vector<int> GetStaticLightIndices(const Vector3& doorPos)
{
    std::vector<int> affected;

    for (int i = 0; i < (int)dungeonLights.size(); ++i)
    {
        const LightSource& l = dungeonLights[i];

        float dist = Vector3Distance(doorPos, l.position);
        if (dist < l.range)
        {
            affected.push_back(i);
        }
    }

    return affected;
}



// Simple smooth falloff (1 at center -> 0 at radius)
float SmoothFalloff(float d, float radius)
{
    if (d >= radius) return 0.0f;
    float t = 1.0f - (d / radius);   // 1..0
    // smootherstep style
    return t * t * (3.0f - 2.0f * t);
}


XZBounds ComputeXZFromTiles(const std::vector<FloorTile>& tiles, float tileSize) {
    XZBounds b{ FLT_MAX, -FLT_MAX, +FLT_MAX, -FLT_MAX };
    for (const auto& t : tiles) {
        b.minX = std::min(b.minX, t.position.x);
        b.maxX = std::max(b.maxX, t.position.x);
        b.minZ = std::min(b.minZ, t.position.z);
        b.maxZ = std::max(b.maxZ, t.position.z);
    }
    const float half = tileSize * 0.5f;
    b.minX -= half; b.maxX += half;
    b.minZ -= half; b.maxZ += half;
    return b;


}



static void StampDynamicLight(const Vector3& lightPos, float radius, Color color) {
    // Map world XZ -> texture space //used for fireballs and player light, No occlusion
    float u = (lightPos.x - gDynamic.minX) / gDynamic.sizeX; // 0..1
    float v = (lightPos.z - gDynamic.minZ) / gDynamic.sizeZ; // 0..1

    int cx = (int)(u * gDynamic.w);
    int cy = (int)(v * gDynamic.h);

    int rx = (int)ceilf((radius / gDynamic.sizeX) * gDynamic.w);
    int ry = (int)ceilf((radius / gDynamic.sizeZ) * gDynamic.h);

    int x0 = std::max(0, cx - rx), x1 = std::min(gDynamic.w - 1, cx + rx);
    int y0 = std::max(0, cy - ry), y1 = std::min(gDynamic.h - 1, cy + ry);

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            float texU = (x + 0.5f) / gDynamic.w;
            float texV = (y + 0.5f) / gDynamic.h;
            float wx = gDynamic.minX + texU * gDynamic.sizeX;
            float wz = gDynamic.minZ + texV * gDynamic.sizeZ;

            float dx = wx - lightPos.x;
            float dz = wz - lightPos.z;
            float d  = sqrtf(dx*dx + dz*dz);

            float w = SmoothFalloff(d, radius);
            if (w <= 0.0f) continue;

            Color& p = gDynamic.pixels[y * gDynamic.w + x];

            // Additive (clamped); dynamic map stores 0..255 color
            int r = p.r + (int)(color.r * w);
            int g = p.g + (int)(color.g * w);
            int b = p.b + (int)(color.b * w);
            p.r = (unsigned char)std::min(r, 255);
            p.g = (unsigned char)std::min(g, 255);
            p.b = (unsigned char)std::min(b, 255);
            // alpha unused; keep 255
        }
    }
}

void OnDoorToggled_RebakeStaticLights(const Vector3& doorWorldPos,
                                      const std::vector<int>& affectedStaticLightIndices)
{
    // Safety: nothing to do
    if (affectedStaticLightIndices.empty()){
        return;

    }
    const int w = gDynamic.w;
    const int h = gDynamic.h;

    // --- Compute patch region (lightmap pixel rect) ---
    // Use whatever you implemented. This is just a common default:
    // (4 px per tile because you InitDynamicLightmap(dungeonWidth * 4))
    constexpr int PIXELS_PER_TILE = 4;

    // You said 2200 range ~= 11 tiles. If lights have per-light range,
    // you can also compute this dynamically (max tile radius across affected lights).
    constexpr int DEFAULT_RANGE_TILES = 4;
    //this never runs some how


    // Use a radius that matches the “zone that could change”.
    // You can use 2200, or better: the max range of affected lights.
    float patchRadius = 800.0f;

    RectI region = ComputeDoorAffectedRegion_LightmapPixels_UsingStamperMapping(
        doorWorldPos, w, h, patchRadius);


    // EXTREME DEBUG: paint the region bright magenta
    // for (int y = region.y0; y <= region.y1; ++y) {
    //     for (int x = region.x0; x <= region.x1; ++x) {
    //         gStaticBase[y * w + x] = (Color){255, 0, 255, 255};
    //     }
    // }
    // std::cout << "DEBUG: painted region magenta\n";
    // return; // <-- return early so stamping doesn't overwrite it


    // empty region guard (depending on your rect convention)
    if (region.x0 > region.x1 || region.y0 > region.y1){

        return;

    }


    // --- Clear only that region of the static base ---
    ClearStaticBaseRegion(gStaticBase, w, h, region);

    // --- Restamp only affected static lights, clipped to region ---
    for (int idx : affectedStaticLightIndices)
    {
        if (idx < 0 || idx >= (int)dungeonLights.size())
            continue;

        const LightSource& L = dungeonLights[idx];

        Color c = {
            (unsigned char)Clamp(L.colorTint.x * 255.0f * L.intensity, 0.0f, 255.0f),
            (unsigned char)Clamp(L.colorTint.y * 255.0f * L.intensity, 0.0f, 255.0f),
            (unsigned char)Clamp(L.colorTint.z * 255.0f * L.intensity, 0.0f, 255.0f),
            255
        };

        StampLight_StaticBase_Subtile2x2_ToBuffer_Clipped(
            gStaticBase, w, h,
            L.position, L.range, c,
            region
        );
    }

    // No headroom pass (per your preference).
    // Your regular render/update path should copy gStaticBase -> gDynamic each frame,
    // so this patch will show up immediately on the next dynamic build.
}



void BuildStaticLightmapOnce(const std::vector<LightSource>& dungeonLights)
{
    gStaticBase.assign((size_t)gDynamic.w * gDynamic.h, (Color){0,0,0,255});
    for (const auto& L : dungeonLights) {
        Color c = {
            (unsigned char)Clamp(L.colorTint.x * 255.0f * L.intensity, 0.0f, 255.0f),
            (unsigned char)Clamp(L.colorTint.y * 255.0f * L.intensity, 0.0f, 255.0f),
            (unsigned char)Clamp(L.colorTint.z * 255.0f * L.intensity, 0.0f, 255.0f),
            255
        };

        StampLight_StaticBase_Subtile2x2_ToBuffer(gStaticBase, gDynamic.w, gDynamic.h, L.position, L.range, c);

    }

    // headroom so fireballs add nicely:
    for (Color& p : gStaticBase) {
        p.r = (unsigned char)(p.r * 0.65f);
        p.g = (unsigned char)(p.g * 0.65f);
        p.b = (unsigned char)(p.b * 0.65f);
    }

        
}


void BuildDynamicLightmapFromFrameLights(const std::vector<LightSample>& frameLights)
{
    // Start from the static base. Copy the static light values every frame.
    gDynamic.pixels = gStaticBase;

    //skip the player light for now. looks bad when getting close to static lights, makes it too orange.

    //Dynamic player light
    const LightSample ls =  {
        player.position,
        lightConfig.playerColor, 
        600.0f, //hard coded, lightConfig.playerRadius doesn't work?
        lightConfig.playerIntensity,
    };

    Color c = {
        (unsigned char)Clamp(ls.color.x * 255.0f * ls.intensity, 0.0f, 255.0f),
        (unsigned char)Clamp(ls.color.y * 255.0f * ls.intensity, 0.0f, 255.0f),
        (unsigned char)Clamp(ls.color.z * 255.0f * ls.intensity, 0.0f, 255.0f),
        255
    };

    //stamp player light. 
    StampDynamicLight(ls.pos, ls.range, c);  

    // Stamp dynamic movers (fireballs).
    for (const LightSample& L : frameLights) {
        
        Color c = {
            (unsigned char)Clamp(L.color.x * 255.0f * L.intensity, 0.0f, 255.0f),
            (unsigned char)Clamp(L.color.y * 255.0f * L.intensity, 0.0f, 255.0f),
            (unsigned char)Clamp(L.color.z * 255.0f * L.intensity, 0.0f, 255.0f),
            255
        };

        StampDynamicLight(L.pos, L.range, c);
        // No occlusion for fireballs, too expensive. 
    }

    UpdateTexture(gDynamic.tex, gDynamic.pixels.data());



}




