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



