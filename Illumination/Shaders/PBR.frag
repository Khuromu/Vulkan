#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MEDIUMP_FLT_MAX    65504.0
#define saturateMediump(x) min(x, MEDIUMP_FLT_MAX)

//layout(binding = 1) uniform sampler2D albedo;
//layout(binding = 2) uniform sampler2D normalMap;
//layout(binding = 3) uniform sampler2D metallicMap;
//layout(binding = 4) uniform sampler2D roughnessMap;
//layout(binding = 5) uniform sampler2D aoMap;

struct LightStruct
{
    vec3 lightPos;
    vec3 lightCol;
};

//Constants
const float PI = 3.14159265359;

//Material parameters
layout( push_constant ) uniform MeshData
{
    vec3 albedo;
	float metallic;
	float roughness;
	float ao;
} mData;

//Shader in
layout(location = 0) in vec3 outPosition;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 outNormal;

//Camera
layout(location = 3) in vec3 camPos;

// lights
layout(location = 4) in LightStruct lightOut;

//Shader out
layout(location = 0) out vec4 outColor;

//Custom functions
//PBR
vec3 fresnelSchlick(vec3 _f0, float _u)
{
    return _f0 + (vec3(1.0) - _f0) * pow(1.0 - _u, 5.0);
}

float GeometrySchlickGGX(float _NdotL, float _NdotV, float _alphaG)
{
    // This is the optimize version
    float alphaG2 = _alphaG * _alphaG;

    float Lambda_GGXL = _NdotV * sqrt((-_NdotL * alphaG2 + _NdotL) * _NdotL + alphaG2);
    float Lambda_GGXV = _NdotL * sqrt((-_NdotV * alphaG2 + _NdotV) * _NdotV + alphaG2);

    return 0.5 / (Lambda_GGXV + Lambda_GGXL);
}

//Unstable
float DistributionGGX(float _NdotH , float _roughness)
{
    float r2 = _roughness * _roughness;
    float f = (_NdotH * r2 - _NdotH) * _NdotH + 1;

    return r2 / (f * f) ;
}

//float DistributionGGX(float _NdotH , float _roughness, const vec3 _N, const vec3 _H)
//{
//    vec3 NxH = cross(_N, _H);
//    float a = _NdotH * _roughness;
//    float k = _roughness / (dot(NxH,NxH) + a * a);
//    float d = k * k * (1.0 / PI);
//    return saturateMediump(d);
//}

float GeometrySmith(vec3 _N, vec3 _V, vec3 _L, float _roughness)
{
    vec3 H = normalize(_V + _L);

    float NoV = abs(dot(_N, _V)) + 1e-5;
    float NoL = clamp(dot(_N, _L), 0.0, 1.0);
    float NoH = clamp(dot(_N, H), 0.0, 1.0);
    float LoH = clamp(dot(_L, H), 0.0, 1.0);

    float ggx1  = GeometrySchlickGGX(NoL, NoV, _roughness);
	
    return ggx1;
//    return 0.0;
}

//Main    
void main() 
{
    vec3 N = normalize(outNormal);
    vec3 V = normalize(camPos - outPosition);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, mData.albedo, mData.metallic);

    vec3 Lo = vec3(0.0);

    for(int i = 0; i < 1; ++i) 
    {
        vec3 L = normalize(lightOut.lightPos - outPosition);
        vec3 H = normalize(V + L);

        float dist = length(lightOut.lightPos - outPosition);
        float attenuation = 1.0 / (dist * dist);
        vec3 radiance = lightOut.lightCol * attenuation;

        float NoH = clamp(dot(N, H), 0.0, 1.0);
        float LoH = clamp(dot(L, H), 0.0, 1.0);

        float NDF = DistributionGGX(NoH, mData.roughness);        
        float Vis = GeometrySmith(N, V, L, mData.roughness);      
        vec3 F = fresnelSchlick(F0,LoH);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - mData.metallic;

        vec3 numerator = NDF * Vis * F;
//        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
//        vec3 specular = numerator / max(denominator, 0.001);
        vec3 specular = vec3(numerator);
        
        float NoL = max(dot(N, L), 0.0);                
        Lo += (kD * mData.albedo / PI + specular) * radiance * NoL; 
    }

    vec3 ambient = vec3(0.03) * mData.albedo * mData.ao;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0)); // Tone mapping (reinhardt)
//    color = pow(color, vec3(1.0/2.2)); //Gama correction

    outColor = vec4(color, 1.0);
}