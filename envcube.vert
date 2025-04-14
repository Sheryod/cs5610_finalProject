#version 330 core
layout(location = 0) in vec3 pos; // Vertex position of cube

out vec3 texCoord; // Direction vector for environment map sampling

uniform mat4 viewMat;   // View matrix (without translation)
uniform mat4 projMat;   // Projection matrix

void main() {
    // Pass the vertex position as the direction vector
    texCoord = pos; // Use the vertex position as the direction

    vec4 oriPos = projMat * viewMat * vec4(pos, 1.0);

    // set the z value to w
    gl_Position = oriPos.xyww;

}