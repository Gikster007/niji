struct MaterialInfo
{
    float4 AlbedoFactor;
    float4 EmissiveFactor;
    float MetallicFactor;
    float RoughnessFactor;
    int HasNormalMap;
    int HasEmissiveMap;
    int HasMetallicMap;
    int HasRoughnessMap;

    int _padding[4];
}

// Set = 0, Binding = 0
[[vk::binding(0, 0)]]
cbuffer Camera
{
    float4x4 View;
    float4x4 Proj;
    float3 Pos;
    float _pad0;
}

// Set = 1, Binding = 3
[[vk::binding(0, 1)]]
cbuffer ModelData
{
    float4x4 Model;
    float4x4 InvModel;

    MaterialInfo MatInfo;
}

struct VertexInput
{
    float3 Position : ATTRIB0;
    float3 Color : ATTRIB1;
    float3 Normal : ATTRIB2;
    float4 Tangent : ATTRIB3;
    float2 TexCoord : ATTRIB4;
};

struct VertexOutput
{
    float4 Position : SV_POSITION;
    float4 FragPosition : POSITION0;
    float3 Color : COLOR0;
    float3 Normal : NORMAL0;
    float3 Tangent : TANGENT0;
    float3 BiTangent : BITANGENT0;
    float2 TexCoord : TEXCOORD0;
};

[shader("vertex")]
VertexOutput vertex_main(VertexInput input)
{
    VertexOutput output;
    float4 worldPosition = mul(Model, float4(input.Position, 1.0f));
    output.Position = mul(Proj, mul(View, worldPosition));
    output.FragPosition = worldPosition;
    output.Color = input.Color;
    output.Normal = mul((float3x3)InvModel, input.Normal);
    output.Tangent = mul((float3x3)InvModel, input.Tangent.xyz);
    output.BiTangent = cross(output.Normal, output.Tangent.xyz) * input.Tangent.w;
    output.TexCoord = input.TexCoord;
    return output;
}

[shader("fragment")]
void fragment_main()
{

}