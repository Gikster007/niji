//#pragma once
//
//#include "render_pass.hpp"
//
//namespace niji
//{
//
//class ForwardPass final : public RenderPass
//{
//  public:
//    ForwardPass()
//    {
//    }
//
//    void init(Descriptor& globalDescriptor);
//    void update_impl(Renderer& renderer, CommandList& cmd);
//    void record(Renderer& renderer, CommandList& cmd, RenderInfo& info);
//    void deinit();
//
//    void debug_panel();
//  private:
//    DebugSettings m_debugSettings = {};
//
//    BufferHandle m_pointLightBuffer = {};
//    SamplerHandle m_pointSampler = {};
//};
//
//} // namespace niji