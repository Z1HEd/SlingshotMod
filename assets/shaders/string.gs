#version 330 core

layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 4) out;

// projection matrix is handled here after the coordinates are converted to 3D
uniform mat4 P;

// tetrahedron edge vertex indices
const ivec2 edges[6] = ivec2[6](ivec2(0, 1), ivec2(0, 2), ivec2(0, 3), ivec2(1, 2), ivec2(1, 3), ivec2(2, 3));

void main()
{
	// calculate intersection between simplex and hyperplane (3D view)

	// number of intersection points
	int k = 0;
	for (int i = 0; i < 6; ++i)
	{
		if (k == 4)
		{
			break;
		}

		ivec2 e = edges[i];
		vec4 p0 = gl_in[e.x].gl_Position;
		vec4 p1 = gl_in[e.y].gl_Position;

		// vertices are on the same side of the hyperplane so the edge doesn't intersect
		if ((p0[3] < 0.0000001 && p1[3] < 0.0000001) || (p0[3] > -0.0000001 && p1[3] > -0.0000001))
		{
			continue;
		}

		// intersection
		float a = 0;
		if (abs(p1[3] - p0[3]) > 0.0000001)
		{
			a = (-p0[3]) / (p1[3] - p0[3]);
		}
		gl_Position = P * vec4(mix(p0.xyz, p1.xyz, a), 1.0);

		EmitVertex();

		++k;
	}
	EndPrimitive();
}