#version 330 core

layout(location=0) out vec4 color;

in vec3 fragPos;
in vec2 fragTexCoord;
in vec3 fragNormal;

const vec3 baseColor = vec3(0.1, 0.2, 0.35); 

// Directional light
uniform vec3 lightDir;          // Light direction = w
uniform vec3 lightColor;        // Light color
uniform float constShininess;   // the shininess of the reflection = alpha
uniform float constLightIntensity;
uniform float constAmbientLight;

uniform vec3 cameraVec;         // Camera vector = V

//uniform sampler2DShadow shadow;
//in vec4 lightView_Position;

uniform samplerCube env;

// the following is for area lights
const int NUM_LIGHTS = 6; // number of area lights
uniform vec3 areaLightVerts[24]; // 4 vertices for 6 squares

uniform sampler2D ltc1;
uniform sampler2D ltc2;
const float LUT_SIZE = 64.0; // ltc_texture size. LUT = look up table
const float LUT_SCALE = (LUT_SIZE - 1.0)/LUT_SIZE;
const float LUT_BIAS = 0.5/LUT_SIZE;

// calculating the edge integral using a cubic function which approximates acos.
// source: https://learnopengl.com/Guest-Articles/2022/Area-Lights
vec3 integrateEdgeVec(vec3 v1, vec3 v2){
    float x = dot(v1, v2);
    float y = abs(x);
    //return vec3(y); // debugging
    float a = 0.8543985 + (0.4965155 + 0.0145206*y)*y;
    float b = 3.4175940 + (4.1616724 + y)*y;
    float v = a / b;
    float theta_sintheta = (x > 0.0) ? v : 0.5*inversesqrt(max(1.0 - x*x, 1e-7)) - v;
    //return vec3(theta_sintheta); // debugging
    return cross(v1, v2) * theta_sintheta;
}

float IntegrateEdge(vec3 v1, vec3 v2)
{
   return integrateEdgeVec(v1, v2).z;
}

/// calculate for 1 area light source
/// modified function from https://learnopengl.com/Advanced-Lighting/Area-Lights
vec3 evaluateLTC(int lightNum, vec3 N, vec3 V, vec3 P, mat3 Minv){ // which light number is this
    
    // construct orthonormal basis around N
    vec3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);

    // rotate area light in (T1, T2, N) basis
    Minv = Minv * transpose(mat3(T1, T2, N));    

    vec3 L[4]; // non transformed light vectors
    vec3 transformedLight[4];
    int index = lightNum * 4;
    for (int i = 0; i < 4; i++){
        L[i] = areaLightVerts[index + i];
        transformedLight[i] = normalize(Minv * (L[i] - P));
        //transformedLight[i] = Minv * (L[i] - P);
    }

    vec3 dir = L[0] - P; // direction from light to surface
    vec3 lightNormal = cross(L[1] - L[0], L[3] - L[0]); // light normal])
    bool behind = (dot(dir, lightNormal) < 0.0); // check if the light is behind the surface))
    if(behind){
        return vec3(0.0); // if the light is behind the surface, return 0
    }


    // integrate the edge of the light
    vec3 vSum = vec3(0.0);
    vSum += integrateEdgeVec(transformedLight[0], transformedLight[1]);
    vSum += integrateEdgeVec(transformedLight[1], transformedLight[2]);
    vSum += integrateEdgeVec(transformedLight[2], transformedLight[3]);
    vSum += integrateEdgeVec(transformedLight[3], transformedLight[0]);

    // return vSum; // just for debugging

    // form factor of the polygon in direction vsum
    float len = length(vSum);
    if (len < 1e-7) {
        return vec3(0.0);
    }

    float z = vSum.z/len;

    vec2 uv = vec2(z * 0.5 + 0.5, clamp(len, 0.0, 1.0)); // range [0, 1]
    uv = uv*LUT_SCALE + LUT_BIAS;

    float scale = texture(ltc2, uv).w;

    float sum = len*scale;

    // Outgoing radiance (solid angle) for the entire polygon
    return vec3(sum);

    //vec3 Lo_i = vec3(sum, sum, sum);
    //return Lo_i;
}


