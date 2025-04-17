#version 330 core
out vec4 color; // Output color

in vec3 texCoord; // Direction vector from vertex shader

uniform samplerCube env; // Environment map (cube map)
uniform int isDirectionalLight; // Flag to toggle environment map

void main() {
    // Sample the environment map using the direction vector
    if (isDirectionalLight == 1)
        color = texture(env, texCoord);
    else
        color = vec4(0.15, 0.15, 0.15, 1.0);
}