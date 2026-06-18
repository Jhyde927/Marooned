#version 330

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D texture0;

uniform vec3 viewPos;

uniform vec3 ghostColor;
uniform float baseAlpha;
uniform float rimPower;
uniform float rimStrength;

// 0.0 = day, 1.0 = night
uniform float nightDarkness;

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord);

    vec3 viewDir = normalize(viewPos - fragPos);

    float rim = 1.0 - max(dot(normalize(fragNormal), viewDir), 0.0);
    rim = pow(rim, rimPower);

    vec3 color = ghostColor;

    color += rim * rimStrength;

    // Controls how dark it gets at full night.
    // 1.0 = unchanged, 0.35 = 35% brightness at night.
    float nightMultiplier = mix(1.0, 0.35, nightDarkness);

    color *= nightMultiplier;

    float alpha = baseAlpha;

    finalColor = vec4(color, alpha);
}


// #version 330

// in vec3 fragPos;
// in vec3 fragNormal;
// in vec2 fragTexCoord;

// out vec4 finalColor;

// // Base texture (optional)
// uniform sampler2D texture0;

// // Camera position (you must set this!)
// uniform vec3 viewPos;

// // Ghost settings
// uniform vec3 ghostColor;
// uniform float baseAlpha;
// uniform float rimPower;
// uniform float rimStrength;

// void main()
// {
//     // Sample base texture (if raft has one)
//     vec4 texColor = texture(texture0, fragTexCoord);

//     // View direction
//     vec3 viewDir = normalize(viewPos - fragPos);

//     // Fresnel rim
//     float rim = 1.0 - max(dot(normalize(fragNormal), viewDir), 0.0);
//     rim = pow(rim, rimPower);

//     vec3 color = ghostColor;

//     // Add rim glow
//     color += rim * rimStrength;

//     float alpha = baseAlpha;

//     finalColor = vec4(color, alpha);
// }