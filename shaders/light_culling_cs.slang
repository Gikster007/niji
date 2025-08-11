// Inspired by: https://www.3dgep.com/forward-plus/#frustum-culling

#define MAX_LIGHTS_PER_TILE 1024
#define BLOCK_SIZE 16

// TODO: Move duplicate structs to a slang module that can be imported in multiple .slang files
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

struct Sphere
{
    float3 Center;
    float _pad0;
    float Radius;
};

struct DirectionalLight
{
    float3 Direction;
    float _pad0;
    float3 Color;
    float _pad1;

    float Intensity; // 36 bytes
}

struct ComputeShaderInput
{
    uint3 GroupID : SV_GroupID; // 3D index of the thread group in the dispatch.

    uint3 GroupThreadID : SV_GroupThreadID; // 3D index of local thread ID in a thread group.

    uint3 DispatchThreadID : SV_DispatchThreadID; // 3D index of global thread ID in the dispatch.

    uint GroupIndex : SV_GroupIndex; // Flattened local index of the thread within a thread group.
};

bool sphere_inside_plane(Sphere sphere, Plane plane)
{

    return dot(plane.Normal, sphere.Center) - plane.Distance < -sphere.Radius;
}

bool sphere_inside_frustum(Sphere sphere, Frustum frustum, float zNear, float zFar)
{
    bool result = true;

    // First check depth
    if (sphere.Center.z - sphere.Radius > zNear || sphere.Center.z + sphere.Radius < zFar)
    {
        result = false;
    }

    // Then check frustum planes
    for (int i = 0; i < 4 && result; i++)
    {
        if (sphere_inside_plane(sphere, frustum.Planes[i]))
        {

            result = false;
        }
    }

    return result;
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
RWStructuredBuffer<Frustum> inFrustums;

// Set = 1, Binding = 2
[[vk::binding(2, 1)]]
RWStructuredBuffer<uint> o_LightIndexCounter;

// Set = 1, Binding = 3
[[vk::binding(3, 1)]]
RWStructuredBuffer<uint> o_LightIndexList;

// Set = 1, Binding = 4
[[vk::binding(4, 1)]]
RWTexture2D<uint2> o_LightGrid;

// Set = 1, Binding = 3
[[vk::binding(5, 1)]]
Texture2D DepthTextureVS;

// Set = 1, Binding = 6
[[vk::binding(6, 1)]]
StructuredBuffer<Sphere> Lights;

// Set = 1, Binding = 7
[[vk::binding(7, 1)]]
cbuffer SceneInfo
{
    DirectionalLight dirLight;
    int PointLightCount;
    int2 _pad2; // 64 bytes
}

groupshared uint uMinDepth;
groupshared uint uMaxDepth;
groupshared Frustum GroupFrustum;
groupshared uint o_LightCount;
groupshared uint o_LightIndexStartOffset;
groupshared uint o_LightList[MAX_LIGHTS_PER_TILE];

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

void o_AppendLight(uint lightIndex)
{
    uint index; // Index into the visible lights array.

    InterlockedAdd(o_LightCount, 1, index);

    if (index < MAX_LIGHTS_PER_TILE)
    {
        o_LightList[index] = lightIndex;
    }
}

[shader("compute")]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void compute_main(ComputeShaderInput input)
{
    //o_LightGrid[input.GroupID.xy] = uint2(0, 0);
    //o_LightIndexList[input.GroupIndex] = 0;
    //GroupMemoryBarrierWithGroupSync();
    // Calculate min & max depth in threadgroup / tile.
    int2 texCoord = input.DispatchThreadID.xy;
    float fDepth = DepthTextureVS.Load(int3(texCoord, 0)).r;
    uint uDepth = asuint(fDepth);

    if (input.GroupIndex == 0) // Avoid contention by other threads in the group.
    {
        uMinDepth = 0xffffffff;
        uMaxDepth = 0;
        o_LightCount = 0;
        GroupFrustum = inFrustums[input.GroupID.x + (input.GroupID.y * numThreadGroups.x)];
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedMin(uMinDepth, uDepth);
    InterlockedMax(uMaxDepth, uDepth);

    GroupMemoryBarrierWithGroupSync();

    float fMinDepth = asfloat(uMinDepth);
    float fMaxDepth = asfloat(uMaxDepth);

    // Convert depth values to view space.
    float minDepthVS = clip_to_view(float4(0, 0, fMinDepth, 1)).z;
    float maxDepthVS = clip_to_view(float4(0, 0, fMaxDepth, 1)).z;
    float nearClipVS = clip_to_view(float4(0, 0, 0, 1)).z;

    // Clipping plane for minimum depth value
    // (used for testing lights within the bounds of opaque geometry).
    Plane minPlane = { float3(0, 0, -1), -minDepthVS };

    // Cull lights
    // Each thread in a group will cull 1 light until all lights have been culled.
    for (uint i = input.GroupIndex; i < PointLightCount; i += BLOCK_SIZE * BLOCK_SIZE)
    {
        Sphere sphere = Lights[i];

        if (sphere_inside_frustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
        {
            if (!sphere_inside_plane(sphere, minPlane))
            {
                // Add light to light list for opaque geometry.
                o_AppendLight(i);
            }
        }
    }

    // Wait till all threads in group have caught up.
    // GroupMemoryBarrierWithGroupSync();
    DeviceMemoryBarrierWithGroupSync();

    // Update global memory with visible light buffer.
    // First update the light grid (only thread 0 in group needs to do this)
    if (input.GroupIndex == 0)
    {
        // Update light grid for opaque geometry.
        InterlockedAdd(o_LightIndexCounter[0], o_LightCount, o_LightIndexStartOffset);

        o_LightGrid[input.GroupID.xy] = uint2(o_LightIndexStartOffset, o_LightCount);
    }

    GroupMemoryBarrierWithGroupSync();

    // Now update the light index list (all threads)
    for (uint i = input.GroupIndex; i < o_LightCount; i += BLOCK_SIZE * BLOCK_SIZE)
    {
        o_LightIndexList[o_LightIndexStartOffset + i] = o_LightList[i];
    }
}
