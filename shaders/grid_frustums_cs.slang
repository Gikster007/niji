// Inspired by: https://www.3dgep.com/forward-plus/#Compute_Grid_Frustums

#define BLOCK_SIZE 16

struct Plane
{
    float3 Normal;
    float Distance; // Distance to origin
};

struct Frustum
{
    // Top, Bottom, Left, Right 
    // (Front and Back will be calculated in the Light Culling stage)
    Plane Planes[4]; 
};

Plane compute_plane(float3 point0, float3 point1, float3 point2)
{
    Plane plane;

    float3 vector0 = point1 - point0;
    float3 vector2 = point2 - point0;

    plane.Normal = normalize(cross(vector0, vector2));

    plane.Distance = dot(plane.Normal, point0);

    return plane;
}

// Set = 1, Binding = 0
[[vk::binding(0, 1)]]
cbuffer DispatchParams
{
    float4x4 InverseProjection;
    // Number of groups dispatched
    uint3 numThreadGroups;
    uint _pad0;
    // Total number of threads dispatched
    uint3 numThreads;
    uint _pad1;
    float2 ScreenDimensions;
    float2 _pad2;
}

// Set = 1, Binding = 1
[[vk::binding(1, 1)]]
RWStructuredBuffer<Frustum> outFrustums;

struct ComputeShaderInput
{
    uint3 GroupID : SV_GroupID; // 3D index of the thread group in the dispatch.

    uint3 GroupThreadID : SV_GroupThreadID; // 3D index of local thread ID in a thread group.

    uint3 DispatchThreadID : SV_DispatchThreadID; // 3D index of global thread ID in the dispatch.

    uint GroupIndex : SV_GroupIndex; // Flattened local index of the thread within a thread group.
};

float4 clip_to_view(float4 clip)
{
    // View space position.
    float4 view = mul(InverseProjection, clip);

    // Perspective projection.
    view = view / view.w;

    return view;
}

float4 screen_to_view(float4 screen)
{
    // Convert to normalized texture coordinates
    float2 texCoord = screen.xy / ScreenDimensions;

    // Convert to clip space
    float4 clip = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2.0f - 1.0f, screen.z, screen.w);
    return clip_to_view(clip);
}

[shader("compute")]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void compute_main(ComputeShaderInput input)
{
    // View space eye position is always at the origin.
    const float3 eyePos = float3(0.0f, 0.0f, 0.0f);

    // Compute the 4 corner points on the far clipping plane to use as the
    // frustum vertices.
    float4 screenSpace[4];

    // Top left point
    screenSpace[0] = float4(input.DispatchThreadID.xy * BLOCK_SIZE, -1.0f, 1.0f);

    // Top right point
    screenSpace[1] = float4(float2(input.DispatchThreadID.x + 1, input.DispatchThreadID.y) * BLOCK_SIZE, -1.0f, 1.0f);

    // Bottom left point
    screenSpace[2] = float4(float2(input.DispatchThreadID.x, input.DispatchThreadID.y + 1) * BLOCK_SIZE, -1.0f, 1.0f);

    // Bottom right point
    screenSpace[3] = float4(float2(input.DispatchThreadID.x + 1, input.DispatchThreadID.y + 1) * BLOCK_SIZE, -1.0f, 1.0f);

    float3 viewSpace[4];
    for (int i = 0; i < 4; i++)
    {
        viewSpace[i] = screen_to_view(screenSpace[i]).xyz;
    }

    // Now build the frustum planes from the view space points
    Frustum frustum;

    // Left plane
    frustum.Planes[0] = compute_plane(eyePos, viewSpace[2], viewSpace[0]);

    // Right plane
    frustum.Planes[1] = compute_plane(eyePos, viewSpace[1], viewSpace[3]);

    // Top plane
    frustum.Planes[2] = compute_plane(eyePos, viewSpace[0], viewSpace[1]);

    // Bottom plane
    frustum.Planes[3] = compute_plane(eyePos, viewSpace[3], viewSpace[2]);

    // Store the computed frustum in global memory (if our thread ID is in bounds of the grid).
    if (input.DispatchThreadID.x < numThreads.x && input.DispatchThreadID.y < numThreads.y)
    {
        uint index = input.DispatchThreadID.x + (input.DispatchThreadID.y * numThreads.x);

        outFrustums[index] = frustum;
    }
}