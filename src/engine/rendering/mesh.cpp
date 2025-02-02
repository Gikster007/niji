#include "mesh.hpp"

#include "engine.hpp"
#include "core/context.hpp"

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/util.hpp>

using namespace niji;

VkCommandBuffer Mesh::begin_single_time_commands()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = nijiEngine.m_context.m_commandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer = {};
    vkAllocateCommandBuffers(nijiEngine.m_context.m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void Mesh::end_single_time_commands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(nijiEngine.m_context.m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(nijiEngine.m_context.m_graphicsQueue);

    vkFreeCommandBuffers(nijiEngine.m_context.m_device, nijiEngine.m_context.m_commandPool, 1,
                         &commandBuffer);
}

void Mesh::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
                         VkBuffer& buffer, VmaAllocation& allocation, bool persistent)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    if (persistent)
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if (vmaCreateBuffer(nijiEngine.m_context.m_allocator, &bufferInfo, &allocInfo, &buffer,
                        &allocation, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Failed to Create Buffer with VMA!");
}

void Mesh::copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = begin_single_time_commands();

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    end_single_time_commands(commandBuffer);
}

Mesh::Mesh(fastgltf::Asset& model, fastgltf::Primitive& primitive)
{
    std::vector<uint32_t> uintIndices = {};
    std::vector<uint16_t> ushortIndices = {};
    std::vector<Vertex> vertices = {};

    // Load Indices
    {
        fastgltf::Accessor& indexAccessor = model.accessors[primitive.indicesAccessor.value()];

        switch (indexAccessor.componentType)
        {
        case fastgltf::ComponentType::UnsignedShort:
            m_ushortIndices = true;
            ushortIndices.reserve(ushortIndices.size() + indexAccessor.count);

            fastgltf::iterateAccessor<std::uint16_t>(model, indexAccessor, [&](std::uint16_t idx) {
                ushortIndices.push_back(idx);
            });

            m_indexCount = ushortIndices.size();
            break;

        case fastgltf::ComponentType::UnsignedInt:
            uintIndices.reserve(uintIndices.size() + indexAccessor.count);

            fastgltf::iterateAccessor<std::uint32_t>(model, indexAccessor, [&](std::uint32_t idx) {
                uintIndices.push_back(idx);
            });

            m_indexCount = uintIndices.size();
            break;

        default:
            break;
        }

        
    }

    // Load Vertices
    {
        fastgltf::Accessor& posAccessor =
            model.accessors[primitive.findAttribute("POSITION")->accessorIndex];
        vertices.resize(vertices.size() + posAccessor.count);
        fastgltf::iterateAccessorWithIndex<glm::vec3>(model, posAccessor,
                                                      [&](glm::vec3 v, size_t index) {
                                                          Vertex newVertex;
                                                          newVertex.Pos = v;
                                                          newVertex.Color = glm::vec4{1.0f};
                                                          newVertex.TexCoord =
                                                              glm::vec2(0.0f, 0.0f);
                                                          vertices[index] = newVertex;
                                                      });
    }

    // Load UVs
    {
        auto uv = primitive.findAttribute("TEXCOORD_0");
        if (uv != primitive.attributes.end())
        {

            fastgltf::iterateAccessorWithIndex<glm::vec2>(model,
                                                          model.accessors[(*uv).accessorIndex],
                                                          [&](glm::vec2 v, size_t index) {
                                                              vertices[index].TexCoord = v;
                                                          });
        }
    }

    // Load Vertex Colors
    {
        auto colors = primitive.findAttribute("COLOR_0");
        if (colors != primitive.attributes.end())
        {

            fastgltf::iterateAccessorWithIndex<glm::vec4>(model,
                                                          model.accessors[(*colors).accessorIndex],
                                                          [&](glm::vec4 v, size_t index) {
                                                              vertices[index].Color =
                                                                  glm::vec3(v.x, v.y, v.z);
                                                          });
        }
    }

    // Create Vertex Buffer
    {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer = {};
        VmaAllocation stagingBufferAllocation = {};
        create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                      stagingBuffer, stagingBufferAllocation);

        void* data = nullptr;
        vmaMapMemory(nijiEngine.m_context.m_allocator, stagingBufferAllocation, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vmaUnmapMemory(nijiEngine.m_context.m_allocator, stagingBufferAllocation);

        create_buffer(bufferSize,
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VMA_MEMORY_USAGE_GPU_ONLY, m_vertexBuffer.Buffer,
                      m_vertexBuffer.BufferAllocation);

        copy_buffer(stagingBuffer, m_vertexBuffer.Buffer, bufferSize);

        vmaDestroyBuffer(nijiEngine.m_context.m_allocator, stagingBuffer, stagingBufferAllocation);
    }

    // Create Index Buffer
    {
        VkDeviceSize bufferSize = {};
        VkBuffer stagingBuffer = {};
        VmaAllocation stagingBufferAllocation = {};

        if (m_ushortIndices) // Unsigned Short
        {
            bufferSize = sizeof(ushortIndices[0]) * ushortIndices.size();

            create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                          stagingBuffer, stagingBufferAllocation);

            void* data = nullptr;
            vmaMapMemory(nijiEngine.m_context.m_allocator, stagingBufferAllocation, &data);
            memcpy(data, ushortIndices.data(), (size_t)bufferSize);
            vmaUnmapMemory(nijiEngine.m_context.m_allocator, stagingBufferAllocation);
        }
        else // Unsigned Int
        {
            bufferSize = sizeof(uintIndices[0]) * uintIndices.size();

            create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                          stagingBuffer, stagingBufferAllocation);

            void* data = nullptr;
            vmaMapMemory(nijiEngine.m_context.m_allocator, stagingBufferAllocation, &data);
            memcpy(data, uintIndices.data(), (size_t)bufferSize);
            vmaUnmapMemory(nijiEngine.m_context.m_allocator, stagingBufferAllocation);
        }

        create_buffer(bufferSize,
                      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VMA_MEMORY_USAGE_GPU_ONLY, m_indexBuffer.Buffer,
                      m_indexBuffer.BufferAllocation);

        copy_buffer(stagingBuffer, m_indexBuffer.Buffer, bufferSize);

        vmaDestroyBuffer(nijiEngine.m_context.m_allocator, stagingBuffer, stagingBufferAllocation);
    }
}
