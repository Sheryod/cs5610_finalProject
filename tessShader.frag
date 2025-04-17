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
uniform int isDirectionalLight;

uniform vec3 cameraVec;         // Camera vector = V

uniform samplerCube env;

// the following is for area lights
uniform vec3 areaLightVerts[24]; // 4 vertices for 6 squares
uniform sampler2D areaLightTex; // area light texture
//uniform vec2 areaLightTexOffset[6]; // offset for area light texture
//uniform vec2 areaLightTexScale; // scale for area light texture
uniform vec2 areaLightTexCorners[4]; // the corner of texture coords
uniform vec3 areaLight4Corners[4]; // the corner vertices

// LTC look up table
uniform sampler2D ltc1;
uniform sampler2D ltc2;
const float LUT_SIZE = 64.0; // ltc_texture size. LUT = look up table
const float LUT_SCALE = (LUT_SIZE - 1.0)/LUT_SIZE;
const float LUT_BIAS = 0.5/LUT_SIZE;

//////////////////////////////
// Structures and functions //
//////////////////////////////

// Structure for holding transformed light data.
struct TransformedLight {
    vec3 center;         // Transformed light center in cosine space.
    vec3 right;          // Transformed "right" vector (texture u-axis).
    vec3 up;             // Transformed "up" vector (texture v-axis).
    float area;          // Area of the transformed light polygon.
    mat3 Minv;           // Inverse LTC matrix after basis alignment.
    vec3 transformedLight[4];  // Transformed light corner vectors.
};

// Approximate edge integral (replacing acos with a rational function)
vec3 integrateEdgeVec(vec3 v1, vec3 v2){
    float x = dot(v1, v2);
    float y = abs(x);
    float a = 0.8543985 + (0.4965155 + 0.0145206*y)*y;
    float b = 3.4175940 + (4.1616724 + y)*y;
    float v = a / b;
    // Use a conditional approximation to avoid singularity at negative dot products.
    float theta_sintheta = (x > 0.0) 
        ? v 
        : 0.5 * inversesqrt(max(1.0 - x*x, 1e-7)) - v;
    return cross(v1, v2) * theta_sintheta;
}

/// Evaluate the transformed light for an area light using the inverse matrix.
/// This implements a key step: transforming light corner directions into the
/// cosine configuration.
TransformedLight evaluateTransLight(vec3 N, vec3 V, vec3 P, mat3 Minv){
    TransformedLight transLight;

    // Create an orthonormal basis around the surface normal N.
    vec3 T1 = normalize(V - N * dot(V, N));
    vec3 T2 = cross(N, T1);

    // Build a new transformation matrix aligning the LTC space with (T1, T2, N).
    mat3 newMinv = Minv * transpose(mat3(T1, T2, N));

    // Transform each of the area light’s corners into cosine space.
    // Using areaLight4Corners (assumed provided in world-space order).
    for (int i = 0; i < 4; i++) {
        // Direction from the surface point to the light corner, then transformed.
        transLight.transformedLight[i] = normalize(newMinv * (areaLight4Corners[i] - P));
    }

    // Compute the center and basis vectors.
    transLight.center = (transLight.transformedLight[0] +
                         transLight.transformedLight[1] +
                         transLight.transformedLight[2] +
                         transLight.transformedLight[3]) / 4.0;
    // For robust basis extraction, assume correct ordering of corners.
    transLight.right = normalize(transLight.transformedLight[1] - transLight.transformedLight[0]);
    transLight.up    = normalize(transLight.transformedLight[3] - transLight.transformedLight[0]);
    transLight.area  = length(transLight.transformedLight[1] - transLight.transformedLight[0]) *
                       length(transLight.transformedLight[3] - transLight.transformedLight[0]);
    transLight.Minv = newMinv;

    return transLight;
}

