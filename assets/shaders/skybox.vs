#version 330
layout(location=0) in vec3 vertexPosition;
uniform mat4 mvp;
out vec3 vDir; // local cube direction

void main() {
    vDir = vertexPosition;         // cube face direction
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}

