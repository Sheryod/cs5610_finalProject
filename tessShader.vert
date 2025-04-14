#version 330 core

layout(location=0) in vec3 pos; // vector position
layout(location=1) in vec2 txc;

out vec3 fragPos;		// the position of current fragment
out vec2 fragTexCoord;

// for control shader: calculate distance to camera
out vec3 worldPos;
uniform mat4 modelMat;

void main()
{
	// sends the data to tesselation control shader
	fragPos = pos;
	fragTexCoord = txc;
	gl_Position = vec4( pos, 1);

	// set up worldPos
	worldPos = vec3(modelMat * vec4(pos, 1));
}