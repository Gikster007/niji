#include "render_pass.hpp"

using namespace niji;

#include "../../engine.hpp"

void RenderPass::base_cleanup()
{
    m_pipeline.cleanup();

    for (int i = 0; i < m_passBuffer.size(); i++)
    {
        m_passBuffer[i].cleanup();
    }

    m_passDescriptor.cleanup();
}