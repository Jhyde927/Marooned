#version 330

in vec2 vUV;
in vec3 vWorldPos;
in vec4 vColor;
in vec3 vNormal;

out vec4 finalColor;

uniform sampler2D texture0;   // Albedo
uniform vec4      colDiffuse;

// Lightmap from emission slot
uniform sampler2D texture4;

// { minX, minZ, invSizeX, invSizeZ }
uniform vec4 gridBounds;
uniform float dynStrength;
uniform float ambientBoost;

void main()
{
    // --- Albedo ---
    vec4 baseS = texture(texture0, vUV);
    vec3 base  = baseS.rgb * colDiffuse.rgb;
    float alpha = baseS.a * colDiffuse.a;

    // --- Lightmap UV ---
    float u = (vWorldPos.x - gridBounds.x) * gridBounds.z;
    float v = (vWorldPos.z - gridBounds.y) * gridBounds.w;
    vec2 lmUV = clamp(vec2(u, v), 0.0, 1.0);

    // --- Dynamic lightmap sample ---
    vec3 dyn = texture(texture4, lmUV).rgb;
    vec3 L   = clamp(dyn * dynStrength + vec3(ambientBoost), 0.0, 1.0);

    // --- Wall detection using normal ---
    vec3 N = normalize(vNormal);
    float wallFactor = 1.0 - abs(N.y);

    float wallBoost = 1.1;
    L *= mix(1.0, wallBoost, wallFactor);

    // --- Warm tint near bright lights ---
    float lightLevel = max(L.r, max(L.g, L.b));
    vec3  torchColor = vec3(1.0, 0.75, 0.6);

    float baseTint = smoothstep(0.1, 1.0, lightLevel);
    float hotspot  = smoothstep(0.07, 1.0, lightLevel);
    float extraHot = 0.9;

    float tintAmount = baseTint * (1.0 + hotspot * extraHot);
    tintAmount = clamp(tintAmount, 0.0, 2.0);

    vec3 lightTint = mix(vec3(1.0), torchColor, tintAmount);

    vec3 lit = base * (L * lightTint);

    finalColor = vec4(lit, alpha);
}