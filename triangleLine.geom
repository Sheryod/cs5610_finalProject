#version 330 core

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

uniform vec4 lineOffset;

void main()
{
	for (int i = 0; i < 3; i++) {

		gl_Position = gl_in[i].gl_Position + lineOffset;
		EmitVertex();
		gl_Position = gl_in[(i + 1) % 3].gl_Position + lineOffset;
		EmitVertex();
		
		EndPrimitive();
	};
}