#version 330

in vec2 vUV;
in vec3 vWorldPos;
in vec4 vColor;          // For floors, set tint to WHITE and ignore it here

out vec4 finalColor;

uniform sampler2D texture0;        // floor albedo
uniform vec4      colDiffuse;      // set to WHITE for floors

uniform sampler2D dynamicGridTex;  // the single XZ lightmap (RGB = lights, A = lava mask)

// gridBounds = { minX, minZ, invSizeX, invSizeZ }
uniform vec4  gridBounds;

// Lighting knobs
uniform float dynStrength;     // 0..2 (default 1.0)
uniform float ambientBoost;    // e.g., 0.02–0.08

// New lava/ceiling knobs
uniform int   isCeiling;       // 0 = floor, 1 = ceiling
uniform float lavaCeilStrength; // how much lava boosts ceilings
uniform float ceilHeight;      // world-space Y where ceilings live (for attenuation)
uniform float lavaFalloff;     // how fast lava glow fades with height
                               // (e.g., 200.0 means at 200 units above floor, glow ≈0)

void main() {
    // Albedo (ignore vColor tint for floors if colDiffuse = WHITE)
    vec4 baseS = texture(texture0, vUV);
    vec3 base  = baseS.rgb * colDiffuse.rgb;
    float alpha = baseS.a * colDiffuse.a;

    // World XZ -> dynamic lightmap UV
    float u = (vWorldPos.x - gridBounds.x) * gridBounds.z;
    float v = (vWorldPos.z - gridBounds.y) * gridBounds.w;
    vec2  lmUV = clamp(vec2(u, v), vec2(0.0), vec2(1.0));
    //vec2 lmUV = clamp(vec2(u, 1.0 - v), vec2(0.0), vec2(1.0));

    // Sample lightmap once: RGB = dynamic light, A = lava mask
    vec4 lm = texture(dynamicGridTex, lmUV);
    vec3 dyn = lm.rgb;
    float lavaMask = lm.a; // 0..1

    // Base lighting (same as before)
    vec3 L = clamp(dyn * dynStrength + vec3(ambientBoost), 0.0, 1.0);

    // If drawing ceilings, add lava glow based on alpha mask
    if (isCeiling == 1) {
        // Vertical attenuation so glow fades the higher the ceiling
        float distY = max(0.0, vWorldPos.y - ceilHeight);
        float atten = exp(-distY / lavaFalloff); // exponential falloff

        // Add emissive red/orange glow
        vec3 lavaGlow = vec3(1.0, 0.25, 0.0) * lavaMask * lavaCeilStrength * atten;
        L = clamp(L + lavaGlow, 0.0, 1.0);
    }

    // Shade final
    finalColor = vec4(base * L, alpha);
}
