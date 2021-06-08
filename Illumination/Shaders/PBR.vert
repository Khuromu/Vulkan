#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject 
{
//    mat4 model[16];
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPositions;
    vec3 lightColors;
} ubo;

layout( push_constant ) uniform MeshData
{
    vec3 albedo;
	float metallic;
	float roughness;
	float ao;
} mData;

struct LightStruct
{
    vec3 lightPos;
    vec3 lightCol;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 camPos;
layout(location = 4) out LightStruct lightOut;


void main() 
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
//    gl_Position = ubo.proj * ubo.view * ubo.model[gl_InstanceID] * vec4(inPosition, 1.0);
    fragUV = inUV;
    outNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
    outPosition = inPosition;
    
    vec3 matToCamPos = -vec3(ubo.view[3]) * mat3(ubo.view);
    camPos = matToCamPos;

    lightOut.lightPos = ubo.lightPositions;
    lightOut.lightCol = ubo.lightColors;
}
