//#include "render_pass.hpp"
//
//using namespace niji;
//
//#include <iostream>
//
//#include "engine.hpp"
//
//#include "rendering/renderer.hpp"
//#include "rendering/resource_bank.hpp"
//
//void RenderPass::update(Renderer& renderer, CommandList& cmd)
//{
//    if (m_vertFrag.Type != ShaderType::NONE && m_shaderWatcher.hasChanged(m_vertFrag.Source))
//    {
//        if (!m_vertFrag.re_compile())
//        {
//            return;
//            //throw std::runtime_error("Failed To Recompile Shader");
//        }
//        vkDeviceWaitIdle(renderer.m_context->m_device);
//
//        for (auto& [name, pipeline] : m_pipelines)
//        {
//            if (!pipeline.IsGraphicsPipeline)
//                continue;
//
//            GraphicsPipelineDesc desc = pipeline.GraphicsDesc;
//            pipeline.cleanup();
//
//            pipeline = Pipeline(desc);
//
//            std::cout << "[HotReload] Reloaded Pipeline:" << name << " From " << m_name << "\n";
//        }
//    }
//
//    if (m_compute.Type != ShaderType::NONE && m_shaderWatcher.hasChanged(m_compute.Source))
//    {
//        if (!m_compute.re_compile())
//        {
//            return;
//            //throw std::runtime_error("Failed To Recompile Shader");
//        }
//
//        vkDeviceWaitIdle(renderer.m_context->m_device);
//        for (auto& [name, pipeline] : m_pipelines)
//        {
//            if (pipeline.IsGraphicsPipeline)
//                continue;
//
//            ComputePipelineDesc desc = pipeline.ComputeDesc;
//            pipeline.cleanup();
//
//            pipeline = Pipeline(desc);
//
//            std::cout << "[HotReload] Reloaded Pipeline:" << name << " From " << m_name << "\n";
//        }
//    }
//
//    update_impl(renderer, cmd);
//}
//
//void RenderPass::base_cleanup()
//{
//    for (auto& [name, pipeline] : m_pipelines)
//    {
//        pipeline.cleanup();
//    }
//
//    nijiEngine.m_renderer.m_resourceBank.destroy(m_passBuffer);
//
//
//    m_passDescriptor.cleanup();
//}