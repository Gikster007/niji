struct Vertex
{
    float3 Position;
};

struct RasterPush
{
    Vertex* vertices;
};

[[vk::push_constant]]
RasterPush pc;

struct VSOutput
{
    float4 Position : SV_Position;
    float3 Color    : COLOR0;
};

[shader("vertex")]
VSOutput vertex_main(uint vertexId : SV_VertexID)
{
    Vertex v = pc.vertices[vertexId];

    VSOutput output;
    // No camera yet — just scale the cube down and pass positions straight through.
    // Cube spans [-1,1]; scale so it fits in clip space.
    output.Position = float4(v.Position * 0.5f, 1.0f);
    // Color by position so each face is visibly different
    output.Color = v.Position * 0.5f + 0.5f;
    return output;
}

[shader("pixel")]
float4 fragment_main(VSOutput input) : SV_Target
{
    return float4(input.Color, 1.0);
}