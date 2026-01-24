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

// NEW: separate these:
uniform vec2 u_GridSize;      // (dungeonWidth, dungeonHeight) in tiles
uniform vec2 u_TilingXZ;      // how many times the ceiling texture repeats across the plane

void main()
{
    // -----------------------------
    // 1) MASK UV (tile space)
    // -----------------------------
    // Plane UVs represent dungeon footprint but your PNG/world mapping is flipped on both axes:
    vec2 uvMask01 = vec2(1.0 - vUV.x, 1.0 - vUV.y);
    uvMask01 = clamp(uvMask01, 0.0, 1.0);

    // Snap mask sampling to tile centers (prevents gray blending / random cutouts)
    vec2 gridSize = max(u_GridSize, vec2(1.0));
    vec2 tile   = floor(uvMask01 * gridSize);
    vec2 maskUV = (tile + vec2(0.5)) / gridSize;

    float isVoid = texture(texture3, maskUV).r; // 0 or 1-ish
    if (isVoid > 0.5) discard;

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
    // Use the *same orientation as the mask* so ceiling tiles visually align with your dungeon
    vec2 uvTiled = uvMask01 * u_TilingXZ;

    vec4 baseS  = texture(texture0, uvTiled);
    vec3 base   = baseS.rgb * colDiffuse.rgb * vColor.rgb;
    float alpha = baseS.a   * colDiffuse.a  * vColor.a;

    // Dynamic lighting (same mapping as floors/walls)
    vec3 dyn = texture(texture4, uvLight01).rgb;
    vec3 L   = clamp(dyn * dynStrength + vec3(ambientBoost), 0.0, 1.0);

    // Warm tint near bright lights
    float lightLevel = max(L.r, max(L.g, L.b));
    vec3  torchColor = vec3(1.0, 0.75, 0.6);

    float baseTint  = smoothstep(0.1, 1.0, lightLevel);
    float hotspot   = smoothstep(0.07, 1.0, lightLevel);
    float extraHot  = 0.9;

    float tintAmount = clamp(baseTint * (1.0 + hotspot * extraHot), 0.0, 2.0);
    vec3  lightTint  = mix(vec3(1.0), torchColor, tintAmount);

    vec3 lit = base * (L * lightTint);

    finalColor = vec4(lit, alpha);
}


// #version 330

// in vec2 vUV;
// in vec3 vWorldPos;
// in vec4 vColor;
// in vec3 vNormal;

// out vec4 finalColor;

// uniform sampler2D texture0;   // Albedo (ceiling tiles)
// uniform vec4      colDiffuse; // raylib material tint

// uniform sampler2D texture4;   // Lightmap (emission slot)

// // NEW: bind void mask to MATERIAL_MAP_OCCLUSION -> texture3
// uniform sampler2D texture3;   // Void mask (255=void, 0=solid)

// //uniform sampler2D voidMaskTex;

// uniform vec4  gridBounds;     // (no longer needed for uv01, but you can keep it)
// uniform float dynStrength;
// uniform float ambientBoost;

// uniform vec2 u_TilingXZ;      // (dungeonWidth, dungeonHeight)

// void main()
// {
//     // --- Use plane UVs as 0..1 across dungeon footprint ---
//     vec2 uv01 = vec2(1.0 - vUV.x, 1.0 - vUV.y); //flip box X and Y
//     vec2 uvClamped = clamp(uv01, 0.0, 1.0);

//     vec2 gridSize = max(u_TilingXZ, vec2(1.0));
//     vec2 tile   = floor(uvClamped * gridSize);
//     vec2 maskUV = (tile + vec2(0.5)) / gridSize;

//     float isVoid = texture(texture3, maskUV).r;  // <-- use explicit sampler
//     if (isVoid > 0.5) discard;

//     // --- Lightmap UV must be clamped ---
//     vec2 lmUV = uvClamped;

//     // --- Tiled ceiling UVs ---
//     vec2 uvTiled = uv01 * u_TilingXZ;

