// terrain.vert
#version 330
in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;

uniform mat4 mvp;
uniform mat4 matModel;

out vec3 fragPosition;

void main() {
    vec4 wp = matModel * vec4(vertexPosition, 1.0);
    fragPosition = wp.xyz;                 // WORLD SPACE!
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}

// #version 330

// in vec3 vertexPosition;
// in vec3 vertexNormal;
// // in vec2 vertexTexCoord; // not needed for your current FS

// uniform mat4 mvp;
// uniform mat4 matModel;

// out vec3 fragPosition;
// out vec3 fragNormal;

// void main()
// {
//     vec4 worldPos = matModel * vec4(vertexPosition, 1.0);
//     fragPosition = worldPos.xyz;

//     // Approximate normal transform (good enough if you’re not shearing)
//     fragNormal = normalize(mat3(matModel) * vertexNormal);

//     gl_Position = mvp * vec4(vertexPosition, 1.0);
// }


// #version 330
// layout(location = 0) in vec3 vertexPosition;
// layout(location = 2) in vec3 vertexNormal;

// uniform mat4 mvp;
// uniform mat4 matModel;

// out vec3 fragNormal;
// out vec3 fragPosition;

// void main() {
//     vec4 worldPos   = matModel * vec4(vertexPosition, 1.0);
//     fragPosition    = worldPos.xyz;
//     fragNormal      = mat3(transpose(inverse(matModel))) * vertexNormal;

//     // Raylib mvp already includes matModel
//     gl_Position     = mvp * vec4(vertexPosition, 1.0);
// }



