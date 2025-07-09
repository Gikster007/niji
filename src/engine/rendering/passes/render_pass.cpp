#include "render_pass.hpp"


using namespace niji;

#include "../../engine.hpp"

void RenderPass::base_cleanup()
{
    m_pipeline.cleanup();
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_passBuffer[i].cleanup();
    }
    vkDestroyDescriptorSetLayout(nijiEngine.m_context.m_device, m_passDescriptorSetLayout, nullptr);
}