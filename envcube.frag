#version 330 core
out vec4 color; // Output color

in vec3 texCoord; // Direction vector from vertex shader

uniform samplerCube env; // Environment map (cube map)
uniform bool isEnvironment; // Flag to toggle environment map

void main() {
    // Sample the environment map using the direction vector
    //if (isEnvironment)
    color = texture(env, texCoord);
    //else
        //color = vec4(0.9, 0.9, 0.9, 1.0);
}