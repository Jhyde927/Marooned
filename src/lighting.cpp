#include "lighting.h"
#include "raylib.h"
#include "pathfinding.h"
#include "dungeonGeneration.h"
#include "raymath.h"
#include "cfloat"
#include "stdint.h"
#include "world.h"
#include "utilities.h"


BakedLightmap gDynamic; 

static std::vector<Color> gStaticBase;   // same w*h as gDynamic

Texture2D gDungeonOcclusionTex; //forward
LightingConfig lightConfig;


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

        if (HasWorldLineOfSight(from, to, epsilonFrac, LOSMode::Lighting)) ++visible;
    }
    return (float)visible / (float)rays;
}


// --- 2×2 sub-tile visibility using your world-ray LOS fan ---
static void SubtileVis2x2(float vis[2][2],
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

            //if (TileNearSolid(tx, tz, dungeonImg)) { //dont do this check for better results. 
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

                    // falloff (smootherstep)
                    const float t = 1.0f - (d2 / r2);
                    if (t <= 0.0f) continue;
                    float w = t*t*(3.0f - 2.0f*t);

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

void InitDungeonLightingBounds()
{
    // World-space mapping for this level (XZ bounds)
    ComputeDungeonXZBounds(
        dungeonWidth,
        dungeonHeight,
        tileSize,
        floorHeight,          // same baseY you use for floors
        gDynamic.minX,
        gDynamic.minZ,
        gDynamic.sizeX,
        gDynamic.sizeZ
    );

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


static size_t CountNonBlack(const std::vector<Color>& buf) {
    size_t n = 0;
    for (const Color& c : buf) if ( (c.r | c.g | c.b) != 0 ) ++n;
    return n;
}

// Call this right before/after UpdateTexture(...)
void LogDynamicLightmapNonBlack(const char* tag) {
    size_t nb = CountNonBlack(gDynamic.pixels);
    TraceLog(LOG_INFO, "[%s] nonBlack=%zu / %zu  texID=%d  res=%dx%d  bounds={minX=%.2f minZ=%.2f sizeX=%.2f sizeZ=%.2f}",
             tag, nb, gDynamic.pixels.size(),
             gDynamic.tex.id, gDynamic.w, gDynamic.h,
             gDynamic.minX, gDynamic.minZ, gDynamic.sizeX, gDynamic.sizeZ);
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

    //BuildLavaMaskAlphaFromImage(dungeonImg);
        
}

void DebugUploadLightmapOpaque()
{
    std::vector<Color> tmp = gDynamic.pixels;
    for (auto &px : tmp) {
        px.a = 255;        // force opaque for HUD visualization
    }
    UpdateTexture(gDynamic.tex, tmp.data());
}

void BuildDynamicLightmapFromFrameLights(const std::vector<LightSample>& frameLights)
{
    // Start from the static base. Copy the static light values every frame.
    gDynamic.pixels = gStaticBase;

    //Dynamic player light
    const LightSample ls =  {
        player.position,
        Vector3 {1.0f, 1.0f, 1.0f},  //white
        player.lightRange,
        player.lightIntensity,
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
    // if (debugInfo){
    //     DebugUploadLightmapOpaque();
    // } else{
    UpdateTexture(gDynamic.tex, gDynamic.pixels.data());
    //}


}

//forward lighting

void BuildStaticOcclusionTexture()
{

    // 1. Decide dims
    int subtilesX = dungeonWidth * 2;
    int subtilesZ = dungeonHeight * 2;

    const int MAX_STATIC_LIGHTS = 32; // must match GLSL MAX_LIGHTS & "static" limit

    int staticCount = (int)gStaticLightCount;
    int texW = subtilesX;
    int texH = subtilesZ * staticCount;

    // 2. Allocate CPU buffer of Color for the whole thing, init to fully visible (or fully blocked).
    //    Your choice here:
    //    - "0" = default no light, only where LOS exists gets vis>0
    //    - "1" = default visible, and you only zero where LOS says it's blocked
    //    To be close to stamping behavior, treat outside radius as 0 visibility.
    //    So starting from 0 is reasonable.
    std::vector<Color> occPixels((size_t)texW * texH, {0, 0, 0, 255});

    // default fully visible: R=G=B=255
    //std::vector<Color> occPixels((size_t)texW * texH, {255, 255, 255, 255});

    // 3. For each STATIC light, build its layer
    for (int li = 0; li < staticCount; ++li)
    {
        const SimpleLight& L = gDungeonLightsForward[li]; // static lights first

        const Vector3 lightPos = L.pos;
        const float   radius   = L.radius;
        //const float   floorHeight = floorHeight; // whatever you used before for SubtileVis2x2

        float radius2 = radius * radius;

        // Decide tile bounds to consider (like your stamp function)
        int lx = (int)floorf((lightPos.x - gDynamic.minX) / tileSize);
        int lz = (int)floorf((lightPos.z - gDynamic.minZ) / tileSize);
        int R  = (int)ceilf(radius / tileSize);

        int tx0 = std::max(0, lx - R);
        int tx1 = std::min(dungeonWidth  - 1, lx + R);
        int tz0 = std::max(0, lz - R);
        int tz1 = std::min(dungeonHeight - 1, lz + R);

        // For each tile in range, compute 2x2 visibility
        for (int tz = tz0; tz <= tz1; ++tz)
        {
            for (int tx = tx0; tx <= tx1; ++tx)
            {
                // Tile center
                float cx = gDynamic.minX + (tx + 0.5f) * tileSize;
                float cz = gDynamic.minZ + (tz + 0.5f) * tileSize;

                // Circle cull (center)
                float cdx = cx - lightPos.x;
                float cdz = cz - lightPos.z;
                if (cdx*cdx + cdz*cdz > radius2)
                    continue;

                float vis2x2[2][2];
                SubtileVis2x2(vis2x2, lightPos, cx, cz, tileSize, floorHeight*2);

                // If all zero, skip
                if (vis2x2[0][0]==0 && vis2x2[0][1]==0 &&
                    vis2x2[1][0]==0 && vis2x2[1][1]==0)
                {
                    continue;
                }

                for (int sy = 0; sy < 2; ++sy)
                {
                    for (int sx = 0; sx < 2; ++sx)
                    {
                        float vis = vis2x2[sy][sx];
                        if (vis <= 0.0f) continue;

                        int subX = tx * 2 + sx;
                        int subZ = tz * 2 + sy;

                        int imgY = li * subtilesZ + subZ;
                        int imgX = subX;

                        if (imgX < 0 || imgX >= subtilesX) continue;

                        Color& p = occPixels[(size_t)imgY * texW + imgX];

                        // You can choose to overwrite or max with existing.
                        // Max makes sense if LOS is run multiple times, but here we just set it.
                        unsigned char v = (unsigned char)std::round(vis * 255.0f);
                        p.r = v;
                        p.g = v;
                        p.b = v;
                    }
                }
            }
        }
    }

    // 4. Upload to an Image and then to a Texture
    Image occImg = {
        .data = occPixels.data(),
        .width = texW,
        .height = texH,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    // You probably want a global Texture2D gDungeonOcclusionTex;
    if (gDungeonOcclusionTex.id != 0)
        UnloadTexture(gDungeonOcclusionTex);

    gDungeonOcclusionTex = LoadTextureFromImage(occImg);

    // (Optional) set texture filtering to nearest to avoid cross-texel bleeding
    SetTextureFilter(gDungeonOcclusionTex, TEXTURE_FILTER_ANISOTROPIC_16X);
    

    // 5. Bind to dungeon materials via MATERIAL_MAP_OCCLUSION
    Model& floorModel   = R.GetModel("floorTileGray");
    Model& wallModel    = R.GetModel("wallSegment");
    Model& doorwayModel = R.GetModel("doorWayGray");
    Model& launcherModel= R.GetModel("stonePillar");
    Model& barrelModel  = R.GetModel("barrelModel");
    Model& brokeModel   = R.GetModel("brokeBarrel");

    auto setOcc = [&](Model& m) {
        for (int i = 0; i < m.materialCount; ++i)
            m.materials[i].maps[MATERIAL_MAP_OCCLUSION].texture = gDungeonOcclusionTex;
    };

    setOcc(floorModel);
    setOcc(wallModel);
    setOcc(doorwayModel);
    setOcc(launcherModel);
    setOcc(barrelModel);
    setOcc(brokeModel);
}





