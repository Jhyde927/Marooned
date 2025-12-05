#version 330

// Raylib standard attribute names — let it bind them by location.
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

out vec2 vUV;
out vec3 vWorldPos;
out vec4 vColor;
out vec3 vNormal;

uniform mat4 mvp;
uniform mat4 matModel;

void main()
{
    // World position of the vertex
    vec4 world = matModel * vec4(vertexPosition, 1.0);
    vWorldPos = world.xyz;

    // Pass through UV and vertex color
    vUV    = vertexTexCoord;
    vColor = vertexColor;

    // Transform normal into world space (or model space if you prefer)
    vNormal = mat3(matModel) * vertexNormal;

    // Final clip-space position
    gl_Position = mvp * vec4(vertexPosition, 1.0);
    // Or:
    // gl_Position = mvp * world;
}


