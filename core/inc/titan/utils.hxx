#pragma once

#include <titan/api.hxx>
#include <titan/result.hxx>
#include <titan/wrapper/vk.hxx>
#include <titan/wrapper/xr.hxx>

#include <vector>

namespace titan::xr
{
    template<typename P>
    P GetInstanceProcAddr(XrInstance instance, const char *name)
    {
        PFN_xrVoidFunction pfn;

        if (xrGetInstanceProcAddr(instance, name, &pfn))
            return nullptr;

        return reinterpret_cast<P>(pfn);
    }

    template<typename T, typename E = T, typename F, typename... A>
        requires std::is_same_v<XrResult(*)(A..., uint32_t, uint32_t *, E *), F>
    toolkit::result<std::vector<T>> Enumerate(F fn, const T &value, A... args)
    {
        uint32_t count;
        if (auto res = fn(args..., 0, &count, nullptr))
            return toolkit::make_error("xr::Enumerate => {}", res);

        std::vector<T> elements(count, value);
        if (auto res = fn(args..., count, &count, reinterpret_cast<E *>(elements.data())))
            return toolkit::make_error("xr::Enumerate => {}", res);

        return elements;
    }

    toolkit::result<std::vector<XrViewConfigurationType>> EnumerateViewConfigurationTypes(
        XrInstance instance,
        XrSystemId system_id);

    toolkit::result<std::vector<XrViewConfigurationView>> EnumerateViewConfigurationViews(
        XrInstance instance,
        XrSystemId system_id,
        XrViewConfigurationType view_configuration_type);

    template<typename T = int64_t>
    auto EnumerateSwapchainFormats(
        XrSession session,
        const T &value = {})
    {
        return Enumerate<T, int64_t>(
            xrEnumerateSwapchainFormats,
            value,
            session);
    }

    template<typename T>
    auto EnumerateSwapchainImages(
        XrSwapchain swapchain,
        const T &value)
    {
        return Enumerate<T, XrSwapchainImageBaseHeader>(
            xrEnumerateSwapchainImages,
            value,
            swapchain);
    }

    toolkit::result<std::vector<XrEnvironmentBlendMode>> EnumerateEnvironmentBlendModes(
        XrInstance instance,
        XrSystemId system_id,
        XrViewConfigurationType view_configuration_type);

    toolkit::result<std::vector<XrView>> LocateViews(
        XrSession session,
        const XrViewLocateInfo &view_locate_info,
        XrViewState &view_state);

    toolkit::result<XrGraphicsRequirementsVulkan2KHR> GetVulkanGraphicsRequirements2KHR(
        XrInstance instance,
        XrSystemId system_id);

    toolkit::result<vk::Instance> CreateVulkanInstanceKHR(
        XrInstance instance,
        const XrVulkanInstanceCreateInfoKHR &create_info);

    toolkit::result<vk::Device> CreateVulkanDeviceKHR(
        XrInstance instance,
        const XrVulkanDeviceCreateInfoKHR &create_info);

    toolkit::result<VkPhysicalDevice> GetVulkanGraphicsDevice2KHR(
        XrInstance instance,
        const XrVulkanGraphicsDeviceGetInfoKHR &get_info);

    toolkit::result<XrSystemId> GetSystem(XrInstance instance, const XrSystemGetInfo &get_info);

    toolkit::result<XrFrameState> WaitFrame(XrSession session, const XrFrameWaitInfo &frame_wait_info);

    toolkit::result<> BeginFrame(XrSession session, const XrFrameBeginInfo &frame_begin_info);
    toolkit::result<> EndFrame(XrSession session, const XrFrameEndInfo &frame_end_info);

    toolkit::result<XrPath> StringToPath(XrInstance instance, const std::string &str);
    toolkit::result<std::string> PathToString(XrInstance instance, XrPath path);

    toolkit::result<> SuggestInteractionProfileBindings(
        XrInstance instance,
        const std::string &profile,
        const std::vector<std::pair<XrAction, std::string>> &bindings);

    toolkit::result<XrInteractionProfileState> GetCurrentInteractionProfile(XrSession session, XrPath path);
}

