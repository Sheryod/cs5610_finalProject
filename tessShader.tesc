#version 410 core

//layout(vertices = 4) out;
layout(vertices = 3) out;

in vec3 fragPos[];
in vec2 fragTexCoord[];
in vec3 worldPos[];

out vec3 pos[];
out vec2 uvs[]; // is texture coords

uniform int tessLevel;
uniform vec3 cameraVec;
uniform float innerRadius;
uniform float outerRadius;

void main() {
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	
	pos[gl_InvocationID] = fragPos[gl_InvocationID];
	uvs[gl_InvocationID] = fragTexCoord[gl_InvocationID];

	if (gl_InvocationID == 0) {

		vec3 edge0 = (worldPos[0] + worldPos[1]) / 2.0; // triangle side 0
		vec3 edge1 = (worldPos[1] + worldPos[2]) / 2.0; // triangle side 1
		vec3 edge2 = (worldPos[2] + worldPos[0]) / 2.0; // triangle side 2

		float distanceToCam0 = length(edge0 - cameraVec);
		float distanceToCam1 = length(edge1 - cameraVec);
		float distanceToCam2 = length(edge2 - cameraVec);

		float r0 = smoothstep(innerRadius, outerRadius, distanceToCam0);
		float r1 = smoothstep(innerRadius, outerRadius, distanceToCam1);
		float r2 = smoothstep(innerRadius, outerRadius, distanceToCam2);

		float tess0 = mix(4.0 + tessLevel, float(tessLevel), r0);
		float tess1 = mix(4.0 + tessLevel, float(tessLevel), r1);
		float tess2 = mix(4.0 + tessLevel, float(tessLevel), r2);

		gl_TessLevelOuter[0] = round(tess0); 
		gl_TessLevelOuter[1] = round(tess1);
		gl_TessLevelOuter[2] = round(tess2); 
	
		gl_TessLevelInner[0] = round(max(max(tess0, tess1), tess2));
		//gl_TessLevelInner[0] = round((tess0 + tess1 + tess2) / 3);
	}
}