#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D albedoSampler;
layout(binding = 2) uniform sampler2D normalSampler;
layout(binding = 3) uniform sampler2D metallicSampler;
layout(binding = 4) uniform sampler2D roughnessSampler;
layout(binding = 5) uniform sampler2D aoSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor; 

void main() 
{
	//vec4 color = vec4(1.0, 0.5, 0.25, 1.0);
	/*si color est sRGB (provient d'une texture ou color-picker)
	il faut appliquer une conversion sRGB->linear*/
	//color.rgb = pow(color.rgb, vec3(2.2));

	/*si la swapchain utilise un format _SRGB
	le GPU va automatiquement convertir linear->SRGB (gamma)
	cad pow(outColor, 1/2.2)*/
	//outColor = color;
    //outColor = vec4(fragColor, 1.0);

	//outColor = texture(texSampler, fragTexCoord); //Basic
	//outColor = texture(texSampler, fragTexCoord * 2.0); //Repetition

	outColor = vec4(fragColor * texture(albedoSampler, fragTexCoord).rgb, 1.0); //Colored
}