namespace titan::vk
{
    template<typename P>
    P GetInstanceProcAddr(VkInstance instance, const char *name)
    {
        auto pfn = vkGetInstanceProcAddr(instance, name);

        if (!pfn)
            return nullptr;

        return reinterpret_cast<P>(pfn);
    }

    template<typename P>
    P GetDeviceProcAddr(VkDevice device, const char *name)
    {
        auto pfn = vkGetDeviceProcAddr(device, name);

        if (!pfn)
            return nullptr;

        return reinterpret_cast<P>(pfn);
    }

    template<typename T, typename E = T, typename F, typename... A>
        requires std::is_same_v<VkResult (*)(A..., uint32_t *, E *), F>
    toolkit::result<std::vector<T>> Enumerate(F fn, const T &value, A... args)
    {
        uint32_t count;
        if (auto res = fn(args..., &count, nullptr))
            return toolkit::make_error("vk::Enumerate => {}", res);

        std::vector<T> elements(count, value);
        if (auto res = fn(args..., &count, reinterpret_cast<E *>(elements.data())))
            return toolkit::make_error("vk::Enumerate => {}", res);

        return elements;
    }

    template<typename T, typename E = T, typename F, typename... A>
        requires std::is_same_v<void (*)(A..., uint32_t *, E *), F>
    auto Get(F fn, const T &value, A... args)
    {
        uint32_t count;
        fn(args..., &count, nullptr);

        std::vector<T> elements(count, value);
        fn(args..., &count, reinterpret_cast<E *>(elements.data()));

        return elements;
    }

    toolkit::result<std::vector<VkSurfaceFormat2KHR>> GetPhysicalDeviceSurfaceFormats2KHR(
        VkPhysicalDevice physical_device,
        const VkPhysicalDeviceSurfaceInfo2KHR &surface_info);

    toolkit::result<std::vector<VkPresentModeKHR>> GetPhysicalDeviceSurfacePresentModesKHR(
        VkPhysicalDevice physical_device,
        VkSurfaceKHR surface);

    toolkit::result<std::vector<VkImage>> GetSwapchainImagesKHR(
        VkDevice device,
        VkSwapchainKHR swapchain);

    std::vector<VkQueueFamilyProperties2> GetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physical_device);

    VkPhysicalDeviceFeatures2 GetPhysicalDeviceFeatures2(VkPhysicalDevice physical_device);

    VkMemoryRequirements2 GetBufferMemoryRequirements2(
        VkDevice device,
        const VkBufferMemoryRequirementsInfo2 &requirements_info);

    toolkit::result<> BindBufferMemory2(VkDevice device, const VkBindBufferMemoryInfo &bind_info);
    toolkit::result<> BindBufferMemory2(VkDevice device, const std::vector<VkBindBufferMemoryInfo> &bind_infos);

    toolkit::result<> BindImageMemory2(VkDevice device, const VkBindImageMemoryInfo &bind_info);
    toolkit::result<> BindImageMemory2(VkDevice device, const std::vector<VkBindImageMemoryInfo> &bind_infos);

    VkPhysicalDeviceProperties2 GetPhysicalDeviceProperties2(VkPhysicalDevice physical_device);

    toolkit::result<std::vector<char>> GetPipelineCacheData(VkDevice device, VkPipelineCache pipeline_cache);

    toolkit::result<> ResetCommandBuffer(VkCommandBuffer command_buffer, VkCommandBufferResetFlags flags);
    toolkit::result<> BeginCommandBuffer(VkCommandBuffer command_buffer, const VkCommandBufferBeginInfo &begin_info);
    toolkit::result<> EndCommandBuffer(VkCommandBuffer command_buffer);

    toolkit::result<void *> MapMemory2(VkDevice device, const VkMemoryMapInfo &map_info);
    toolkit::result<> UnmapMemory2(VkDevice device, const VkMemoryUnmapInfo &unmap_info);

    toolkit::result<VkSurfaceCapabilities2KHR> GetPhysicalDeviceSurfaceCapabilities2KHR(
        VkPhysicalDevice physical_device,
        const VkPhysicalDeviceSurfaceInfo2KHR &surface_info);
}
