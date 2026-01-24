#version 330

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
    vec4 wp = matModel * vec4(vertexPosition, 1.0);
    vWorldPos = wp.xyz;

    vUV     = vertexTexCoord;
    vColor  = vertexColor;
    vNormal = mat3(matModel) * vertexNormal; // fine for uniform scale

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}


// #version 330

// // Raylib standard attribute names (raylib binds these by name)
// in vec3 vertexPosition;
// in vec2 vertexTexCoord;
// in vec3 vertexNormal;
// in vec4 vertexColor;

// // Must match your fragment shader "in" names exactly
// out vec2 vUV;
// out vec3 vWorldPos;
// out vec4 vColor;
// out vec3 vNormal;

// // Raylib matrices (you MUST set shader.locs for these in C++)
// uniform mat4 mvp;
// uniform mat4 matModel;

// void main()
// {
//     // World position
//     vec4 world = matModel * vec4(vertexPosition, 1.0);
//     vWorldPos = world.xyz;

//     // Pass-through UV + vertex color
//     vUV    = vertexTexCoord;
//     vColor = vertexColor;

//     // World-space normal (good enough for your wallFactor)
//     // NOTE: if you ever use non-uniform scaling, you'll want inverse-transpose.
//     vNormal = mat3(matModel) * vertexNormal;

//     // Clip-space
//     gl_Position = mvp * vec4(vertexPosition, 1.0);
// }
