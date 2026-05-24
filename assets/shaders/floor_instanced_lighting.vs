#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;
layout(location = 3) in vec4 vertexColor;

// Match the debug shader.
// mat4 consumes 6, 7, 8, 9.
layout(location = 6) in mat4 instanceTransform;

uniform mat4 mvp;

out vec2 vUV;
out vec3 vWorldPos;
out vec4 vColor;
out vec3 vNormal;

void main()
{
    vec4 world = instanceTransform * vec4(vertexPosition, 1.0);

    vWorldPos = world.xyz;
    vUV       = vertexTexCoord;
    vColor    = vertexColor;
    vNormal   = normalize(mat3(instanceTransform) * vertexNormal);

    gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);
}

// #version 330

// // Raylib standard mesh attributes
// in vec3 vertexPosition;
// in vec2 vertexTexCoord;
// in vec3 vertexNormal;
// in vec4 vertexColor;

// // Raylib DrawMeshInstanced feeds this per instance.
// // In C++, store its attrib loc in SHADER_LOC_MATRIX_MODEL.
// in mat4 instanceTransform;

// out vec2 vUV;
// out vec3 vWorldPos;
// out vec4 vColor;
// out vec3 vNormal;

// uniform mat4 mvp;

// void main()
// {
//     vec4 world = instanceTransform * vec4(vertexPosition, 1.0);

//     vWorldPos = world.xyz;
//     vUV       = vertexTexCoord;
//     vColor    = vertexColor;
//     vNormal   = normalize(mat3(instanceTransform) * vertexNormal);

//     gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);
// }