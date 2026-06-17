#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace niji
{

class ComputeNode;

struct Pipeline
{
    VkDescriptorSetLayout Descriptors {};
    VkPipelineLayout Layout {};
    VkPipeline Object {};

    std::string Name = "Unknown Pipeline";
};

class PipelineCache
{
  public:
    PipelineCache() = default;

    // Clear the Pipeline Cache
    void clear();

    // Fetch the Compute Pipeline from Cache if Existent. Otherwise, Create one and Return it
    Pipeline get_pipeline(const std::string_view path, const ComputeNode& node);
    
    //Pipeline get_pipeline(const std::string_view path, const RasterNode& node);

  private:
    // Key: Pass Name | Value: Pipeline
    std::unordered_map<std::string, Pipeline> m_cache {};
    
};

} // namespace niji