#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;

out vec4 finalColor;

uniform float time;
uniform vec3 cameraPos;

// Fade system
uniform vec2  u_WaterCenterXZ;
uniform float u_PatchHalfSize;
uniform float u_FadeStart;
uniform float u_FadeEnd;

uniform vec3  u_waterColor;
uniform int   u_isSwamp;

// Sky / time-of-day color
uniform vec3  u_SkyColorTop;
uniform vec3  u_SkyColorHorizon;
uniform float u_SkyReflectionStrength; // try 0.35 day, 0.65 night
uniform float u_WaterNightDarkness;    // 0.0 = day, 1.0 = night

void main()
{
    // --- Ripple / wave brightness ---
    float wave = sin(fragTexCoord.x * 30.0 + time * 0.5) * 0.15 +
                 cos(fragTexCoord.y * 30.0 + time * 0.3) * 0.15;

    float brightness = 1.3;//+ wave;

    // --- Depth-based color gradient ---
    float distanceToCam = length(fragPosition - cameraPos);
    float depthFactor = clamp((distanceToCam - 500.0) / 6000.0, 0.0, 1.0);

    vec3 shallowColor = vec3(0.32, 0.48, 0.65);
    vec3 deepColor    = vec3(0.15, 0.38, 0.60);
    vec3 swampColor   = vec3(0.32, 0.45, 0.30);

    vec3 waterColor;


    waterColor = mix(shallowColor, deepColor, depthFactor) * brightness;
    

    // --- Current sky color ---
    // You can replace 60.0 with a u_SeaLevel uniform later if you want.
    float skyT = clamp((cameraPos.y - 60.0) / 600.0, 0.0, 1.0);
    vec3 skyColor = mix(u_SkyColorHorizon, u_SkyColorTop, skyT);

    // --- Make water follow sky color ---
    // Farther water reflects/absorbs more sky color.
    float skyWaterAmount = smoothstep(100.0, 8000.0, distanceToCam);
    skyWaterAmount *= u_SkyReflectionStrength;

    waterColor = mix(waterColor, skyColor, skyWaterAmount);

    // --- General night darkening ---
    // Keeps nearby water from glowing too much at night.
    float nightT = clamp(u_WaterNightDarkness, 0.0, 1.0);
    float nightBrightness = mix(1.0, 0.45, nightT);
    waterColor *= nightBrightness;

    // --- Edge Fade: radial based on patch center ---
    float radialDist = length(fragPosition.xz - u_WaterCenterXZ);
    float edgeFade = smoothstep(u_PatchHalfSize, u_PatchHalfSize, radialDist); // <<-------- mess with this. 0.82

    // --- Horizon Fade: distance from camera ---
    float horizonFade = smoothstep(u_FadeStart, u_FadeEnd, distanceToCam);

    // Combine fades
    float fade = clamp(edgeFade + horizonFade - edgeFade * horizonFade, 0.0, 1.0);

    // Since final alpha is forced opaque, fade color toward sky instead of black.
    // This usually blends better with horizon/sky.
    //vec3 finalRGB = mix(waterColor, skyColor, fade * 0.65);

    vec3 finalRGB = waterColor;//mix(waterColor, skyColor, fade);


    finalColor = vec4(finalRGB, 1.0);
}
