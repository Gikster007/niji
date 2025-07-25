#include "vulkan-functions.hpp"

// Define the actual storage for the function pointers
PFN_vkCmdBeginRenderingKHR VKCmdBeginRenderingKHR = nullptr;
PFN_vkCmdEndRenderingKHR VKCmdEndRenderingKHR = nullptr;
PFN_vkSetDebugUtilsObjectNameEXT VKSetDebugUtilsObjectNameEXT = nullptr;
PFN_vkCmdPipelineBarrier2KHR VKCmdPipelineBarrier2KHR = nullptr;
PFN_vkCmdPushDescriptorSetKHR VKCmdPushDescriptorSetKHR = nullptr;
PFN_vkCmdBeginDebugUtilsLabelEXT VKCmdBeginDebugUtilsLabelEXT = nullptr;
PFN_vkCmdEndDebugUtilsLabelEXT VKCmdEndDebugUtilsLabelEXT = nullptr;

void niji::load_vulkan_function_pointers(VkDevice device)
{
    VKCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(
        vkGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR"));
    VKCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(
        vkGetDeviceProcAddr(device, "vkCmdEndRenderingKHR"));
    VKSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
        vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
    VKCmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(
        vkGetDeviceProcAddr(device, "vkCmdPipelineBarrier2KHR"));
    VKCmdPushDescriptorSetKHR = reinterpret_cast<PFN_vkCmdPushDescriptorSetKHR>(
        vkGetDeviceProcAddr(device, "vkCmdPushDescriptorSetKHR"));
    VKCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
        vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT"));
    VKCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
        vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT"));
}
