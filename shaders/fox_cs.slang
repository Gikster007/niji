[[vk::binding(0, 0)]]
Texture2D<float4> g_sampled_images[];
[[vk::binding(1, 0)]]
RWTexture2D<float4> g_storage_images[];
[[vk::binding(2, 0)]]
SamplerState g_samplers[];

struct Constants
{
    uint32_t tex;
    uint32_t viewport;
    uint32_t rt;
};

[[vk::push_constant]]
Constants pc;

[shader("compute")]
[numthreads(8, 8, 1)]
void compute_main(uint3 thread_id: SV_DispatchThreadID)
{
    float2 uvs = thread_id.xy / float2(1920.0f, 1080.0f);
    // g_storage_images[pc.viewport - 1][thread_id.xy] = float4(uvs, 0.0f, 1.0f);
    g_storage_images[pc.viewport - 1][thread_id.xy] = g_sampled_images[pc.tex - 1][thread_id.xy];
}