//     vec4 baseS  = texture(texture0, uvTiled);
//     vec3 base   = baseS.rgb * colDiffuse.rgb * vColor.rgb;
//     float alpha = baseS.a   * colDiffuse.a  * vColor.a;

//     vec3 dyn = texture(texture4, lmUV).rgb;
//     vec3 L   = clamp(dyn * dynStrength + vec3(ambientBoost), 0.0, 1.0);

//     vec3 N = normalize(vNormal);
//     float wallFactor = 1.0 - abs(N.y);

//     float lightLevel = max(L.r, max(L.g, L.b));
//     vec3  torchColor = vec3(1.0, 0.75, 0.6);

//     float baseTint  = smoothstep(0.1, 1.0, lightLevel);
//     float hotspot   = smoothstep(0.07, 1.0, lightLevel);
//     float extraHot  = 0.9;

//     float tintAmount = clamp(baseTint * (1.0 + hotspot * extraHot), 0.0, 2.0);
//     vec3  lightTint  = mix(vec3(1.0), torchColor, tintAmount);

//     vec3 lit = base * (L * lightTint);
//     finalColor = vec4(lit, alpha);

// }



// #version 330

// in vec2 vUV;
// in vec3 vWorldPos;
// in vec4 vColor;
// in vec3 vNormal;

// out vec4 finalColor;

// uniform sampler2D texture0;   // Albedo (ceiling tiles)
// uniform vec4      colDiffuse; // raylib material tint

// uniform sampler2D texture4;   // Lightmap (emission slot)

// uniform vec4  gridBounds;     // { minX, minZ, invSizeX, invSizeZ }
// uniform float dynStrength;
// uniform float ambientBoost;

// // NEW: repeats across dungeon footprint
// uniform vec2 u_TilingXZ;      // e.g. (dungeonWidth, dungeonHeight)

// void main()
// {
//     //--- 0..1 across dungeon footprint in world space ---
//     vec2 uv01 = vec2(
//         (vWorldPos.x - gridBounds.x) * gridBounds.z,
//         (vWorldPos.z - gridBounds.y) * gridBounds.w
//     );

//     // Lightmap UV must be clamped, always
//     vec2 lmUV = clamp(uv01, 0.0, 1.0);

//     // For tiling, we *can* keep uv01 unclamped (optional),
//     // but clamping is fine as long as you're mapping exactly within bounds.
//     vec2 uvTiled = uv01 * u_TilingXZ;

//     // --- Albedo (world-projected + tiled) ---
//     vec4 baseS  = texture(texture0, uvTiled);
//     vec3 base   = baseS.rgb * colDiffuse.rgb * vColor.rgb;
//     float alpha = baseS.a   * colDiffuse.a  * vColor.a;

//     // --- Lightmap sample ---
//     vec3 dyn = texture(texture4, lmUV).rgb;
//     vec3 L   = clamp(dyn * dynStrength + vec3(ambientBoost), 0.0, 1.0);
//     //vec3 L = vec3(1.0); // force full bright
//     // --- Wall detection using normal ---
//     vec3 N = normalize(vNormal);
//     float wallFactor = 1.0 - abs(N.y);   // 1 = wall, 0 = floor/ceiling

//     // float wallBoost = 1.1;
//     // L *= mix(1.0, wallBoost, wallFactor);

//     // --- Warm tint near bright lights ---
//     float lightLevel = max(L.r, max(L.g, L.b));
//     vec3  torchColor = vec3(1.0, 0.75, 0.6);

//     float baseTint  = smoothstep(0.1, 1.0, lightLevel);
//     float hotspot   = smoothstep(0.07, 1.0, lightLevel);
//     float extraHot  = 0.9;

//     float tintAmount = clamp(baseTint * (1.0 + hotspot * extraHot), 0.0, 2.0);
//     vec3  lightTint  = mix(vec3(1.0), torchColor, tintAmount);

//     vec3 lit = base * (L * lightTint);

//     finalColor = vec4(lit, alpha);

//     //finalColor = vec4(fract(vUV), 0.0, 1.0);


// }
