#version 330 core

layout(location=0) out vec4 color;

in vec2 fragTexCoord;
uniform sampler2D areaLightTex;
uniform int useTexture;

void main() {
    // Sample the area light texture
    if(useTexture == 0) {
        color = vec4(0.8, 0.8, 0.8, 1);
    }
    else {
        color = texture(areaLightTex, fragTexCoord);
    }
}