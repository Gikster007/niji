// Set = 0, Binding = 0
[[vk::binding(0, 0)]]
cbuffer Camera
{
    float4x4 View;
    float4x4 Proj;
    float3 Pos;
    float _pad0;
}
// Set = 1, Binding = 0
[[vk::binding(0, 1)]]
TextureCube<float4> cubemap;

// Set = 1, Binding = 1
[[vk::binding(1, 1)]]
SamplerState linearSampler;

struct SkyboxInput
{
    float3 Position : ATTRIB0;
};

struct VertexOutput
{
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD0;
};

[shader("vertex")]
VertexOutput vertex_main(SkyboxInput input)
{
    VertexOutput output;
    float3x3 view3 = float3x3(View);
    float4x4 view4 = float4x4(float4(view3[0], 0.0), float4(view3[1], 0.0), float4(view3[2], 0.0),
                              float4(0.0, 0.0, 0.0, 1.0));

    float4x4 pvm = mul(Proj, view4);
    output.Position = mul(pvm, float4(input.Position, 1.0f));
    output.TexCoord = input.Position;
    return output;
}

[shader("fragment")]
float4 fragment_main(VertexOutput input) : SV_Target
{
    float3 environment = pow(cubemap.SampleLevel(linearSampler, normalize(input.TexCoord), 0.0f).rgb, 1.0f / 2.2f);

    return float4(environment, 1.0f);
}