// Integrate LTC over the light polygon. This approximates the highlight strength.
float integrateLTC(TransformedLight transLight, vec3 P) {
    // Compute light-to-surface vector (not used directly, but available for extensions).
    //vec3 dir = transLight.center - transLight.Minv * P;

    // Compute light polygon normal in cosine space.
    vec3 lightNormal = cross(transLight.transformedLight[1] - transLight.transformedLight[0],
                             transLight.transformedLight[3] - transLight.transformedLight[0]);

    // If the light is behind the surface, return zero.
    if(dot(transLight.transformedLight[0], lightNormal) < 0.0) {
        return 0.0;
    }
    
    // Sum the edge integration contributions.
    vec3 vSum = vec3(0.0);
    vSum += integrateEdgeVec(transLight.transformedLight[0], transLight.transformedLight[1]);
    vSum += integrateEdgeVec(transLight.transformedLight[1], transLight.transformedLight[2]);
    vSum += integrateEdgeVec(transLight.transformedLight[2], transLight.transformedLight[3]);
    vSum += integrateEdgeVec(transLight.transformedLight[3], transLight.transformedLight[0]);

    float len = length(vSum);
    if(len < 1e-7) {
        return 0.0;
    }

    // Use the z component of vSum (which corresponds roughly to the clamped cosine)
    float z = vSum.z / len;

    // Derive UV coordinates for the LTC lookup texture based on (z, len).
    vec2 lookupUV = vec2(z * 0.5 + 0.5, clamp(len, 0.0, 1.0));

    // Map lookup UV into the texture’s coordinate system.
    lookupUV = lookupUV * LUT_SCALE + LUT_BIAS;

    // The scaling factor from the LTC2 texture.
    float scale = texture(ltc2, lookupUV).w;
    return len * scale;
}

////////////////////////////
// Texture mapping helper //
////////////////////////////

// Compute texture coordinates via an orthonormal projection
// which ensures that the average direction is well-defined.
vec2 computeTransUV(TransformedLight transLight) {
    // Transform the fragment position relative to the light center into cosine space.
    vec3 fragP_cosine = transLight.Minv * (fragPos - transLight.center);
    // Compute the light plane normal from the transformed basis vectors.
    vec3 planeNormal = cross(transLight.right, transLight.up);
    // Project the fragment point onto the light plane.
    vec3 projected = fragP_cosine - planeNormal * dot(fragP_cosine, planeNormal);

    // Because the basis vectors might not be unit length, we compute parametric u,v.
    float u = dot(projected, transLight.right) / length(transLight.right);
    float v = dot(projected, transLight.up)    / length(transLight.up);
    // Map from the expected range of [-1,1] to [0,1]
    u = clamp(u * 0.5 + 0.5, 0.0, 1.0);
    v = clamp(v * 0.5 + 0.5, 0.0, 1.0);
    
    // Bilinear interpolation between the precomputed texture corners.
    vec2 bottom = mix(areaLightTexCorners[0], areaLightTexCorners[1], u);
    vec2 top    = mix(areaLightTexCorners[3], areaLightTexCorners[2], u);
    return mix(bottom, top, v);
}

// Compute a simple LOD value based on the distance from the fragment to the light center.
float computeLOD(vec3 P, vec3 lightCenter, float polygonArea) {
    float distanceToLight = length(P - lightCenter);
    float sigma = sqrt((distanceToLight * distanceToLight) / (2.0 * polygonArea));
    return sigma;
}

///////////////////////////////////////
// Gamma correction helper functions //
///////////////////////////////////////

vec3 PowVec3(vec3 v, float p) {
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}
const float gamma = 2.2;
vec3 ToLinear(vec3 v) { return PowVec3(v, gamma); }
vec3 ToSRGB(vec3 v)   { return PowVec3(v, 1.0 / gamma); }

//////////////////////////////////////
// Main fragment shader entry point //
//////////////////////////////////////

