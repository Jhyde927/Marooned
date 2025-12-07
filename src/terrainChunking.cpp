#include "terrainChunking.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "raymath.h"
#include "world.h"
#include "viewCone.h"

TerrainGrid terrain;

static inline unsigned char SampleGray(const Image& img, int x, int z) {
    x = std::clamp(x, 0, img.width  - 1);
    z = std::clamp(z, 0, img.height - 1);
    const unsigned char* px = reinterpret_cast<unsigned char*>(img.data);
    return px[z * img.width + x]; // PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
}

static inline float HeightMeters(const Image& img, int x, int z, float heightScale) {
    return (SampleGray(img, x, z) / 255.0f) * heightScale;
}

static inline Vector3 ComputeNormalCd(const Image& hm, int x, int z, float heightScale, float cellX, float cellZ) {
    float hL = HeightMeters(hm, x - 1, z    , heightScale);
    float hR = HeightMeters(hm, x + 1, z    , heightScale);
    float hD = HeightMeters(hm, x    , z - 1, heightScale);
    float hU = HeightMeters(hm, x    , z + 1, heightScale);

    // central differences: slope scaled by world cell size
    Vector3 n = {
        -(hR - hL) / (2.0f * cellX),
         1.0f,
        -(hU - hD) / (2.0f * cellZ)
    };
    return Vector3Normalize(n);
}

//static inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

