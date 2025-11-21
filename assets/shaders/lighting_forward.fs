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

void main()
{
    // Base albedo
    vec4 baseS = texture(texture0, vUV);
    vec3 base  = baseS.rgb * colDiffuse.rgb;
    float alpha = baseS.a * colDiffuse.a;

    // Start with ambient
    vec3 lighting = vec3(uAmbient);

    // We only care about XZ plane for distance
    vec2 fragXZ = vWorldPos.xz;

    for (int i = 0; i < uLightCount && i < MAX_LIGHTS; ++i)
    {
        vec3 lp     = uLightPosRadius[i].xyz;
        float radius = uLightPosRadius[i].w;

        vec3 color   = uLightColorIntensity[i].rgb;
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

        lighting += color * (intensity * falloff);
    }

    // Clamp a bit so it doesn't blow out
    lighting = clamp(lighting, 0.0, 2.0);

    finalColor = vec4(base * lighting, alpha);
}
