//#pragma once
//
//namespace niji
//{
//
//
//class Swapchain
//{
//  public:
//    Swapchain();
//
//    void recreate();
//
//  private:
//    void create();
//    void create_image_views();
//
//    void deinit();
//
//    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
//    VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes);
//    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);
//
//  private:
//    friend class Renderer;
//    friend class ForwardPass;
//    friend class ImGuiPass;
//    friend class SkyboxPass;
//    friend class LineRenderPass;
//    friend class DepthPass;
//
//    
//};
//} // namespace niji
