#version 330 core

in vec4 fsVertPos;
in vec4 fsNormal;
in vec4 fsColor; // r - slingshot material, g - string material, b - unused, a - alpha

out vec4 color;

uniform vec4 lightDir;
uniform int type;

const vec3 woodStringColor = vec3(0.94, 0.992, 1.0);
const vec3 deadlyStringColor = vec3(0.7, 0.7, 0.7);
const vec3 woodColor = vec3(0.76, 0.36, 0.2);

vec3 deadly(in float light)
{
    vec4 pos = normalize(fsVertPos * 0.5);
    
    float pattern = (sin(pos.x * 2.0) + cos(pos.y * 5.0) + cos(pos.z * 15.0) + sin(pos.w * 5.0)) * 0.5 + 0.5;
    
    float h = sin((pos.w * 0.5 + 0.05 + 0.495 * pattern) * 0.5 * 10.0);
    float h2 = cos((pos.w * 0.3 + 0.2 + 0.695 * pattern) * 0.5 * 5.0);
    float h3 = sin((pos.w * 0.7 + 0.7 - 1.2 * pattern) * 0.5 * 15.0);
    
    vec3 result
        = (  mix(vec3(1.0,0.1,1.0),vec3(0.1,1.0,1.0), h)
           + mix(vec3(0.0,0.2,0.2),vec3(2.2,0.0,-0.1), max(h2, 0.0)) * 0.35
           + mix(vec3(0.0,0.0,0.0),vec3(0.0,2.2,2.2), max(h3, 0.0)) * 0.35) * fsColor.r
        + deadlyStringColor * fsColor.g;

    return result * mix(min(light + 0.4, 1.1), light, fsColor.g);
}

vec3 wood(in float light)
{
    vec3 result
        = woodColor * fsColor.r
        + woodStringColor * fsColor.g;
    return result * light;
}

void main()
{
    float light = max(dot(fsNormal, lightDir), 0.0) + 0.1;

    vec3 col = type == 0 ? wood(light) : deadly(light);
    color = vec4(col, fsColor.a);
}
