#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;

out vec4 finalColor;

uniform float time;
uniform vec3 cameraPos;

// ✦ New uniforms for fade system ✦
uniform vec2  u_WaterCenterXZ;   // world XZ of the patch center (camera-following)
uniform float u_PatchHalfSize;   // half size of patch in world units (e.g., 3000)
uniform float u_FadeStart;       // start of horizon fade (e.g., 2500)
uniform float u_FadeEnd;         // end of horizon fade (e.g., 4500)

uniform vec3  u_waterColor;

void main()
{
    // --- Ripple / wave brightness (your original) ---
    float wave = sin(fragTexCoord.x * 30.0 + time * 0.5) * 0.15 +
                 cos(fragTexCoord.y * 30.0 + time * 0.3) * 0.15;
    float brightness = 1.3 + wave;

    // --- Depth-based color gradient (your original) ---
    float distanceToCam = length(fragPosition - cameraPos);
    float depthFactor = clamp((distanceToCam - 500.0) / 6000.0, 0.0, 1.0);

    vec3 shallowColor = vec3(0.25, 0.6, 0.77);
    vec3 deepColor    = vec3(0.1, 0.35, 0.68);

    vec3 mixedColor = mix(shallowColor, deepColor, depthFactor) * brightness;

    vec3 waterColor = u_waterColor;//mix(shallowColor, deepColor, depthFactor) * brightness;

    // --- ✦ Edge Fade: radial based on patch center ---
    float radialDist = length(fragPosition.xz - u_WaterCenterXZ);
    float edgeFade = smoothstep(u_PatchHalfSize * 0.82, u_PatchHalfSize, radialDist);

    // --- ✦ Horizon Fade: distance from camera ---
    float horizonFade = smoothstep(u_FadeStart, u_FadeEnd, distanceToCam);

    // Combine fades (both reduce visibility)
    float fade = clamp(edgeFade + horizonFade - edgeFade * horizonFade, 0.0, 1.0);

    // Apply fade to alpha and a touch to brightness
    float alpha = mix(0.8, 0.0, fade);
    vec3 finalRGB = mix(waterColor, vec3(0.0), fade * 0.2);

    finalColor = vec4(finalRGB, 1.0); //fully opaque to hide culled under water chunks
}
