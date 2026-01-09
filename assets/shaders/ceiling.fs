#version 330

in vec2 vUV;
in vec3 vWorldPos;
in vec4 vColor;
in vec3 vNormal;

out vec4 finalColor;

uniform sampler2D texture0;   // Albedo (ceiling tiles)
uniform vec4      colDiffuse; // raylib material tint

uniform sampler2D texture4;   // Lightmap (emission slot)

uniform vec4  gridBounds;     // { minX, minZ, invSizeX, invSizeZ }
uniform float dynStrength;
uniform float ambientBoost;

// NEW: repeats across dungeon footprint
uniform vec2 u_TilingXZ;      // e.g. (dungeonWidth, dungeonHeight)

void main()
{
    //--- 0..1 across dungeon footprint in world space ---
    vec2 uv01 = vec2(
        (vWorldPos.x - gridBounds.x) * gridBounds.z,
        (vWorldPos.z - gridBounds.y) * gridBounds.w
    );

    // Lightmap UV must be clamped, always
    vec2 lmUV = clamp(uv01, 0.0, 1.0);

    // For tiling, we *can* keep uv01 unclamped (optional),
    // but clamping is fine as long as you're mapping exactly within bounds.
    vec2 uvTiled = uv01 * u_TilingXZ;

    // --- Albedo (world-projected + tiled) ---
    vec4 baseS  = texture(texture0, uvTiled);
    vec3 base   = baseS.rgb * colDiffuse.rgb * vColor.rgb;
    float alpha = baseS.a   * colDiffuse.a  * vColor.a;

    // --- Lightmap sample ---
    vec3 dyn = texture(texture4, lmUV).rgb;
    vec3 L   = clamp(dyn * dynStrength + vec3(ambientBoost), 0.0, 1.0);
    //vec3 L = vec3(1.0); // force full bright
    // --- Wall detection using normal ---
    vec3 N = normalize(vNormal);
    float wallFactor = 1.0 - abs(N.y);   // 1 = wall, 0 = floor/ceiling

    // float wallBoost = 1.1;
    // L *= mix(1.0, wallBoost, wallFactor);

    // --- Warm tint near bright lights ---
    float lightLevel = max(L.r, max(L.g, L.b));
    vec3  torchColor = vec3(1.0, 0.75, 0.6);

    float baseTint  = smoothstep(0.1, 1.0, lightLevel);
    float hotspot   = smoothstep(0.07, 1.0, lightLevel);
    float extraHot  = 0.9;

    float tintAmount = clamp(baseTint * (1.0 + hotspot * extraHot), 0.0, 2.0);
    vec3  lightTint  = mix(vec3(1.0), torchColor, tintAmount);

    vec3 lit = base * (L * lightTint);

    finalColor = vec4(lit, alpha);

    //finalColor = vec4(fract(vUV), 0.0, 1.0);


}
