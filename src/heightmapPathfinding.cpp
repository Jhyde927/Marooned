#include "heightmapPathfinding.h"

#include <queue>
#include <limits>
#include <cmath>

// --------------------------------------
// HeightmapNavGrid helpers
// --------------------------------------

bool HeightmapNavGrid::IsInside(int gx, int gz) const
{
    return (gx >= 0 && gx < gridWidth &&
            gz >= 0 && gz < gridHeight);
}

bool HeightmapNavGrid::IsWalkable(int gx, int gz) const
{
    if (!IsInside(gx, gz)) return false;
    int idx = gz * gridWidth + gx;
    if (idx < 0 || idx >= (int)walkable.size()) return false;
    return walkable[idx];
}

Vector3 HeightmapNavGrid::CellToWorldCenter(int gx, int gz, float terrainScaleY) const
{
    // XZ center
    float worldX = worldOrigin.x + (gx + 0.5f) * cellWorldSizeX;
    float worldZ = worldOrigin.y + (gz + 0.5f) * cellWorldSizeZ;

    // Height lookup: stored as [0..1], scale to world Y if desired
    float y = 0.0f;
    int idx = gz * gridWidth + gx;
    if (idx >= 0 && idx < (int)heightSamples.size())
    {
        y = heightSamples[idx] * terrainScaleY;
    }

    return { worldX, y, worldZ };
}

void HeightmapNavGrid::WorldToCell(const Vector3& worldPos, int& outX, int& outZ) const
{
    float localX = worldPos.x - worldOrigin.x;
    float localZ = worldPos.z - worldOrigin.y;

    outX = (int)std::floor(localX / cellWorldSizeX);
    outZ = (int)std::floor(localZ / cellWorldSizeZ);
}

// --------------------------------------
// Namespace HeightmapPathfinding
// --------------------------------------

namespace HeightmapPathfinding
{
    // Helper to sample a grayscale height from the original heightmap at normalized coords
    static float SampleHeightNormalized(const Image& img, float u, float v)
    {
        // Clamp
        if (u < 0.0f) u = 0.0f;
        if (u > 1.0f) u = 1.0f;
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;

        int ix = (int)(u * (img.width  - 1));
        int iy = (int)(v * (img.height - 1));

        // Assuming PIXELFORMAT_UNCOMPRESSED_GRAYSCALE (1 byte per pixel)
        unsigned char* data = (unsigned char*)img.data;
        int idx = iy * img.width + ix;

        unsigned char value = data[idx];
        return (float)value / 255.0f; // 0..1
    }

    HeightmapNavGrid BuildNavGridFromHeightmap(
        const Image& heightmap,
        int gridWidth,
        int gridHeight,
        float seaLevel,
        float worldSizeX,
        float worldSizeZ
    )
    {
        HeightmapNavGrid nav;
        nav.gridWidth  = gridWidth;
        nav.gridHeight = gridHeight;
        nav.seaLevel   = seaLevel;

        nav.cellWorldSizeX = worldSizeX / (float)gridWidth;
        nav.cellWorldSizeZ = worldSizeZ / (float)gridHeight;

        // You can adjust origin if your world isn't centered this way
        nav.worldOrigin = {
            -worldSizeX * 0.5f,
            -worldSizeZ * 0.5f
        };

        nav.heightSamples.resize(gridWidth * gridHeight);
        nav.walkable.resize(gridWidth * gridHeight);

        // For now, we only use height vs sea level.
        // You can add slope checks or obstacles later.
        for (int gz = 0; gz < gridHeight; ++gz)
        {
            for (int gx = 0; gx < gridWidth; ++gx)
            {
                float u = (gx + 0.5f) / (float)gridWidth;
                float v = (gz + 0.5f) / (float)gridHeight;

                float h = SampleHeightNormalized(heightmap, u, v);
                int idx = gz * gridWidth + gx;

                nav.heightSamples[idx] = h;

                // Basic walkability: above sea-level is candidate walkable
                //sea level is set to 60, so they can come right up to waters edge.
                bool walkable = (h > seaLevel);

                // TODO: add slope checks here if you want to block cliffs

                nav.walkable[idx] = walkable;
            }
        }

        return nav;
    }

    bool FindPathBFS(
        const HeightmapNavGrid& nav,
        const Vector3& startWorld,
        const Vector3& goalWorld,
        float terrainScaleY,
        std::vector<Vector3>& outPath
    )
    {
        outPath.clear();

        int startX, startZ;
        int goalX,  goalZ;
        nav.WorldToCell(startWorld, startX, startZ);
        nav.WorldToCell(goalWorld,  goalX,  goalZ);

        if (!nav.IsWalkable(startX, startZ) || !nav.IsWalkable(goalX, goalZ))
        {
            return false;
        }

        const int w = nav.gridWidth;
        const int h = nav.gridHeight;

        std::vector<bool> visited(w * h, false);
        std::vector<HMCell> parent(w * h, {-1, -1});

        auto indexOf = [w](int gx, int gz)
        {
            return gz * w + gx;
        };

        std::queue<HMCell> q;
        q.push({ startX, startZ });
        visited[indexOf(startX, startZ)] = true;

        bool found = false;

        const HMCell neighbors[8] = { //include diagonals
            { 1,  0}, {-1,  0},
            { 0,  1}, { 0, -1},
            { 1,  1}, { 1, -1},
            {-1,  1}, {-1, -1}
        };

        // const HMCell neighbors[4] = {
        //     { 1,  0},
        //     {-1,  0},
        //     { 0,  1},
        //     { 0, -1}
        // };

        while (!q.empty())
        {
            HMCell current = q.front();
            q.pop();

            if (current.x == goalX && current.z == goalZ)
            {
                found = true;
                break;
            }

            for (const HMCell& n : neighbors)
            {
                int nx = current.x + n.x;
                int nz = current.z + n.z;

                if (!nav.IsWalkable(nx, nz)) continue;

                int idx = indexOf(nx, nz);
                if (visited[idx]) continue;

                visited[idx] = true;
                parent[idx]  = current;
                q.push({ nx, nz });
            }
        }

        if (!found)
        {
            return false;
        }

        // Reconstruct path from goal → start
        std::vector<HMCell> reverseCells;
        {
            HMCell cur = { goalX, goalZ };
            while (!(cur.x == startX && cur.z == startZ))
            {
                reverseCells.push_back(cur);
                int idx = indexOf(cur.x, cur.z);
                cur = parent[idx];
            }
            reverseCells.push_back({ startX, startZ });
        }

        // Reverse into outPath and convert to world positions
        outPath.reserve(reverseCells.size());
        for (int i = (int)reverseCells.size() - 1; i >= 0; --i)
        {
            const HMCell& c = reverseCells[i];
            Vector3 wp = nav.CellToWorldCenter(c.x, c.z, terrainScaleY);
            outPath.push_back(wp);
        }

        return true;
    }
}
