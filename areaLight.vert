#version 330 core

layout(location=0) in vec3 pos; // vector position
layout(location=1) in vec2 txc;

out vec3 fragPos;		// the position of current fragment
out vec2 fragTexCoord;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projectionMat;

void main()
{
	// the position in view space
	fragPos = vec3(modelMat * vec4(pos, 1));
	fragTexCoord = txc;
	gl_Position = projectionMat * viewMat * modelMat * vec4( pos, 1);
}