#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D sceneTexture;
uniform vec2 resolution;

// Bloom + tone mapping
uniform float bloomStrength;
uniform float uExposure;
uniform int uToneMapOperator;

// Vignette / status / fade / dungeon
uniform float baseVignetteStrength;

// 0 = normal red damage
// 1 = frozen blue
// 2 = quad damage orange
// 3 = haste yellow
uniform int vignetteMode;
uniform float vignetteIntensity;
uniform float fadeToBlack;
uniform float dungeonDarkness;
uniform float dungeonContrast;
uniform int isDungeon;

// Cinematic letterbox
uniform float letterboxAmount;   // 0.0 = none, 0.12 = 12% of screen height per bar
uniform float letterboxSoftness; // Try 0.004

// sRGB <-> linear helpers
vec3 toLinear(vec3 c)
{
    return pow(c, vec3(2.2));
}

vec3 toSRGB(vec3 c)
{
    return pow(c, vec3(1.0 / 2.2));
}

// Tone mapping operators, expect linear color
vec3 ToneMapACES(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;

    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 ToneMapReinhard(vec3 x)
{
    return x / (1.0 + x);
}

void main()
{
    vec2 texelSize = 1.0 / resolution;

    // Source scene is assumed to be sRGB.
    vec3 srcSRGB = texture(sceneTexture, fragTexCoord).rgb;
    vec3 srcLin = toLinear(srcSRGB);

    // ------------------------------------------------------------
    // 1) Wide bloom in linear space
    // ------------------------------------------------------------
    vec3 bloomLin = vec3(0.0);

    if (bloomStrength > 0.001)
    {
        const int radius = 6;
        const float spacing = 3.0;
        const float sigma = 3.0;

        float threshold = (isDungeon == 1) ? 0.02 : 0.15;
        float softRange = (isDungeon == 1) ? 0.04 : 0.2;
        float weightSum = 0.0;

        for (int y = -radius; y <= radius; ++y)
        {
            for (int x = -radius; x <= radius; ++x)
            {
                vec2 offset = vec2(float(x), float(y));

                vec2 uv = fragTexCoord + offset * texelSize * spacing;
                uv = clamp(uv, vec2(0.0), vec2(1.0));

                vec3 s = toLinear(texture(sceneTexture, uv).rgb);

                float lum = dot(s, vec3(0.2126, 0.7152, 0.0722));
                float brightMask = smoothstep(
                    threshold, threshold + softRange, lum
                );

                float blurWeight = exp(
                    -dot(offset, offset) / (2.0 * sigma * sigma)
                );

                bloomLin += s * brightMask * blurWeight;
                weightSum += blurWeight;
            }
        }

        bloomLin /= weightSum;
    }

    vec3 resultLin = srcLin + bloomLin * bloomStrength;

    // ------------------------------------------------------------
    // 2) Exposure + tone mapping in linear space
    // ------------------------------------------------------------
    vec3 mappedLin = resultLin * uExposure;

    if (uToneMapOperator == 1)
    {
        mappedLin = ToneMapACES(mappedLin);
    }
    else if (uToneMapOperator == 2)
    {
        mappedLin = ToneMapReinhard(mappedLin);
    }

    // Convert back to sRGB before screen-space UI-style effects.
    vec3 final = toSRGB(mappedLin);

    // ------------------------------------------------------------
    // 3) Vignette / status / fade / dungeon effects
    // ------------------------------------------------------------
    float dist = distance(fragTexCoord, vec2(0.5));
    float vignette = smoothstep(0.4, 0.8, dist);

    // Base dark corner vignette
    final = mix(final, vec3(0.0), vignette * baseVignetteStrength);

    // Damage / status vignette
    vec3 vignetteColor = vec3(1.0, 0.0, 0.0); // Red damage default

    if (vignetteMode == 1)
    {
        vignetteColor = vec3(0.2, 0.6, 1.0); // Frozen
    }
    else if (vignetteMode == 2)
    {
        vignetteColor = vec3(1.0, 0.25, 0.0); // Quad damage
    }
    else if (vignetteMode == 3)
    {
        vignetteColor = vec3(1.0, 0.9, 0.0); // Haste
    }

    final = mix(final, vignetteColor, vignette * vignetteIntensity);

    // Fade to black
    final = mix(final, vec3(0.0), fadeToBlack);

    // Dungeon darkening / contrast / tint
    if (isDungeon == 1)
    {
        final *= 1.0 - dungeonDarkness;

        float midpoint = 0.5;
        final = (final - midpoint) * dungeonContrast + midpoint;

        // Slight blue dungeon tint
        final = mix(final, vec3(0.1, 0.3, 0.7), 0.13);
    }

    // ------------------------------------------------------------
    // 4) Cinematic letterbox
    // ------------------------------------------------------------
    float lb = clamp(letterboxAmount, 0.0, 0.49);
    float soft = max(letterboxSoftness, 0.00001);

    float y = fragTexCoord.y;

    // Bottom bar: 1.0 inside bar, 0.0 outside
    float bottomBar = 1.0 - smoothstep(lb, lb + soft, y);

    // Top bar: 1.0 inside bar, 0.0 outside
    float topBar = smoothstep(1.0 - lb - soft, 1.0 - lb, y);

    float letterboxMask = clamp(bottomBar + topBar, 0.0, 1.0);
    final = mix(final, vec3(0.0), letterboxMask);

    finalColor = vec4(clamp(final, 0.0, 1.0), 1.0);
}