#version 450
#extension GL_EXT_samplerless_texture_functions : require

layout(set = 1, binding = 0) uniform RenderFlags
{
    int Albedo;
    int Normals;
} flags;

layout(set = 1, binding = 2) uniform sampler linearSampler;

layout(set = 1, binding = 3) uniform texture2D baseColor;
layout(set = 1, binding = 4) uniform texture2D normalMap;
layout(set = 1, binding = 5) uniform texture2D occlusionMap;
layout(set = 1, binding = 6) uniform texture2D roughMetalMap;
layout(set = 1, binding = 7) uniform texture2D emissiveMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    if (flags.Albedo == 1)
    {
        outColor = texture(sampler2D(baseColor, linearSampler), fragTexCoord);
    }
    else if (flags.Normals == 1)
        outColor = texture(sampler2D(normalMap, linearSampler), fragTexCoord);
}