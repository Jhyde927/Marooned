uniform sampler2D texture0; // raylib diffuse/albedo slot

uniform vec2 tiles;         // (dungeonWidth, dungeonHeight) OR any repeat count you want

in vec2 fragTexCoord;
out vec4 finalColor;

void main()
{
    vec2 uv = fract(fragTexCoord * tiles);
    vec4 albedo = texture(texture0, uv);

    finalColor = albedo;
}
