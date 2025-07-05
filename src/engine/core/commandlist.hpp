#include "common.hpp"

namespace niji
{
class CommandList
{
  public:
    CommandList();

    void BeginList(const char* debugName = "Unnamed Commandlist") const;

    void BeginRendering(const RenderInfo& info) const;
    void BindPipeline(const VkPipeline& pipeline) const;
    void BindViewport(const VkViewport& viewport) const;
    void BindScissor(const VkRect2D& scissor) const;
    
    void BindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* buffers,
                         const VkDeviceSize* offsets) const;
    void BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) const;
    void BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
                            uint32_t firstSet, uint32_t descriptorSetCount,
                            const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount = 0,
                            const uint32_t* pDynamicOffsets = nullptr) const;
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                     int32_t vertexOffset,
                     uint32_t firstInstance) const;

    void EndRendering(const RenderInfo& info) const;
    void EndList() const;

    void Reset() const;

    void Cleanup();

    // Helpers
    void TransitionImage(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                         VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                         VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                         VkImageAspectFlags aspectMask, uint32_t mipLevels, uint32_t layerCount) const;
    void TransitionImage(VkImage image, VkFormat format, VkImageLayout oldLayout,
                         VkImageLayout newLayout, TransitionType usage,
                         uint32_t mipLevels = 1, uint32_t layerCount = 1) const;

  private:
    friend class Renderer;

    VkCommandBuffer m_commandBuffer = {};
};
} // namespace niji