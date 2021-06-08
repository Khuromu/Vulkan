#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MEDIUMP_FLT_MAX    65504.0
#define saturateMediump(x) min(x, MEDIUMP_FLT_MAX)

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPositions;
    vec3 lightColors;
} ubo;

layout(binding = 1) uniform sampler2D albedoSampler;
layout(binding = 2) uniform sampler2D normalSampler;
layout(binding = 3) uniform sampler2D metallicSampler;
layout(binding = 4) uniform sampler2D roughnessSampler;
layout(binding = 5) uniform sampler2D aoSampler;

struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 UV;
    vec3 tangent;
    vec3 bitangent;
};

struct LightStruct
{
    vec3 lightPos;
    vec3 lightCol;
};

struct FragStruct
{
    Vertex outVer;
    vec3 camPos;
    LightStruct outLight;
};

//Constants
const float PI = 3.14159265359;

layout(location = 0) in FragStruct outFrag;

//Shader out
layout(location = 0) out vec4 outColor;

//Custom functions
//PBR
vec3 fresnelSchlick(vec3 f0, float u)
{
    return f0 + (vec3(1.0) - f0) * pow(1.0 - u, 5.0);
}

float GeometrySchlickGGX(float nDotL, float nDotV, float alphaG)
{
    // This is the optimize version
    float alphaG2 = alphaG * alphaG;

    float lambda_GGXL = nDotV * sqrt((-nDotL * alphaG2 + nDotL) * nDotL + alphaG2);
    float lambda_GGXV = nDotL * sqrt((-nDotV * alphaG2 + nDotV) * nDotV + alphaG2);

    return 0.5 / (lambda_GGXV + lambda_GGXL);
}

//Unstable
float DistributionGGX(float nDotH , float roughness)
{
    float r2 = roughness * roughness;
    float f = (nDotH * r2 - nDotH) * nDotH + 1;

    return r2 / (f * f) ;
}

float GeometrySmith(vec3 n, vec3 v, vec3 l, float roughness)
{
    vec3 h = normalize(v + l);

    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);

    float ggx1  = GeometrySchlickGGX(NoL, NoV, roughness);
	
    return ggx1;
}

vec3 GetNormalMap()
{
    vec3 normal = texture(normalSampler, outFrag.outVer.UV).rgb;
    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);
    return normal;
}

//Main    
void main() 
{
    Vertex outVer = outFrag.outVer;

    vec3 textToNormal = texture(normalSampler, outVer.UV).rgb;
    // transform normal vector to range [-1,1]
    textToNormal = normalize(textToNormal * 2.0 - 1.0);

    vec3 albedo = pow(texture(albedoSampler, outVer.UV).rgb, vec3(2.2f));
    vec3 normal = GetNormalMap();
    float metallic = texture(metallicSampler, outVer.UV).r;
    float roughness = texture(roughnessSampler, outVer.UV).r;
    float ao = texture(aoSampler, outVer.UV).r;

    vec3 camPos = outFrag.camPos;
    LightStruct outLight = outFrag.outLight;

    vec3 N = normalize(normal);
    vec3 V = normalize(camPos - outVer.position);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    for(int i = 0; i < 1; ++i) 
    {
        vec3 L = normalize(outLight.lightPos - outVer.position);
        vec3 H = normalize(V + L);

        float dist = length(outLight.lightPos - outVer.position);
        float attenuation = 1.0 / (dist * dist);
        vec3 radiance = outLight.lightCol * attenuation;

        float NoH = clamp(dot(N, H), 0.0, 1.0);
        float LoH = clamp(dot(L, H), 0.0, 1.0);

        float NDF = DistributionGGX(NoH, roughness);
        float Vis = GeometrySmith(N, V, L, roughness);      
        vec3 F = fresnelSchlick(F0,LoH);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 numerator = NDF * Vis * F;
        vec3 specular = vec3(numerator);
        
        float NoL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NoL; 
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0)); // Tone mapping (reinhardt)

    outColor = vec4(color, 1.0);

    //outColor = vec4(texture(albedoSampler, outVer.UV).rgb, 1.0); //Colored
    //outColor = vec4(outVer.UV, 0.0, 1.0); //Colored
}