#pragma once

extern PFN_vkCmdBeginRenderingKHR VKCmdBeginRenderingKHR;
extern PFN_vkCmdEndRenderingKHR VKCmdEndRenderingKHR;
extern PFN_vkSetDebugUtilsObjectNameEXT VKSetDebugUtilsObjectNameEXT;
extern PFN_vkCmdPipelineBarrier2KHR VKCmdPipelineBarrier2KHR;

namespace niji
{
void LoadVulkanFunctionPointers(VkDevice device);
}
