#include "grass.h"
#include "utilities.h"
#include <iostream>

namespace Grass
{
    std::vector<GrassInstanceSource> gGrassInstanceSources;

    void Clear()
    {
        gGrassInstanceSources.clear();
    }

    Matrix BuildGrassTransform(Vector3 position, float yawDeg, float scale)
    {
        Matrix matScale = MatrixScale(scale, scale, scale);
        Matrix matRot   = MatrixRotateY(yawDeg * DEG2RAD);
        Matrix matTrans = MatrixTranslate(position.x, position.y, position.z);

        // Same general order raylib examples use for instancing:
        // scale -> rotate -> translate
        return MatrixMultiply(MatrixMultiply(matScale, matRot), matTrans);
    }

void GenerateFromHeightmap(
    Image& heightmap,
    Vector3 terrainScale,
    float grassSpacing,
    float heightThreshold,
    int maxGrassCards
)
    {
        Clear();
        gGrassInstanceSources.reserve(maxGrassCards);

        if (heightmap.data == nullptr)
        {
            std::cout << "Grass::GenerateFromHeightmap failed: heightmap.data is null\n";
            return;
        }

        Image heightCopy = ImageCopy(heightmap);
        ImageFormat(&heightCopy, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);

        unsigned char* pixels = (unsigned char*)heightCopy.data;

        int step = (int)grassSpacing;
        if (step < 1) step = 1;

        const float jitterAmount = grassSpacing * 0.45f;

        for (int z = 0; z < heightCopy.height; z += step)
        {
            for (int x = 0; x < heightCopy.width; x += step)
            {
                if ((int)gGrassInstanceSources.size() >= maxGrassCards)
                {
                    UnloadImage(heightCopy);
                    return;
                }

                // Randomly offset the sample so the grass does not form a perfect grid.
                int jitterX = GetRandomValue((int)-jitterAmount, (int)jitterAmount);
                int jitterZ = GetRandomValue((int)-jitterAmount, (int)jitterAmount);

                int sampleX = x + jitterX;
                int sampleZ = z + jitterZ;

                sampleX = Clamp(sampleX, 0, heightCopy.width - 1);
                sampleZ = Clamp(sampleZ, 0, heightCopy.height - 1);

                int index = sampleZ * heightCopy.width + sampleX;

                unsigned char heightValue = pixels[index];
                float height01 = heightValue / 255.0f;

                if (height01 < heightThreshold)
                    continue;

                // Optional: skip some valid spots so it looks more natural.
                // Higher number = less grass.
                if (GetRandomValue(0, 100) > 50)
                    continue;

                float worldX = ((float)sampleX / (float)heightCopy.width)  * terrainScale.x - terrainScale.x * 0.5f;
                float worldZ = ((float)sampleZ / (float)heightCopy.height) * terrainScale.z - terrainScale.z * 0.5f;
                float worldY = height01 * terrainScale.y;

                float sinkAmount = (float)GetRandomValue(5, 25);
                worldY -= sinkAmount;

                GrassInstanceSource grass;
                grass.position = { worldX, worldY, worldZ };

                grass.yawDeg = (float)GetRandomValue(0, 359);

                float randomScale = RandomFloat(0.15f, 0.4f);
                grass.scale = randomScale;

                grass.textureIndex = GetRandomValue(0, 3);

                grass.transform = BuildGrassTransform(
                    grass.position,
                    grass.yawDeg,
                    grass.scale
                );

                gGrassInstanceSources.push_back(grass);
            }
        }

        UnloadImage(heightCopy);
    }
}
