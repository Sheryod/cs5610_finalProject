#version 410 core

//layout(quads, equal_spacing, ccw) in;
layout(triangles, equal_spacing, ccw) in;

in vec3 pos[];
in vec2 uvs[];
//in vec3 wPos[];

out vec3 fragPos;
out vec2 fragTexCoord;
out vec3 fragNormal;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projectionMat;

uniform float time;

uniform int numOfWaves;
uniform float waveAmplitude[32];
uniform float waveFrequency[32];
uniform float waveSpeed[32];
uniform vec2 waveDirection[32]; // the number must be constant or it breaks

void main() {

    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
    float w = 1.0 - u - v;
    fragTexCoord = u * uvs[0] + v * uvs[1] + w * uvs[2];

    vec3 currentPos = u * pos[0] + v * pos[1] + w * pos[2];

    // calculate normal and height
    vec3 tangent = vec3(1.0, 0.0, 0.0);
    vec3 binormal = vec3(0.0, 1.0, 0.0);
    float height = 0.0;

    float tempPrevDerivative = 0;
    float derivative;
    for (int i = 0; i < numOfWaves; i++) {
        float phase = time * waveSpeed[i];
        vec2 currPos = currentPos.xz;
        currPos.x += tempPrevDerivative;
        float wave = dot(waveDirection[i], currPos) * waveFrequency[i];
        derivative = exp(sin(wave + phase) - 1) * cos(wave + phase) * waveFrequency[i] * waveAmplitude[i];

        height += waveAmplitude[i] * exp(sin(wave + phase) - 1);
        tangent.z += waveDirection[i].x * derivative; // dY/dX
        binormal.z += waveDirection[i].y * derivative; // dY/dZ
        tempPrevDerivative = derivative;
    }

    fragNormal = normalize(cross(tangent, binormal));

    currentPos.y = height;
    // the position in view space
	fragPos = vec3(modelMat * vec4(currentPos, 1));

	gl_Position = projectionMat * viewMat * modelMat * vec4( currentPos, 1);

}