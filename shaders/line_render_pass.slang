struct VertexInput
{
    float3 Position : ATTRIB0;
    float3 Color : ATTRIB1;
};

struct VertexOutput
{
    float4 Position : SV_POSITION;
    float3 Color : COLOR0;
};

// Set = 0, Binding = 0
[[vk::binding(0, 0)]]
cbuffer Camera
{
    float4x4 View;
    float4x4 Proj;
    float3 Pos;
}

[shader("vertex")]
VertexOutput vertex_main(VertexInput input)
{
    VertexOutput output;

    output.Position = mul(mul(Proj, View), float4(input.Position, 1.0f));
    output.Color = input.Color;

    return output;
}

[shader("fragment")]
float4 fragment_main(VertexOutput input) : SV_Target
{
    return float4(input.Color, 1.0f);
}