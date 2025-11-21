#version 330

// Inputs from raylib
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

// Outputs to fragment shader
out vec2 vUV;
out vec3 vWorldPos;
out vec4 vColor;

// Raylib-provided matrices
uniform mat4 mvp;          // model * view * projection
uniform mat4 matModel;     // model transform only

void main()
{
    // Pass UV & vertex color straight through
    vUV    = vertexTexCoord;
    vColor = vertexColor;

    // Compute world-space position of this vertex
    vec4 world = matModel * vec4(vertexPosition, 1.0);
    vWorldPos = world.xyz;

    // Standard transform to clip space
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
