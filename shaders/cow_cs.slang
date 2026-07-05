[[vk::binding(0, 0)]] Texture2D<float4>   g_sampled_images[];
[[vk::binding(1, 0)]] RWTexture2D<float4> g_storage_images[];
[[vk::binding(2, 0)]] SamplerState        g_samplers[];

[[vk::push_constant]]
uint32_t index;

[shader("compute")]
[numthreads(8, 8, 1)]
void compute_main(uint3 thread_id : SV_DispatchThreadID)
{
    g_storage_images[index][thread_id.xy] = float4(1.0f);
}
