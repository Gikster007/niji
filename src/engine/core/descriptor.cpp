#include "descriptor.hpp"

#include <stdexcept>

#include "engine.hpp"

using namespace niji;

Descriptor::Descriptor(DescriptorInfo& info) : m_info(info)
{
    // 1. Create descriptor set layout
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings = {};
        for (int i = 0; i < info.Bindings.size(); i++)
        {
            const auto& binding = info.Bindings[i];
            VkDescriptorSetLayoutBinding b = {};
            b.binding = i;
            switch (binding.Type)
            {
            case DescriptorBinding::BindType::NONE:
                throw std::runtime_error("Invalid Descriptor Binding Type!");
                break;
            case DescriptorBinding::BindType::UBO:
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;
            case DescriptorBinding::BindType::SAMPLER:
                b.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                break;
            case DescriptorBinding::BindType::TEXTURE:
                b.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                break;
            case DescriptorBinding::BindType::STORAGE_BUFFER:
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                break;
            default:
                throw std::runtime_error("Invalid Descriptor Binding Type!");
                break;
            }
            switch (binding.Stage)
            {
            case DescriptorBinding::BindStage::NONE:
                throw std::runtime_error("Invalid Descriptor Binding Stage!");
                break;
            case DescriptorBinding::BindStage::VERTEX_SHADER:
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case DescriptorBinding::BindStage::FRAGMENT_SHADER:
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case DescriptorBinding::BindStage::ALL_GRAPHICS:
                b.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
                break;
            default:
                throw std::runtime_error("Invalid Descriptor Binding Stage!");
                break;
            }
            b.descriptorCount = binding.Count;
            b.pImmutableSamplers = binding.Sampler;
            bindings.push_back(b);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        if (info.IsPushDescriptor)
            layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        layoutInfo.bindingCount = static_cast<uint32_t>(info.Bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(nijiEngine.m_context.m_device, &layoutInfo, nullptr,
                                        &m_setLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor set layout.");
        }
    }

    if (!info.IsPushDescriptor)
    {
        // 2. Create Descriptor Pool
        VkDescriptorPoolSize poolSize = {};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

        if (vkCreateDescriptorPool(nijiEngine.m_context.m_device, &poolInfo, nullptr, &m_pool) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor pool.");
        }

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            // 3. Allocate descriptor set
            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = m_pool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &m_setLayout;

            if (vkAllocateDescriptorSets(nijiEngine.m_context.m_device, &allocInfo, &m_set[i]) !=
                VK_SUCCESS)
            {
                throw std::runtime_error("Failed to allocate descriptor set.");
            }

            // 4. Write to descriptor set
            std::vector<VkWriteDescriptorSet> writes = {};
            VkDescriptorBufferInfo bufferInfo = {};
            for (int j = 0; j < info.Bindings.size(); j++)
            {
                const auto& binding = info.Bindings[j];
                switch (binding.Type)
                {
                case DescriptorBinding::BindType::NONE:
                    throw std::runtime_error("Invalid Descriptor Binding Type!");
                    break;
                case DescriptorBinding::BindType::UBO:
                    std::visit(
                        [&](auto&& res) {
                            using T = std::decay_t<decltype(res)>;
                            if constexpr (std::is_same_v<T, std::vector<Buffer>*>)
                            {
                                std::vector<Buffer>& arr = *res;
                                for (const auto& buffer : arr)
                                {
                                    bufferInfo.buffer = buffer.Handle;
                                    bufferInfo.offset = 0;
                                    bufferInfo.range = buffer.Desc.Size;

                                    VkWriteDescriptorSet write = {};
                                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                    write.dstSet = m_set[i];
                                    write.dstBinding = j;
                                    write.dstArrayElement = 0;
                                    write.descriptorCount = binding.Count;
                                    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                    write.pBufferInfo = &bufferInfo;
                                    writes.push_back(write);
                                }
                            }
                        },
                        binding.Resource);
                    break;
                case DescriptorBinding::BindType::SAMPLER:
                    std::visit(
                        [&](auto&& res) {
                            using T = std::decay_t<decltype(res)>;
                            if constexpr (std::is_same_v<T, Sampler*>)
                            {
                                Sampler& sampler = *res;

                                VkDescriptorImageInfo samplerInfo = {};
                                samplerInfo.sampler = sampler.Handle;
                                samplerInfo.imageView = VK_NULL_HANDLE;
                                samplerInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

                                VkWriteDescriptorSet write = {};
                                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                write.pNext = nullptr;
                                write.dstSet = m_set[i];
                                write.dstBinding = j;
                                write.dstArrayElement = 0;
                                write.descriptorCount = binding.Count;
                                write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                                write.pImageInfo = &samplerInfo;
                                write.pBufferInfo = nullptr;
                                write.pTexelBufferView = nullptr;
                                writes.push_back(write);
                            }
                        },
                        binding.Resource);

                    break;
                case DescriptorBinding::BindType::TEXTURE:
                    std::visit(
                        [&](auto&& res) {
                            using T = std::decay_t<decltype(res)>;
                            if constexpr (std::is_same_v<T, Texture*>)
                            {
                                Texture& texture = *res;

                                VkWriteDescriptorSet write = {};
                                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                write.pNext = nullptr;
                                write.dstSet = m_set[i];
                                write.dstBinding = j;
                                write.dstArrayElement = 0;
                                write.descriptorCount = binding.Count;
                                write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                                write.pImageInfo = &texture.ImageInfo;
                                write.pBufferInfo = nullptr;
                                write.pTexelBufferView = nullptr;
                                writes.push_back(write);
                            }
                        },
                        binding.Resource);
                    break;
                case DescriptorBinding::BindType::STORAGE_BUFFER:
                    std::visit(
                        [&](auto&& res) {
                            using T = std::decay_t<decltype(res)>;
                            if constexpr (std::is_same_v<T, std::vector<Buffer>*>)
                            {
                                std::vector<Buffer>& arr = *res;
                                for (const auto& buffer : arr)
                                {
                                    bufferInfo.buffer = buffer.Handle;
                                    bufferInfo.offset = 0;
                                    bufferInfo.range = buffer.Desc.Size;

                                    VkWriteDescriptorSet write = {};
                                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                    write.dstSet = m_set[i];
                                    write.dstBinding = j;
                                    write.dstArrayElement = 0;
                                    write.descriptorCount = binding.Count;
                                    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                                    write.pBufferInfo = &bufferInfo;
                                    writes.push_back(write);
                                }
                            }
                        },
                        binding.Resource);
                    break;
                default:
                    throw std::runtime_error("Invalid Descriptor Binding Type!");
                    break;
                }
            }

            vkUpdateDescriptorSets(nijiEngine.m_context.m_device,
                                   static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        }
    }
}

void Descriptor::push_descriptor_writes(std::vector<VkWriteDescriptorSet>& writes,
                                        std::vector<VkDescriptorBufferInfo>& bufferInfos,
                                        std::vector<VkDescriptorImageInfo>& imageInfos)
{

    for (int i = 0; i < m_info.Bindings.size(); ++i)
    {
        const auto& binding = m_info.Bindings[i];
        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstBinding = i;
        write.dstArrayElement = 0;
        write.descriptorCount = binding.Count;
        write.dstSet = VK_NULL_HANDLE;

        switch (binding.Type)
        {
        case DescriptorBinding::BindType::UBO: {
            auto* buffer = std::get<Buffer*>(binding.Resource);

            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = buffer ? buffer->Handle : VK_NULL_HANDLE;
            bufferInfo.offset = 0;
            bufferInfo.range = buffer ? buffer->Desc.Size : 0;

            bufferInfos.push_back(bufferInfo);

            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.pBufferInfo = &bufferInfos.back();
            break;
        }
        case DescriptorBinding::BindType::SAMPLER: {
            auto* sampler = std::get<Sampler*>(binding.Resource);

            VkDescriptorImageInfo samplerInfo = {};
            samplerInfo.sampler = sampler ? sampler->Handle : VK_NULL_HANDLE;
            samplerInfo.imageView = VK_NULL_HANDLE;
            samplerInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            imageInfos.push_back(samplerInfo);

            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            write.pImageInfo = &imageInfos.back();
            break;
        }
        case DescriptorBinding::BindType::TEXTURE: {
            auto* texture = std::get<Texture*>(binding.Resource);

            VkDescriptorImageInfo textureInfo = {};
            textureInfo.sampler = VK_NULL_HANDLE;
            textureInfo.imageView = texture ? texture->ImageInfo.imageView : VK_NULL_HANDLE;
            textureInfo.imageLayout =
                texture ? texture->ImageInfo.imageLayout : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            imageInfos.push_back(textureInfo);

            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageInfo = &imageInfos.back();
            break;
        }
        case DescriptorBinding::BindType::STORAGE_BUFFER: {
            auto* buffer = std::get<Buffer*>(binding.Resource);

            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = buffer ? buffer->Handle : VK_NULL_HANDLE;
            bufferInfo.offset = 0;
            bufferInfo.range = buffer ? buffer->Desc.Size : 0;

            bufferInfos.push_back(bufferInfo);

            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.pBufferInfo = &bufferInfos.back();
            break;
        }
        default:
            throw std::runtime_error("Invalid Descriptor Binding Type!");
            break;
        }

        writes.push_back(write);
    }
}

void Descriptor::cleanup() const
{
    vkDestroyDescriptorSetLayout(nijiEngine.m_context.m_device, m_setLayout, nullptr);
    vkDestroyDescriptorPool(nijiEngine.m_context.m_device, m_pool, nullptr);
}
