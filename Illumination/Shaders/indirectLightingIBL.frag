#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.14159265

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_normal;
layout(location = 3) in vec4 v_tangent;
layout(location = 4) in vec3 v_eyePosition;

layout(set = 2, binding = 0) uniform sampler2D u_envmap;
layout(set = 2, binding = 1) uniform sampler2D u_diffuseMap;
layout(set = 2, binding = 2) uniform sampler2D u_normalMap;
layout(set = 2, binding = 3) uniform sampler2D u_pbrMap;
layout(set = 2, binding = 4) uniform sampler2D u_occlusionMap;
layout(set = 2, binding = 5) uniform sampler2D u_emissiveMap;

layout(location = 0) out vec4 outColor;

vec3 FresnelSchlick(vec3 f0, float cosTheta) {
	return f0 + (vec3(1.0) - f0) * pow(1.0 - cosTheta, 5.0);
}

vec3 Fresnel(vec3 f0, float cosTheta, float roughness)
{
	float schlick = pow(1.0 - cosTheta, 5.0);
	return f0 + ((max(vec3(1.0 - roughness), f0) - f0) * schlick);
}

// equi-rectangular ou lat-long ou spherical mapping
vec2 LatLongSample(vec3 dir)
{
	vec2 envmapUV = vec2(atan(dir.z, dir.x), acos(dir.y));
	envmapUV *= vec2(1.0/(2*PI), 1.0/PI);
	return envmapUV;
}

void main() 
{
	// MATERIAU
	const vec3 albedo = texture(u_diffuseMap, v_uv).rgb; //vec3(1.0, 0.0, 1.0);	// albedo = Cdiff, ici magenta
	const vec4 pbr = texture(u_pbrMap, v_uv);
	float AO = texture(u_occlusionMap, v_uv).r;
	vec3 emissiveColor = texture(u_emissiveMap, v_uv).rgb;

	const float metallic = pbr.b;					// surface metallique ou pas ?
	const float perceptual_roughness = pbr.g;

	const float roughness = perceptual_roughness * perceptual_roughness;
	// shininess = brilliance speculaire, glossiness = interdependance brillance et rugosite
	const float glossiness = (2.0 / max(roughness*roughness, 0.0000001)) - 2.0;//512.0;				// brillance speculaire (intensite & rugosite)

	const vec3 f0 = mix(vec3(0.04), albedo, metallic);	// reflectivite a 0 degre, ici 4% si isolant
	
	// rappel : le rasterizer interpole lineairement
	// il faut donc normaliser autrement la norme des vecteurs va changer de magnitude
	vec3 V = normalize(v_eyePosition - v_position);
	vec3 N = normalize(v_normal);
	vec3 T = normalize(v_tangent.xyz);
	vec3 B = cross(N, T) * v_tangent.w;
	mat3 TBN = mat3(T, B, N);
	vec3 normalTS = texture(u_normalMap, v_uv).rgb * 2.0 - 1.0;
	N = normalize(TBN * normalTS);

	// 
	// indirect
	//
	
	// technique simple, on sample un lod en particulier
	// Sachant que maxLod = log²(taille_texture) + 1
	float maxLod = log2(textureSize(u_envmap, 0).x) + 1;
	//int maxLod = textureQueryLevels(u_envmap);

	vec3 indirectDiffuse = vec3(0.0);
	vec2 diffuseUV = LatLongSample(N);
	indirectDiffuse = textureLod(u_envmap, diffuseUV, maxLod-4).rgb;
	indirectDiffuse *= (1.0 - metallic);

	vec3 indirectSpecular = vec3(0.0);
	vec3 R = reflect(-V, N);
	vec2 specularUV = LatLongSample(R);
	float roughLod = maxLod * perceptual_roughness;
	indirectSpecular = textureLod(u_envmap, specularUV, roughLod).rgb;

	// ATTENTION, ici les seuls vecteurs a notre disposition sont 
	// R, V ou N
	//vec3 f0 = mix(vec3(0.04), albedo, metallic);
	vec3 Ks = FresnelSchlick(f0, dot(N, V));

	vec3 indirectColor = (1.0-Ks) * indirectDiffuse 
						+ Ks * indirectSpecular;

	//
	// final
	//

	vec3 finalColor = indirectColor /* + directColor*/;

    outColor = vec4(finalColor, 1.0);
}