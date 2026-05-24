#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;
layout(location = 3) in vec4 vertexColor;

// Put instanceTransform somewhere safely after the normal mesh attributes.
// mat4 consumes locations 6, 7, 8, 9.
layout(location = 6) in mat4 instanceTransform;

uniform mat4 mvp;

out vec4 fragColor;

void main()
{
    fragColor = vertexColor;
    gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);
}