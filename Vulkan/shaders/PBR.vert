#version 450
#extension GL_ARB_separate_shader_objects : enable

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

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPositions;
    vec3 lightColors;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out FragStruct outFrag;

void main() 
{
    Vertex ver;

    ver.position = inPosition;
    ver.UV = inUV;
    ver.normal = mat3(transpose(inverse(ubo.model))) * inNormal;
    ver.tangent = inTangent;
    ver.bitangent = inBitangent;

    outFrag.outVer = ver;
    
    outFrag.outLight.lightPos = ubo.lightPositions;
    outFrag.outLight.lightCol = ubo.lightColors;

    vec3 matToCamPos = -vec3(ubo.view[3]) * mat3(ubo.view);

    outFrag.camPos = matToCamPos;

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}
