// lighting_forward.fs
#version 330

in vec2 vUV;
in vec3 vWorldPos;
in vec4 vColor;    // unused for floors, but passed by raylib

out vec4 finalColor;

uniform sampler2D texture0;   // Albedo (tile texture)
uniform vec4      colDiffuse; // Usually WHITE



// ---- Simple forward lights ----

#define MAX_LIGHTS 32

// xyz = position (world space), w = radius (world units)
uniform vec4 uLightPosRadius[MAX_LIGHTS];

// rgb = color (0..1), a = intensity multiplier
uniform vec4 uLightColorIntensity[MAX_LIGHTS];

// number of active lights in the arrays
uniform int  uLightCount;

// base ambient term (0..1)
uniform float uAmbient;

// Optional helper: same smootherstep curve as your stamper
float smootherStep(float x)
{
    x = clamp(x, 0.0, 1.0);
    return x * x * (3.0 - 2.0 * x);
}

// ---- Occlusion (visibility mask) ----

// Each static light gets its own "layer" in the occlusion texture.
// Texture layout (integer texel space):
//   width  = subtilesX  (dungeonWidth * 2)
//   height = subtilesZ * MAX_STATIC_LIGHTS
// Layer i occupies rows [i*subtilesZ .. (i+1)*subtilesZ-1]
//uniform sampler2D textureOcclusion;
uniform sampler2D texture3;
uniform int   uStaticLightCount; // how many lights at start of array are STATIC
uniform vec2  uDungeonMinXZ;     // (minX, minZ) of dungeon in world space
uniform float uTileSize;         // world size of one tile (same tileSize as CPU)
uniform int   uSubtilesX;        // dungeonWidth * 2
uniform int   uSubtilesZ;        // dungeonHeight * 2


float SampleOcclusion(int lightIndex, vec2 fragXZ)
{
    if (lightIndex < 0 || lightIndex >= uStaticLightCount) return 1.0;

    vec2 rel = (fragXZ - uDungeonMinXZ) / uTileSize;
    float tx = floor(rel.x);
    float tz = floor(rel.y);

    if (tx < 0.0 || tz < 0.0) return 1.0;
    if (tx >= float(uSubtilesX) * 0.5 || tz >= float(uSubtilesZ) * 0.5) return 1.0;

    float lx = rel.x - tx;
    float lz = rel.y - tz;

    int sx = (lx < 0.5) ? 0 : 1;
    int sz = (lz < 0.5) ? 0 : 1;

    int subX = int(tx) * 2 + sx;
    int subZ = int(tz) * 2 + sz;

    int layerOffset = lightIndex * uSubtilesZ;
    int texY = layerOffset + subZ;

    if (subX < 0 || subX >= uSubtilesX) return 1.0;

    // ---- NEW: normalized UVs for bilinear sampling ----
    float texW = float(uSubtilesX);
    float texH = float(uSubtilesZ * uStaticLightCount);

    // sample at texel centers
    vec2 occlUV = vec2((float(subX) + 0.5) / texW,
                       (float(texY) + 0.5) / texH);

    float vis = texture(texture3, occlUV).r;
    return vis;
}


void main()
{
    // Base albedo
    vec4 baseS = texture(texture0, vUV);
    vec3 base  = baseS.rgb * colDiffuse.rgb;
    float alpha = baseS.a * colDiffuse.a;

    // Start with ambient
    vec3 lighting = vec3(uAmbient);

    // We only care about XZ plane for distance and occlusion
    vec2 fragXZ = vWorldPos.xz;

    for (int i = 0; i < uLightCount && i < MAX_LIGHTS; ++i)
    {
        vec3 lp      = uLightPosRadius[i].xyz;
        float radius = uLightPosRadius[i].w;

        vec3 color    = uLightColorIntensity[i].rgb;
        float intensity = uLightColorIntensity[i].a;

        if (radius <= 0.0 || intensity <= 0.0)
            continue;

        vec2 Lxz = fragXZ - lp.xz;
        float dist = length(Lxz);
        if (dist >= radius)
            continue;

        // Same idea as your lightmap: smooth falloff from center to edge
        float t = 1.0 - (dist * dist) / (radius * radius);
        float falloff = smootherStep(t);

        // ---- NEW: occlusion factor ----
        // For ALL lights we compute falloff,
        // but only STATIC lights (i < uStaticLightCount) get occlusion applied.

        
        float vis = 1.0;
        if (i < uStaticLightCount)
        {
            vis = SampleOcclusion(i, fragXZ);
            if (vis <= 0.0)
                continue;
        }

        lighting += color * (intensity * falloff * vis);
    }

    // Clamp a bit so it doesn't blow out
    lighting = clamp(lighting, 0.0, 2.0);

    finalColor = vec4(base * lighting, alpha);


}

// void main()
// {
//     // Base albedo
//     vec4 baseS = texture(texture0, vUV);
//     vec3 base  = baseS.rgb * colDiffuse.rgb;
//     float alpha = baseS.a * colDiffuse.a;

//     // Start with ambient
//     vec3 lighting = vec3(uAmbient);

//     // We only care about XZ plane for distance
//     vec2 fragXZ = vWorldPos.xz;

//     for (int i = 0; i < uLightCount && i < MAX_LIGHTS; ++i)
//     {
//         vec3 lp     = uLightPosRadius[i].xyz;
//         float radius = uLightPosRadius[i].w;

//         vec3 color   = uLightColorIntensity[i].rgb;
//         float intensity = uLightColorIntensity[i].a;

//         if (radius <= 0.0 || intensity <= 0.0)
//             continue;

//         vec2 Lxz = fragXZ - lp.xz;
//         float dist = length(Lxz);
//         if (dist >= radius)
//             continue;

//         // Same idea as your lightmap: smooth falloff from center to edge
//         float t = 1.0 - (dist * dist) / (radius * radius);
//         float falloff = smootherStep(t);

//         lighting += color * (intensity * falloff);
//     }

//     // Clamp a bit so it doesn't blow out
//     lighting = clamp(lighting, 0.0, 2.0);

//     finalColor = vec4(base * lighting, alpha);
// }
