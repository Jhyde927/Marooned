#version 330

out vec4 finalColor;

uniform vec4 outlineColor;

void main()
{
    finalColor = outlineColor;
}