TerrainGrid BuildTerrainGridFromHeightmap(const Image& heightmapGray, Vector3 terrainScale, int tileRes, bool addSkirt) {
    TerrainGrid T{};
    T.terrainScale = terrainScale;
    T.tileRes      = std::max(5, tileRes);
    T.heightmapW   = heightmapGray.width;
    T.heightmapH   = heightmapGray.height;

    const int W = heightmapGray.width;
    const int H = heightmapGray.height;

    // World spans from -X/2..+X/2 and -Z/2..+Z/2, like your current DrawModel offset.
    const float worldX = terrainScale.x;
    const float worldZ = terrainScale.z;
    const float heightY = terrainScale.y;

    // world-space size per height texel
    const float cellX = worldX / float(W - 1);
    const float cellZ = worldZ / float(H - 1);

    // We want chunks with (tileRes x tileRes) samples in the *core* (no skirt),
    // because triangle count per chunk ~ 2*(tileRes-1)^2 is manageable.
    const int coreCell = T.tileRes - 1;
    T.tilesX = (W - 1 + coreCell - 1) / coreCell; // ceil((W-1)/coreCell)
    T.tilesZ = (H - 1 + coreCell - 1) / coreCell;

    T.chunks.reserve(T.tilesX * T.tilesZ);

    for (int tz = 0; tz < T.tilesZ; ++tz) {
        for (int tx = 0; tx < T.tilesX; ++tx) {

            // core sample rect [x0..x1], [z0..z1] inclusive
            const int x0 = tx * coreCell;
            const int z0 = tz * coreCell;
            const int x1 = std::min(x0 + coreCell, W - 1);
            const int z1 = std::min(z0 + coreCell, H - 1);

            // expand by 1 on each side for skirts, clamped to the image
            const int k  = addSkirt ? 1 : 0;
            const int sx0 = std::max(0,     x0 - k);
            const int sz0 = std::max(0,     z0 - k);
            const int sx1 = std::min(W - 1, x1 + k);
            const int sz1 = std::min(H - 1, z1 + k);

            const int vertsX = (sx1 - sx0) + 1;
            const int vertsZ = (sz1 - sz0) + 1;
            const int numVerts = vertsX * vertsZ;

            const int cellsX = vertsX - 1;
            const int cellsZ = vertsZ - 1;
            const int numTris = cellsX * cellsZ * 2;
            const int numIdx  = numTris * 3;

            // CPU buffers
            std::vector<float> vtx(numVerts * 3);
            std::vector<float> nrm(numVerts * 3);
            std::vector<float> uv (numVerts * 2);
            std::vector<unsigned short> idx(numIdx);

            // Build vertices
            int v = 0;
            for (int zz = 0; zz < vertsZ; ++zz) {
                for (int xx = 0; xx < vertsX; ++xx) {
                    const int hx = sx0 + xx;
                    const int hz = sz0 + zz;

                    const float tX = hx / float(W - 1); // 0..1 across full world
                    const float tZ = hz / float(H - 1);

                    float x = Lerp(-worldX * 0.5f, +worldX * 0.5f, tX);
                    float z = Lerp(-worldZ * 0.5f, +worldZ * 0.5f, tZ);
                    float y = HeightMeters(heightmapGray, hx, hz, heightY);

                    // If this vertex lies on the skirt border, push it slightly down
                    if (addSkirt) {
                        const bool onLeft   = (hx == sx0) && (sx0 < x0);
                        const bool onRight  = (hx == sx1) && (sx1 > x1);
                        const bool onTop    = (hz == sz0) && (sz0 < z0);
                        const bool onBottom = (hz == sz1) && (sz1 > z1);
                        if (onLeft || onRight || onTop || onBottom) {
                            y -= 0.20f; // small drop prevents cracks
                        }
                    }

                    vtx[v*3 + 0] = x;
                    vtx[v*3 + 1] = y;
                    vtx[v*3 + 2] = z;

                    const Vector3 n = ComputeNormalCd(heightmapGray, hx, hz, heightY, cellX, cellZ);
                    nrm[v*3 + 0] = n.x;
                    nrm[v*3 + 1] = n.y;
                    nrm[v*3 + 2] = n.z;

                    // UVs across the entire world (0..1). If you want per-tile tiling, swap later.
                    uv[v*2 + 0] = tX;
                    uv[v*2 + 1] = tZ;

                    ++v;
                }
            }

            // Build indices (two tris per cell)
            int ii = 0;
            for (int zc = 0; zc < cellsZ; ++zc) {
                for (int xc = 0; xc < cellsX; ++xc) {
                    unsigned short i0 = (unsigned short)((zc    ) * vertsX + (xc    ));
                    unsigned short i1 = (unsigned short)((zc    ) * vertsX + (xc + 1));
                    unsigned short i2 = (unsigned short)((zc + 1) * vertsX + (xc    ));
                    unsigned short i3 = (unsigned short)((zc + 1) * vertsX + (xc + 1));

                    // CCW
                    idx[ii++] = i0; idx[ii++] = i2; idx[ii++] = i1;
                    idx[ii++] = i1; idx[ii++] = i2; idx[ii++] = i3;
                }
            }

            // Upload to GPU -> Mesh -> Model
            Mesh mesh = { 0 };
            mesh.vertexCount   = numVerts;
            mesh.triangleCount = numTris;

            mesh.vertices  = (float*)MemAlloc(vtx.size() * sizeof(float));
            mesh.normals   = (float*)MemAlloc(nrm.size() * sizeof(float));
            mesh.texcoords = (float*)MemAlloc(uv.size()  * sizeof(float));
            mesh.indices   = (unsigned short*)MemAlloc(idx.size() * sizeof(unsigned short));

            std::memcpy(mesh.vertices,  vtx.data(), vtx.size() * sizeof(float));
            std::memcpy(mesh.normals,   nrm.data(), nrm.size() * sizeof(float));
            std::memcpy(mesh.texcoords, uv.data(),  uv.size()  * sizeof(float));
            std::memcpy(mesh.indices,   idx.data(), idx.size() * sizeof(unsigned short));

            // Compute AABB BEFORE freeing CPU arrays
            BoundingBox bb = GetMeshBoundingBox(mesh);

            UploadMesh(&mesh, false); // create VAO/VBOs

            Model model = LoadModelFromMesh(mesh);
            // mesh CPU arrays are already on the model's GPU buffers; free CPU copies:
            RL_FREE(mesh.vertices);  mesh.vertices  = nullptr;
            RL_FREE(mesh.normals);   mesh.normals   = nullptr;
            RL_FREE(mesh.texcoords); mesh.texcoords = nullptr;
            RL_FREE(mesh.indices);   mesh.indices   = nullptr;

            // Chunk center from AABB
            Vector3 ctr = {
                (bb.min.x + bb.max.x) * 0.5f,
                (bb.min.y + bb.max.y) * 0.5f,
                (bb.min.z + bb.max.z) * 0.5f
            };

            TerrainChunk chunk{};
            chunk.model  = model;
            chunk.aabb   = bb;
            chunk.center = ctr;

            T.chunks.push_back(chunk);
        }
    }

    return T;
}

