#version 330

in vec2 vUV;
in vec3 vWorldPos;
in vec4 vColor;
in vec3 vNormal;

out vec4 finalColor;

uniform sampler2D texture0;   // Albedo (ceiling tiles) - MATERIAL_MAP_DIFFUSE
uniform vec4      colDiffuse;

uniform sampler2D texture4;   // Dynamic lightmap - MATERIAL_MAP_EMISSION
uniform sampler2D texture3;   // Void mask - MATERIAL_MAP_OCCLUSION

uniform vec4  gridBounds;     // { minX, minZ, invSizeX, invSizeZ }
uniform float dynStrength;
uniform float ambientBoost;

// Separate these:
uniform vec2 u_GridSize;      // (dungeonWidth, dungeonHeight) in tiles
uniform vec2 u_TilingXZ;      // how many times the ceiling texture repeats across the plane

void main()
{
    // -----------------------------
    // 1) MASK UV (tile space)
    // -----------------------------
    // Plane UVs represent dungeon footprint but your PNG/world mapping is flipped on both axes.
    vec2 uvMask01 = vec2(1.0 - vUV.x, 1.0 - vUV.y);
    uvMask01 = clamp(uvMask01, 0.0, 1.0);

    // Snap mask sampling to tile centers.
    vec2 gridSize = max(u_GridSize, vec2(1.0));
    vec2 tile   = floor(uvMask01 * gridSize);
    vec2 maskUV = (tile + vec2(0.5)) / gridSize;

    // Optional ceiling void discard.
    // float isVoid = texture(texture3, maskUV).r;
    // if (isVoid > 0.5) discard;

    // -----------------------------
    // 2) LIGHTMAP UV (world space)
    // -----------------------------
    vec2 uvLight01 = vec2(
        (vWorldPos.x - gridBounds.x) * gridBounds.z,
        (vWorldPos.z - gridBounds.y) * gridBounds.w
    );
    uvLight01 = clamp(uvLight01, 0.0, 1.0);

    // -----------------------------
    // 3) CEILING TEXTURE TILING
    // -----------------------------
    // Use the same orientation as the mask so ceiling tiles align with the dungeon.
    vec2 uvTiled = uvMask01 * u_TilingXZ;

    vec4 baseS  = texture(texture0, uvTiled);
    vec3 base   = baseS.rgb * colDiffuse.rgb * vColor.rgb;
    float alpha = baseS.a   * colDiffuse.a  * vColor.a;

    // -----------------------------
    // 4) LIGHTING
    // -----------------------------
    // The lightmap already contains RGB color.
    vec3 dyn = texture(texture4, uvLight01).rgb;
    vec3 L   = clamp(dyn * dynStrength + vec3(ambientBoost), 0.0, 1.0);

    vec3 lit = base * L;

    finalColor = vec4(lit, alpha);
}