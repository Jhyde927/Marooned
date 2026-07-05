#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;

uniform mat4 mvp;
uniform float outlineWidth;

void main()
{
    vec3 normalDir = normalize(vertexNormal);
    vec3 expandedPosition = vertexPosition + normalDir * outlineWidth;

    gl_Position = mvp * vec4(expandedPosition, 1.0);
}