#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D sceneTexture;
uniform vec2 resolution;

// NEW: base vignette (normal dark corners)
uniform float baseVignetteStrength; // 0.0 = off, ~0.3–0.8 typical

// Existing:
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
    final = mix(final, vec3(1.0, 0.0, 0.0), vignette * vignetteIntensity);

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


// #version 330

// in vec2 fragTexCoord;
// out vec4 finalColor;

// uniform sampler2D sceneTexture;
// uniform vec2 resolution;
// uniform float vignetteIntensity; // 0.0 to 1.0
// uniform float fadeToBlack;       // 0.0 = no fade, 1.0 = full black
// uniform float dungeonDarkness;   // 0.0 = normal, 1.0 = fully dark
// uniform float dungeonContrast;   // 1.0 = normal, >1.0 = more contrast
// uniform int isDungeon;

// void main()
// {
//     // We keep texel around in case you want it later, but it's unused now
//     vec2 texel = 1.0 / resolution;

//     vec3 final = texture(sceneTexture, fragTexCoord).rgb;

//     // Red Vignette Overlay
//     float dist = distance(fragTexCoord, vec2(0.5));
//     float vignette = smoothstep(0.4, 0.8, dist);
//     final = mix(final, vec3(1.0, 0.0, 0.0), vignette * vignetteIntensity);

//     // Fade to black
//     final = mix(final, vec3(0.0), fadeToBlack);

//     // Dungeon darkening / contrast / tint
//     if (isDungeon == 1) {
//         final *= 1.0 - dungeonDarkness;

//         float midpoint = 0.5;
//         final = (final - midpoint) * dungeonContrast + midpoint;

//         // slight blue tint
//         final = mix(final, vec3(0.1, 0.3, 0.7), 0.13);
//     }

//     finalColor = vec4(final, 1.0);
// }



// #version 330

// in vec2 fragTexCoord;
// out vec4 finalColor;

// uniform sampler2D sceneTexture;
// uniform vec2 resolution;
// uniform float vignetteIntensity; // 0.0 to 1.0
// uniform float fadeToBlack; // 0.0 = no fade, 1.0 = full black
// uniform float dungeonDarkness;  // 0.0 = normal, 1.0 = fully dark
// uniform float dungeonContrast;  // 1.0 = normal, >1.0 = more contrast


// uniform int isDungeon;

// void main()
// {
//     vec2 texel = 1.0 / resolution;
//     vec3 center = texture(sceneTexture, fragTexCoord).rgb;

//     float ao = 0.0;
//     ao += length(center - texture(sceneTexture, fragTexCoord + texel * vec2(-1, -1)).rgb);
//     ao += length(center - texture(sceneTexture, fragTexCoord + texel * vec2( 1, -1)).rgb);
//     ao += length(center - texture(sceneTexture, fragTexCoord + texel * vec2(-1,  1)).rgb);
//     ao += length(center - texture(sceneTexture, fragTexCoord + texel * vec2( 1,  1)).rgb);
//     ao = clamp(ao * 0.5, 0.0, 1.0);

//     vec3 final = center - ao * 0.1; //0.2

//     // Red Vignette Overlay
//     float dist = distance(fragTexCoord, vec2(0.5));
//     float vignette = smoothstep(0.4, 0.8, dist);
//     final = mix(final, vec3(1.0, 0.0, 0.0), vignette * vignetteIntensity);

//     // Fade to black
//     final = mix(final, vec3(0.0), fadeToBlack);

//     if (isDungeon == 1) {
//         final *= 1.0 - dungeonDarkness;
//         float midpoint = 0.5;
//         final = (final - midpoint) * dungeonContrast + midpoint;
//         final = mix(final, vec3(0.1, 0.3, 0.7), 0.13); // slight blue tint
//     }
    
//     finalColor = vec4(final, 1.0);


// }

