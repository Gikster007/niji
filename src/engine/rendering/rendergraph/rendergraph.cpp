#include "rendergraph.hpp"

#include "nodes/compute_node.hpp"

namespace niji
{

ComputeNode& RenderGraph::add_compute_node(std::string_view label, std::string_view shader_path)
{
    ComputeNode* node = new ComputeNode(label, shader_path);
    m_nodes.emplace_back((Node*)node);
    return *node;
}

void RenderGraph::execute()
{
    // vkAcquireNextImage();

    // vkBeginCommandBuffer();

    /* for (node : nodes) 
    {
        for (dep : node.dependencies) 
        {
            create barriers
        }

        deploy all barriers at once
        vkCmdPipelineBarrier();

        get pipeline (create or fetch cached one)
        vkCmdBindPipeline();

        vkCmdDispatch();
    }
    */

    // render target pipeline barrier (?)

    // vkCmdEndCommandBuffer();

    // reset in flight fence 
    // vkResetFences();

    // vkQueueSubmit();

    // vkQueuePresent();
}

} // namespace niji