#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 View;
    mat4 Proj;
} ubo;

layout(set = 1, binding = 1) uniform ModelData {
    mat4 Model;
} model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() 
{
    gl_Position = ubo.Proj * ubo.View * model.Model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
}