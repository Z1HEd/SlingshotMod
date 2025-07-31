#version 330 core

in vec3 fsColor;
in float fsLighting;

out vec4 color;

uniform vec4 inColor;

void main()
{
    color = vec4(inColor.rgb * (fsLighting / 4 + 0.75), inColor.a);
}