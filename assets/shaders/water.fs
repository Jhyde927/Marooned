#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;

out vec4 finalColor;

uniform float time;
uniform vec3 cameraPos;


void main()
{
    // Simple sine wave distortion to simulate ripples
    float wave = sin(fragTexCoord.x * 30.0 + time * 0.5) * 0.15 +
                 cos(fragTexCoord.y * 30.0 + time * 0.3) * 0.15;

    float brightness = 1.3f + wave;

    // Depth-based gradient using distance from camera
    float distance = length(fragPosition - cameraPos);
    float depthFactor = clamp((distance - 500.0) / 6000.0, 0.0, 1.0); // adjust range

    vec3 shallowColor = vec3(0.25, 0.5, 0.75);
    vec3 deepColor    = vec3(0.1, 0.25, 0.65);

    vec3 waterColor = mix(shallowColor, deepColor, depthFactor) * brightness;

    finalColor = vec4(waterColor, 0.8); // translucent water
}


