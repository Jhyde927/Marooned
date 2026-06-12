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

// Normal/global ambient used by everything
uniform float ambientBoost;

// Extra readability boost, mostly for props.
// Set this to 0.0 for normal dungeon geometry.
uniform float propAmbientBoost;

// Clamp final brightness so boosted props do not blow out near lights.
// Good values:
// normal geometry: 1.0
// props: 1.15 to 1.35
uniform float maxBrightness;

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
    // The lightmap already contains RGB light color.
    vec3 dyn = texture(texture4, lmUV).rgb;

    // Base lighting:
    // dynStrength controls lightmap intensity.
    // ambientBoost is the normal scene-wide ambient.
    vec3 L = dyn * dynStrength + vec3(ambientBoost);

    // --- Wall detection using normal ---
    vec3 N = normalize(vNormal);
    float wallFactor = 1.0 - abs(N.y);   // 1.0 = vertical wall, 0.0 = floor/ceiling

    // Wall brightness boost
    float wallBoost = 1.1;
    L *= mix(1.0, wallBoost, wallFactor);

    // Clamp lighting before applying base texture.
    // This keeps normal light contribution sane.
    L = clamp(L, 0.0, 1.0);

    // --- Final lighting ---
    vec3 lit = base * L;

    // Prop-only readability boost.
    // This brightens the model using its own albedo color,
    // so it still looks like the prop texture, just less crushed in darkness.
    lit += base * propAmbientBoost;

    // Prevent props from going nuclear when close to a torch/light.
    lit = min(lit, vec3(maxBrightness));

    finalColor = vec4(lit, alpha);
}