// PBR-maps for roughness (and metallic) are usually stored in non-linear
// color space (sRGB), so we use these functions to convert into linear RGB.
vec3 PowVec3(vec3 v, float p)
{
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

const float gamma = 2.2;
vec3 ToLinear(vec3 v) { return PowVec3(v, gamma); }
vec3 ToSRGB(vec3 v)   { return PowVec3(v, 1.0/gamma); }

void main() {

    // general preparations
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(cameraVec - fragPos);
    vec3 P = fragPos;

    /////////////////
    // Area lights //
    /////////////////

    // area lights preparations
    // gamma correction
    vec3 mDiffuse = vec3(0.7f, 0.8f, 0.96f);
    vec3 mSpecular = ToLinear(vec3(0.23f, 0.23f, 0.23f)); // mDiffuse

    vec3 areaLight_color = vec3(1.0);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);

    // use material roughness and sqrt(1 - cos_theta) to sample M_texture
    vec2 uv_sample = vec2(0.2, sqrt(1.0 - dotNV));
    uv_sample = uv_sample * LUT_SCALE + LUT_BIAS;

    vec4 t1 = texture(ltc1, uv_sample);
    vec4 t2 = texture(ltc2, uv_sample);

    mat3 Minv = mat3(vec3(t1.x, 0, t1.y),
                     vec3(0   , 1,    0),
                     vec3(t1.z, 0, t1.w));

    // area lights
    vec3 ltc_spec = vec3(0.0);
    vec3 ltc_diffuse = vec3(0.0);
    for (int i = 0; i < NUM_LIGHTS; i++){
        // For specular, use the LTC matrix.
        ltc_spec += evaluateLTC(i, N, V, P, Minv) * areaLight_color;
        // For diffuse, use an identity matrix.
        ltc_diffuse += evaluateLTC(i, N, V, P, mat3(1)) * areaLight_color;
    }

    //vec3 ltcResult = 10 * (ltc_diffuse + ltc_spec);


    // GGX BRDF shadowing and Fresnel
    // t2.x: shadowedF90 (F90 normally it should be 1.0)
    // t2.y: Smith function for Geometric Attenuation Term, it is dot(V or L, H).
    ltc_spec *= mSpecular * t2.x + (vec3(1.0) - mSpecular) * t2.y;
    //ltc_diffuse *= mDiffuse * ltc_diffuse;
    ltc_diffuse *= mDiffuse;

    vec3 ltcResult = 100 * (ltc_spec + ltc_diffuse);

    color = vec4(ToSRGB(ltcResult), 1.0);


    //vec3 ltc_test = evaluateLTC(0, N, V, P, Minv);

    //color = vec4(ltc_test, 1.0);

    //color = vec4(ltc_spec, 1.0);
    
    //color = texture(ltc1, uv_sample);
    //color = texture(ltc2, uv_sample);

    //vec3 ltc_test = evaluateLTC(0, N, V, P, Minv);
    //color = vec4(ltc_test, 1.0);
    //vec3 ltc_test = evaluateLTC(0, N, V, P, Minv);
    //color = vec4(ToSRGB(ltc_test), 1.0);


    ////////////////////////
    // directional lights //
    ////////////////////////

    // // diffuse // //
    float dirLightIntensity = constLightIntensity;      // I
    vec3 dirLightCol = lightColor;
    float dirCosTheta = max(dot(N, normalize(-lightDir)), 0);
    vec3 dirDiffuse = dirLightCol * dirCosTheta * baseColor;
    
    // calculating h
    vec3 dirW = normalize(-lightDir);   // omega
    vec3 h = normalize(dirW + V);       // half vector
    // // specular // //
    float dirCosPhi = max(dot(N, h), 0);
    float dirAlpha = constShininess;
    vec3 dirSpecReflCol = vec3(0.8, 0.8, 0.8); // K_s
    vec3 dirSpecular = dirLightCol * dirSpecReflCol * pow(dirCosPhi, dirAlpha);

    // // ambient // //
    vec3 dirAmbientCol = baseColor * 0.5;  // K_a
    float dirAmbientLight = constAmbientLight;

    // result without environment map
    vec3 dirResult = dirLightIntensity * (dirDiffuse + dirSpecular) + dirAmbientLight * dirAmbientCol;

    // // environment map // //
    vec3 reflection = reflect(V, N);
    vec3 envCol = texture(env, reflection).rgb;
    float envRatio = 0.7;
    vec3 envResult = envCol * envRatio;

    // modify result with environment map
    float objectRatio = 1 - envRatio;
    dirResult = dirResult * objectRatio;

    // combine env map and direction light result
    vec3 dirColor = dirResult + envResult;

    //color = vec4(dirColor + ToSRGB(ltcResult), 1.0);



    vec3 combDiffuse = dirDiffuse + ToSRGB(ltc_diffuse);
    vec3 combSpecular = dirSpecular + ToSRGB(ltc_spec);
    vec3 combResult = dirLightIntensity * (combDiffuse + combSpecular) + dirAmbientLight * dirAmbientCol;

    //color = vec4(combResult, 1.0);

}