void main() {
    // Basic geometric setup.
    vec3 N = normalize(fragNormal);
    if (!gl_FrontFacing){
        N = -N;
    }
    vec3 V = normalize(cameraVec - fragPos);
    vec3 P = fragPos;


    // LTC BRDF Setup
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    vec2 uv_sample = vec2(0.2, sqrt(1.0 - dotNV));


    //float theta_v = acos(dot(N, V));
    //float roughness = 0.2;

    // Map to LUT UV (assuming 64x64 texture)
    // vec2 uv_sample = vec2(theta_v / (0.5 * 3.14), sqrt(roughness));


    uv_sample = uv_sample * LUT_SCALE + LUT_BIAS;
    vec4 t1 = texture(ltc1, uv_sample);
    vec4 t2 = texture(ltc2, uv_sample);
    
    // LTC inverse matrix from lookup data.
    mat3 Minv = mat3(vec3(t1.x, 0.0, t1.y),
                     vec3(0.0  , 1.0, 0.0),
                     vec3(t1.z, 0.0, t1.w));
    

    // Transformed Light (Cosine) 
    TransformedLight tlight = evaluateTransLight(N, V, P, Minv);

    // LTC integration and texture sampling.

    // UV coordinates via the cosine projection.
    vec2 uv = computeTransUV(tlight);
    
    // Level-of-Detail
    float sigma = sqrt((length(P - tlight.center)*length(P - tlight.center)) / (2.0 * tlight.area));
    float lod = log2(sigma * float(textureSize(areaLightTex, 0).x) / tlight.area);

    // area light’s texture color.
    vec3 lightTexColor = ToLinear(textureLod(areaLightTex, uv, lod).rgb);
    
    
    float ltcStrength = integrateLTC(tlight, P);
    
    vec3 ltc_spec = ltcStrength * lightTexColor;
    vec3 ltc_diff = ltcStrength * baseColor * 2;
    

    ////////////////////////
    // directional lights //
    ////////////////////////

    if (isDirectionalLight == 1) {
        // Compute standard directional lighting (diffuse + specular).
        float dirLightIntensity = constLightIntensity;
        vec3 dirLightCol = lightColor;
        float dirCosTheta = max(dot(N, normalize(-lightDir)), 0.0);
        vec3 dirDiffuse = dirLightCol * dirCosTheta * baseColor;
    
        vec3 dirW = normalize(-lightDir);
        vec3 h = normalize(dirW + V);
        float dirCosPhi = max(dot(N, h), 0.0);
        float dirAlpha = constShininess;
        vec3 dirSpecReflCol = vec3(0.8);
        vec3 dirSpecular = dirLightCol * dirSpecReflCol * pow(dirCosPhi, dirAlpha);
    
        vec3 dirAmbientCol = baseColor * 0.5;
        float dirAmbientLight = constAmbientLight;
        vec3 dirResult = dirLightIntensity * (dirDiffuse + dirSpecular)
                        + dirAmbientLight * dirAmbientCol;
    
        // Environment map contribution.
        vec3 reflection = reflect(V, N);
        vec3 envCol = texture(env, reflection).rgb;
        float envRatio = 0.7;
        vec3 envResult = envCol * envRatio;
        dirResult = (1.0 - envRatio) * dirResult + envResult;
    
        // Combine the LTC contribution with the directional light result.
        // The multiplication factors on specular/diffuse here can be tweaked.
        vec3 combDiffuse = dirDiffuse + ToSRGB(ltc_diff) * 3.0;
        vec3 combSpecular = dirSpecular + ToSRGB(ltc_spec) * 10.0;
        vec3 combResult = dirLightIntensity * (combDiffuse + combSpecular)
                          + dirAmbientLight * dirAmbientCol;
    
        color = vec4(combResult, 1.0);
    }
    else {
        vec3 diffuseTot = ToSRGB(ltc_diff) * 3.0;
        vec3 specTot = ToSRGB(ltc_spec) * 10.0;
        vec3 ltc_result = diffuseTot + specTot;

        color = vec4(ltc_result, 1);
    }
}