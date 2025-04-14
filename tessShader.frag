#version 330 core

layout(location=0) out vec4 color;

in vec3 fragPos;
in vec2 fragTexCoord;
in vec3 fragNormal;

uniform vec3 lightDir;          // Light direction = w
uniform vec3 lightColor;        // Light color
uniform vec3 cameraVec;         // Camera vector = V
uniform float constShininess;   // the shininess of the reflection = alpha
uniform float constLightIntensity;
uniform float constAmbientLight;

//uniform sampler2DShadow shadow;
//in vec4 lightView_Position;

uniform samplerCube env;

// the following is for area lights
const int NUM_LIGHTS = 6; // number of area lights
const float PI = 3.14159265358;
uniform mat3 Minv;
uniform vec3 areaLightVerts[24]; // 4 vertices for 6 squares

float computeLTCSpecular(int lightNum){ // which light number is this
    vec3 transformedLight[4];
    int index = lightNum * 4;
    for (int i = 0; i < 4; i++){
        transformedLight[i] = normalize(Minv * areaLightVerts[index + i]);
    }

    float ltc_value = 0;

    for (int i = 0; i < 4; i++){
        int j = (i + 1) % 4;
        
        float cosTheta = dot(transformedLight[i], transformedLight[j]);
        float angle = acos(clamp(cosTheta, -1, 1));
        vec3 crossProd = normalize(cross(transformedLight[i], transformedLight[j]));
        float direction = dot(crossProd, vec3(0, 0, 1));

        ltc_value += angle * direction;
    }

    return ltc_value / (2 * PI);
}

// calculating the edge integral using a cubic function which approximates acos.
// source: https://learnopengl.com/Guest-Articles/2022/Area-Lights
float integrateEdge(vec3 v1, vec3 v2, vec3 N){
    float x = dot(v1, v2);
    float y = abs(x);
    float a = 0.8543985 + (0.4965155 + 0.0145206 * y) * y;
    float b = 3.4175940 + (4.1616724 + y) * y;
    float v = a / b;
    float theta_sintheta = (x > 0.0) ? v : 0.5 * inversesqrt(max(1.0 - x * x, 1e-7)) - v;
    return dot(cross(v1, v2)* theta_sintheta, N);
}

//float vectorIrradiance(vec3 v1, vec3 v2, vec3 N){
//    vec3 F = integradeEdge(v1, v2, N) * normalize(cross(v1, v2));
//    float angularExtent = asin(sqrt(length(F)));
//    float elevationAngle = dot(N, normalize(F));
//    return elevationAngle;
//}

void main() {
    
    vec3 normal = normalize(fragNormal);

    float lightIntensity = constLightIntensity; // I
    vec3 lightCol = lightColor;

    // diffuse
    vec3 baseColor = vec3(0.1, 0.2, 0.35);   // K_d

    float cosTheta = max(dot(normal, normalize(-lightDir)), 0);
    vec3 diffuse = lightCol * cosTheta * baseColor;

    // for calculating h
    vec3 w = normalize(-lightDir); // omega
    vec3 v = normalize(cameraVec - fragPos);
    vec3 h = normalize(w + v);

    // this is for the environment map
    vec3 cam2Surface = normalize(cameraVec - fragPos);
    //vec3 cam2Surface = normalize(fragPos - cameraVec);
    vec3 reflection = reflect(cam2Surface, normal);
    vec3 envCol = texture(env, reflection).rgb;

    // fresnel
    float fresnel = pow(1 - max(dot(normal, cam2Surface), 0), 5);

    // specular
    float cosPhi = max(dot(normal, h), 0);
    float alpha = constShininess;

    vec3 specReflCol = vec3(0.8, 0.8, 0.8); // K_s
    //vec3 specular = lightCol * specReflCol * pow(cosPhi, alpha) * fresnel;
            // vec3 specular = lightCol * specReflCol * pow(cosPhi, alpha);


    // trial specular
    //vec3 reflectDir = reflect(lightDir, normal); // Reflect light direction
    //float spec = pow(max(dot(cam2Surface, reflectDir), 0.0), constShininess);
    //spec = smoothstep(0.1, 0.15, spec); // Tight threshold for "moon glint"
    //vec3 specular = lightCol * specReflCol * spec;


    // ambient
    vec3 ambientCol = baseColor * 0.5;  // K_a

    float ambientLight = constAmbientLight;
    
    // // only the diffuse and specular is multiplied by the shadow. 
    // vec3 result = lightIntensity * (diffuse + specular) * textureProj(shadow, lightView_Position) + ambientLight * ambientCol;
    
    // result no shadow
            // vec3 result = lightIntensity * (diffuse + specular) + ambientLight * ambientCol;

    // // only normals
    // vec3 result = normal;
    // vec3 result = vec3(fragNormal.x, 0.0, fragNormal.z);

    // float envRatio = 0.7;
    // vec3 envResult = envCol * 0.7;
    // vec3 envResult = envCol * fresnel;

    // float objectRatio = 1 - envRatio;
    // result = result * objectRatio;
    // result = result * (1.0 - fresnel);

            // vec3 finalColor = mix(result, envCol, fresnel);
    // vec3 finalColor = mix(envCol, result, fresnel);

    // color = vec4(result + envResult, 1.0);
            // color = vec4(finalColor, 1.0);
    // color = vec4(baseColor, 1.0); 



    // area lights
    float ltc_spec = 0;
    for (int i = 0; i < NUM_LIGHTS; i++){
        ltc_spec += computeLTCSpecular(i);
    }

    // vec3 specular = lightCol * specReflCol * pow(cosPhi, alpha) * ltc_spec;
    vec3 specular = lightCol * specReflCol * ltc_spec;
    vec3 result = lightIntensity * (diffuse + specular) + ambientLight * ambientCol;

    vec3 finalColor = mix(result, envCol, fresnel);
    color = vec4(finalColor, 1.0);


}