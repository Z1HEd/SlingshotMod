#version 330 core

layout(location = 0) in vec4 pos;

out vec4 gsVertPos;

// model view matrix
uniform float MV[25];

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
}