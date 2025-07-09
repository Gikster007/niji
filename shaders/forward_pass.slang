// Set = 0, Binding = 0
[[vk::binding(0, 0)]]
cbuffer UniformBufferObject
{
    float4x4 View;
    float4x4 Proj;
}

// Set = 1, Binding = 1
[[vk::binding(1, 1)]]
cbuffer ModelData
{
    float4x4 Model;
}

// Set = 1, Binding = 0
[[vk::binding(0, 1)]]
cbuffer RenderFlags
{
    int Albedo;
    int Normals;
}

// Set = 1, Binding = 2
[[vk::binding(2, 1)]]
SamplerState linearSampler;

// Set = 1, Binding = 3..7
[[vk::binding(3, 1)]]
Texture2D baseColor;
[[vk::binding(4, 1)]]
Texture2D normalMap;
[[vk::binding(5, 1)]]
Texture2D occlusionMap;
[[vk::binding(6, 1)]]
Texture2D roughMetalMap;
[[vk::binding(7, 1)]]
Texture2D emissiveMap;

struct VertexInput
{
    float3 inPosition : ATTRIB0;
    float3 inColor : ATTRIB1;
    float3 inNormal : ATTRIB2;
    float2 inTexCoord : ATTRIB3;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR0;
    float3 normal : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
};

[shader("vertex")]
VertexOutput vertex_main(VertexInput input)
{
    VertexOutput output;
    float4 worldPosition = mul(Model, float4(input.inPosition, 1.0f));
    output.position = mul(Proj, mul(View, worldPosition));
    output.color = input.inColor;
    output.normal = input.inNormal;
    output.texCoord = input.inTexCoord;
    return output;
}

[shader("fragment")]
float4 fragment_main(VertexOutput input) : SV_Target
{
    if (Albedo == 1)
        return baseColor.Sample(linearSampler, input.texCoord);
    else if (Normals == 1)
        return normalMap.Sample(linearSampler, input.texCoord);

    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
