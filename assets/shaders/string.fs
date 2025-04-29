#version 330 core

in vec4 fsVertPos;

out vec4 color;

uniform vec4 inColor;

void main()
{
    color = inColor;
}
