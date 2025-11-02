
#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D sceneTexture;
uniform vec2 resolution;
uniform float vignetteIntensity; // 0.0 to 1.0
uniform float fadeToBlack; // 0.0 = no fade, 1.0 = full black
uniform float dungeonDarkness;  // 0.0 = normal, 1.0 = fully dark
uniform float dungeonContrast;  // 1.0 = normal, >1.0 = more contrast
uniform float colorBleedAmount; // 0.0 = none, 0.2 = default

uniform int isDungeon;

void main()
{
    vec2 texel = 1.0 / resolution;
    vec3 center = texture(sceneTexture, fragTexCoord).rgb;

    float ao = 0.0;
    ao += length(center - texture(sceneTexture, fragTexCoord + texel * vec2(-1, -1)).rgb);
    ao += length(center - texture(sceneTexture, fragTexCoord + texel * vec2( 1, -1)).rgb);
    ao += length(center - texture(sceneTexture, fragTexCoord + texel * vec2(-1,  1)).rgb);
    ao += length(center - texture(sceneTexture, fragTexCoord + texel * vec2( 1,  1)).rgb);
    ao = clamp(ao * 0.5, 0.0, 1.0);

    vec3 final = center - ao * 0.1; //0.2

    // Red Vignette Overlay
    float dist = distance(fragTexCoord, vec2(0.5));
    float vignette = smoothstep(0.4, 0.8, dist);
    final = mix(final, vec3(1.0, 0.0, 0.0), vignette * vignetteIntensity);

    // Fade to black
    final = mix(final, vec3(0.0), fadeToBlack);

    if (isDungeon == 1) {
        final *= 1.0 - dungeonDarkness;
        float midpoint = 0.5;
        final = (final - midpoint) * dungeonContrast + midpoint;
        final = mix(final, vec3(0.1, 0.3, 0.7), 0.13); // slight blue tint
    }
    //colorBleedAmount = 0.0;
    // === Color Bleed (uses original `center`) ===
    if (colorBleedAmount > 0.0) {
        vec3 blend = center;
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                vec2 offset = texel * vec2(x, y);
                vec3 sample = texture(sceneTexture, fragTexCoord + offset).rgb;
                float weight = dot(sample, vec3(0.299, 0.587, 0.114)); // brightness
                blend += sample * weight;
            }
        }
        final = mix(center, blend / 10.0, colorBleedAmount); // leave `center` for best result
    }
    

    finalColor = vec4(final, 1.0);
    //finalColor = vec4(1.0, 0.0, 0.0, 1.0);

}

