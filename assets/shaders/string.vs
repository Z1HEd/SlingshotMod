#version 330 core

layout(location = 0) in vec4 pos;
layout(location = 1) in float t;

uniform float model[25];
uniform float view[25];

uniform vec4 targetPos;

vec4 Mat5_multiply(in float m[25], in vec4 v, in float finalComp)
{
	return vec4(
		m[0*5+0] * v[0] + m[1*5+0] * v[1] + m[2*5+0] * v[2] + m[3*5+0] * v[3] + m[4*5+0] * finalComp,
		m[0*5+1] * v[0] + m[1*5+1] * v[1] + m[2*5+1] * v[2] + m[3*5+1] * v[3] + m[4*5+1] * finalComp,
		m[0*5+2] * v[0] + m[1*5+2] * v[1] + m[2*5+2] * v[2] + m[3*5+2] * v[3] + m[4*5+2] * finalComp,
		m[0*5+3] * v[0] + m[1*5+3] * v[1] + m[2*5+3] * v[2] + m[3*5+3] * v[3] + m[4*5+3] * finalComp
	);
}

void Mat5_translate(inout float m[25], in vec4 v)
{
	m[4*5+0] += (m[0*5+0] * v.x) + (m[1*5+0] * v.y) + (m[2*5+0] * v.z) + (m[3*5+0] * v.w);
	m[4*5+1] += (m[0*5+1] * v.x) + (m[1*5+1] * v.y) + (m[2*5+1] * v.z) + (m[3*5+1] * v.w);
	m[4*5+2] += (m[0*5+2] * v.x) + (m[1*5+2] * v.y) + (m[2*5+2] * v.z) + (m[3*5+2] * v.w);
	m[4*5+3] += (m[0*5+3] * v.x) + (m[1*5+3] * v.y) + (m[2*5+3] * v.z) + (m[3*5+3] * v.w);
}

void Mat5_translateB(inout float m[25], in vec4 v)
{
	m[4*5+0] += v.x;
	m[4*5+1] += v.y;
	m[4*5+2] += v.z;
	m[4*5+3] += v.w;
}

void main()
{
	vec4 resultM = Mat5_multiply(model, pos, 1.0);
	if (t > 0.05)
	{
		resultM += targetPos * t;
	}

	vec4 result = Mat5_multiply(view, resultM, 1.0);

	gl_Position = result;
}