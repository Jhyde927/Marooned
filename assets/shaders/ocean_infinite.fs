#version 330

in vec3 vWorldPos;
in vec2 vUV;

out vec4 finalColor;
in float vHit;
in float vT;
uniform float u_Time;
uniform vec3  u_CamPos;

uniform vec3  u_WaterColor;   // base water tint
uniform vec3  u_SkyColor;     // match your sky / fog
uniform float u_FogDensity;   // e.g. 0.00012 (depends on your world scale)

uniform float u_HorizonFadeStart; // e.g. 0.85 (fraction of u_MaxDist)
uniform float u_HorizonFadePower; // e.g. 1.0 to 2.0
uniform float u_MaxDist;          // must match VS value (or pass it here too)

// Wave tuning
uniform float u_WaveAmp;      // e.g. 0.08 (affects fake normal slope)
uniform float u_WaveFreq;     // e.g. 6.0
uniform float u_WaveSpeed;    // e.g. 0.6

vec3 FakeNormal(vec2 uv, float t)
{
    float a = sin(uv.x * u_WaveFreq + t * u_WaveSpeed);
    float b = cos(uv.y * (u_WaveFreq * 0.9) + t * (u_WaveSpeed * 0.8));
    float c = sin((uv.x + uv.y) * (u_WaveFreq * 0.6) - t * (u_WaveSpeed * 0.5));

    // approximate slope in x/z from a few waves
    vec2 grad = vec2(a + 0.5*c, b + 0.5*c) * u_WaveAmp;
    return normalize(vec3(-grad.x, 1.0, -grad.y));
}

void main()
{
    if (vHit < 0.01) discard;
    float dist = length(vWorldPos - u_CamPos);

    vec3 N = FakeNormal(vUV, u_Time);
    vec3 V = normalize(u_CamPos - vWorldPos);

    // Fresnel reflect sky at grazing angles
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 5.0);

    // Subtle brightness ripple
    float ripple = 0.05 * sin(vUV.x * 18.0 + u_Time * 0.7)
                 + 0.05 * cos(vUV.y * 16.0 + u_Time * 0.5);

    vec3 water = u_WaterColor * (1.0 + ripple);

    // Exponential distance fog to sky
    float fog = 1.0 - exp(-dist * u_FogDensity);
    fog = clamp(fog, 0.0, 1.0);

    //Combine: water -> sky reflection -> sky haze
    vec3 rgb = mix(water, u_SkyColor, fresnel * 0.65);
    //rgb = mix(rgb, u_SkyColor, fog);

    // --- ✦ NEW: Horizon clamp fade ---
    // Start fading when vT approaches max distance
    float startDist = u_MaxDist * u_HorizonFadeStart;  // e.g. 0.85 * maxDist
    float horizon   = smoothstep(startDist, u_MaxDist, vT);

    // Optional: shape it (higher power = tighter fade at the very end)
    horizon = pow(horizon, u_HorizonFadePower);

    // Blend into sky as we approach max distance
    rgb = mix(rgb, u_SkyColor, horizon);

    finalColor = vec4(rgb, 1.0); // opaque

    //finalColor = vec4(1.0, 0.0, 0.0, 1.0);
}