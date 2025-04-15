#version 330 core

layout(location=0) out vec4 color;

in vec2 fragTexCoord;
uniform sampler2D areaLightTex;

void main() {
    // Sample the area light texture
    // color = vec4(0.8, 0.8, 0.8, 1);

    color = texture(areaLightTex, fragTexCoord);
}