#pragma once

#include "common.hpp"

namespace niji
{
class CommandList
{
  public:
    CommandList();

    void begin_list(const char* debugName = "Unnamed Commandlist") const;

    void begin_rendering(const RenderInfo& info) const;
    void bind_pipeline(const VkPipeline& pipeline) const;
    void bind_viewport(const VkExtent2D& extent) const;
    void bind_scissor(const VkExtent2D& extent) const;
    
    void bind_vertex_buffer(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* buffers,
                         const VkDeviceSize* offsets) const;
    void bind_index_buffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) const;
    void bind_descriptor_sets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
                            uint32_t firstSet, uint32_t descriptorSetCount,
                            const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount = 0,
                            const uint32_t* pDynamicOffsets = nullptr) const;
    void push_descriptor_set(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
                             uint32_t set, uint32_t descriptorWriteCount,
                             const VkWriteDescriptorSet* pDescriptorWrites);

    void draw_indexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                     int32_t vertexOffset,
                     uint32_t firstInstance) const;

    void end_rendering(const RenderInfo& info) const;
    void end_list() const;

    void reset() const;

    void cleanup();

    // Helpers
    void transition_image(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                         VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                         VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                         VkImageAspectFlags aspectMask, uint32_t mipLevels, uint32_t layerCount) const;
    void transition_image(VkImage image, VkFormat format, VkImageLayout oldLayout,
                         VkImageLayout newLayout, TransitionType usage,
                         uint32_t mipLevels = 1, uint32_t layerCount = 1) const;

  private:
    friend class Renderer;
    friend class ImGuiPass;

    VkCommandBuffer m_commandBuffer = {};
};
} // namespace niji