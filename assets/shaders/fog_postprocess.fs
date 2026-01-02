#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D sceneTexture;
uniform vec2 resolution;

// NEW: base vignette (normal dark corners)
uniform float baseVignetteStrength; // 0.0 = off, ~0.3–0.8 typical

// 0 = normal (red damage)
// 1 = frozen (blue ice)
uniform int vignetteMode;
uniform float vignetteIntensity; // red damage vignette 0.0–1.0
uniform float fadeToBlack;       // 0.0 = no fade, 1.0 = full black
uniform float dungeonDarkness;   // 0.0 = normal, 1.0 = fully dark
uniform float dungeonContrast;   // 1.0 = normal, >1.0 = more contrast
uniform int isDungeon;

void main()
{
    vec2 texel = 1.0 / resolution;
    vec3 final = texture(sceneTexture, fragTexCoord).rgb;

    // Compute vignette factor once
    float dist = distance(fragTexCoord, vec2(0.5));
    float vignette = smoothstep(0.4, 0.8, dist);

    // 1) Base dark vignette (normal edges)
    final = mix(final, vec3(0.0), vignette * baseVignetteStrength);

    // 2) Red vignette overlay (damage)
    //final = mix(final, vec3(1.0, 0.0, 0.0), vignette * vignetteIntensity);
    
    // 2) Damage / status vignette
    vec3 vignetteColor = vec3(1.0, 0.0, 0.0); // red default

    if (vignetteMode == 1) {
        vignetteColor = vec3(0.2, 0.6, 1.0); // icy blue
    }

    final = mix(final, vignetteColor, vignette * vignetteIntensity);

    // 3) Fade to black
    final = mix(final, vec3(0.0), fadeToBlack);

    // 4) Dungeon darkening / contrast / tint
    if (isDungeon == 1) {
        final *= 1.0 - dungeonDarkness;

        float midpoint = 0.5;
        final = (final - midpoint) * dungeonContrast + midpoint;

        // slight blue tint
        final = mix(final, vec3(0.1, 0.3, 0.7), 0.13);
    }

    finalColor = vec4(final, 1.0);
}



