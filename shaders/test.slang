struct Vertex
{
    float3 Position;
};

struct CameraData
{
    float4x4 View;
    float4x4 Proj;
    float3 Pos;
};

struct RasterPush
{
    Vertex* vertices;
    CameraData* cameraData;
};

[[vk::push_constant]]
RasterPush pc;

struct VSOutput
{
    float4 Position : SV_Position;
    float3 Color : COLOR0;
};

[shader("vertex")]
VSOutput vertex_main(uint vertexId: SV_VertexID)
{
    Vertex v = pc.vertices[vertexId];
    CameraData cameraData = *pc.cameraData;

    VSOutput output;
    output.Position = mul(cameraData.Proj, mul(cameraData.View, float4(v.Position, 1.0f)));
    output.Color = v.Position * 0.5f + 0.5f;
    return output;
}

[shader("pixel")]
float4 fragment_main(VSOutput input) : SV_Target
{
    return float4(input.Color, 1.0);
}
