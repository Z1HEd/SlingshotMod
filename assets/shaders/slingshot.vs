#version 330 core

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec4 color;

out vec4 gsVertPos;
out vec4 gsNormal;
out vec4 gsColor;

// model view matrix
uniform float MV[25];

mat4 MV_mat4()
{
	return mat4(
		vec4(MV[0*5+0], MV[0*5+1], MV[0*5+2], MV[0*5+3]),
		vec4(MV[1*5+0], MV[1*5+1], MV[1*5+2], MV[1*5+3]),
		vec4(MV[2*5+0], MV[2*5+1], MV[2*5+2], MV[2*5+3]),
		vec4(MV[3*5+0], MV[3*5+1], MV[3*5+2], MV[3*5+3])
	);
}

void main()
{
	// multiply the vertex by MV
	float v[5] = float[5](pos.x, pos.y, pos.z, pos.w, 1.0);
	vec4 result;
	for (int row = 0; row < 4; ++row)
	{
		result[row] = 0;
		for (int col = 0; col < 5; ++col)
		{
			result[row] += MV[col * 5 + row] * v[col];
		}
	}

	gl_Position = result;
	gsVertPos = pos;
    gsNormal = normalize(transpose(inverse(MV_mat4())) * normal);
    gsColor = color / 255.0;
}