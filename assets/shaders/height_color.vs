#version 330
layout(location = 0) in vec3 vertexPosition;
layout(location = 2) in vec3 vertexNormal;

uniform mat4 mvp;
uniform mat4 matModel;

out vec3 fragNormal;
out vec3 fragPosition;

void main() {
    vec4 worldPos   = matModel * vec4(vertexPosition, 1.0);
    fragPosition    = worldPos.xyz;
    fragNormal      = mat3(transpose(inverse(matModel))) * vertexNormal;

    // Raylib mvp already includes matModel
    gl_Position     = mvp * vec4(vertexPosition, 1.0);
}