void BuildTerrainChunkDrawList(
    const TerrainGrid& T,
    const Camera3D& cam,
    float maxDrawDist,
    int maxChunksToDraw,
    std::vector<ChunkDrawInfo>& outList)
{
    outList.clear();

    // Shared cone parameters (new system)
    ViewConeParams vp = MakeViewConeParams(
        cam,
        60.0f,      // half-FOV (you previously used 60)
        maxDrawDist,
        1600.0f     // near chunk distance (old NEAR_CHUNK_DIST)
    );

    const float WATER_HEIGHT = 55.0f;

    // 1) Collect candidates
    for (const TerrainChunk& c : T.chunks)
    {
        // First do distance + water tests (fast)
        Vector3 toChunk = Vector3Subtract(c.center, vp.camPos);
        float distSq = Vector3LengthSqr(toChunk);

        if (distSq > vp.maxDistSq)
            continue;

        if (c.aabb.max.y < WATER_HEIGHT && !player.isSwimming)
            continue;

        // Use new cone function for visibility
        if (!IsInViewCone(vp, c.center))
            continue;

        // Add to draw list
        outList.push_back({ &c, distSq });
    }

    // 2) Sort by distance
    std::sort(outList.begin(), outList.end(),
        [](const ChunkDrawInfo& a, const ChunkDrawInfo& b){
            return a.distSq < b.distSq;
        });

    // 3) Hard cap
    if ((int)outList.size() > maxChunksToDraw)
        outList.resize(maxChunksToDraw);
}



void DrawTerrainGrid(const TerrainGrid& T, const Camera3D& cam, float maxDrawDist) {

    rlEnableBackfaceCulling();
    int maxChunksToDraw = 250;
    static std::vector<ChunkDrawInfo> drawList;
    BuildTerrainChunkDrawList(T, cam, maxDrawDist, maxChunksToDraw, drawList);

    for (const ChunkDrawInfo& info : drawList) {
        const TerrainChunk* c = info.chunk;
        DrawModel(c->model, Vector3Zero(), 1.0f, WHITE);
    }

    rlDisableBackfaceCulling();
}



// Frees VAO/VBOs and the Model arrays without touching textures.
// After this, 'm' is an empty shell and safe to discard.
static void SafeDestroyModel(Model& m) {
    // 1) clear any textures so we don't double free shared ones
    for (int i = 0; i < m.materialCount; ++i) {
        for (int mi = 0; mi < 12; ++mi)
            m.materials[i].maps[mi].texture.id = 0;
    }

    // 2) free VAO/VBOs
    for (int i = 0; i < m.meshCount; ++i) {
        Mesh& mesh = m.meshes[i];

        if (mesh.vaoId)
            rlUnloadVertexArray(mesh.vaoId);

        // Raylib normally keeps up to 4 vertex buffers per mesh
        for (int b = 0; b < 4; ++b) {
            if (mesh.vboId[b])
                rlUnloadVertexBuffer(mesh.vboId[b]);
        }
    }

    // 3) release CPU-side arrays
    if (m.meshes)    { RL_FREE(m.meshes);    m.meshes    = nullptr; }
    if (m.materials) { RL_FREE(m.materials); m.materials = nullptr; }
    m.meshCount = 0;
    m.materialCount = 0;
}


void UnloadTerrainGrid(TerrainGrid& T) {
    for (auto& c : T.chunks) {
        SafeDestroyModel(c.model); //was crashing when trying to unload chunked terrain models. 
    }

    T.chunks.clear();
    T.tilesX = T.tilesZ = 0;
}
