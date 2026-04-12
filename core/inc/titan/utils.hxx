#pragma once

#include <titan/api.hxx>
#include <titan/result.hxx>
#include <titan/wrapper/vk.hxx>
#include <titan/wrapper/xr.hxx>

#include <vector>

namespace core::xr
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
    result<std::vector<T>> Enumerate(F fn, const T &value, A... args)
    {
        uint32_t count;
        if (auto res = fn(args..., 0, &count, nullptr))
            return error<std::vector<T>>("xr::Enumerate => {}", res);

        std::vector<T> elements(count, value);
        if (auto res = fn(args..., count, &count, reinterpret_cast<E *>(elements.data())))
            return error<std::vector<T>>("xr::Enumerate => {}", res);

        return elements;
    }

    result<std::vector<XrViewConfigurationType>> EnumerateViewConfigurationTypes(
        XrInstance instance,
        XrSystemId system_id);

    result<std::vector<XrViewConfigurationView>> EnumerateViewConfigurationViews(
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

    result<std::vector<XrEnvironmentBlendMode>> EnumerateEnvironmentBlendModes(
        XrInstance instance,
        XrSystemId system_id,
        XrViewConfigurationType view_configuration_type);

    result<std::vector<XrView>> LocateViews(
        XrSession session,
        const XrViewLocateInfo &view_locate_info,
        XrViewState &view_state);

    result<XrGraphicsRequirementsVulkan2KHR> GetVulkanGraphicsRequirements2KHR(
        XrInstance instance,
        XrSystemId system_id);

    result<vk::Instance> CreateVulkanInstanceKHR(
        XrInstance instance,
        const XrVulkanInstanceCreateInfoKHR &create_info);

    result<vk::Device> CreateVulkanDeviceKHR(
        XrInstance instance,
        const XrVulkanDeviceCreateInfoKHR &create_info);

    result<VkPhysicalDevice> GetVulkanGraphicsDevice2KHR(
        XrInstance instance,
        const XrVulkanGraphicsDeviceGetInfoKHR &get_info);

    result<XrSystemId> GetSystem(XrInstance instance, const XrSystemGetInfo &get_info);

    result<XrFrameState> WaitFrame(XrSession session, const XrFrameWaitInfo &frame_wait_info);

    result<> BeginFrame(XrSession session, const XrFrameBeginInfo &frame_begin_info);
    result<> EndFrame(XrSession session, const XrFrameEndInfo &frame_end_info);

    result<XrPath> StringToPath(XrInstance instance, const std::string &str);
    result<std::string> PathToString(XrInstance instance, XrPath path);

    result<> SuggestInteractionProfileBindings(
        XrInstance instance,
        const std::string &profile,
        const std::vector<std::pair<XrAction, std::string>> &bindings);

    result<XrInteractionProfileState> GetCurrentInteractionProfile(XrSession session, XrPath path);
}

namespace core::vk
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
    result<std::vector<T>> Enumerate(F fn, const T &value, A... args)
    {
        uint32_t count;
        if (auto res = fn(args..., &count, nullptr))
            return error<std::vector<T>>("vk::Enumerate => {}", res);

        std::vector<T> elements(count, value);
        if (auto res = fn(args..., &count, reinterpret_cast<E *>(elements.data())))
            return error<std::vector<T>>("vk::Enumerate => {}", res);

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

    result<std::vector<VkSurfaceFormat2KHR>> GetPhysicalDeviceSurfaceFormats2KHR(
        VkPhysicalDevice physical_device,
        const VkPhysicalDeviceSurfaceInfo2KHR &surface_info);

    result<std::vector<VkPresentModeKHR>> GetPhysicalDeviceSurfacePresentModesKHR(
        VkPhysicalDevice physical_device,
        VkSurfaceKHR surface);

    result<std::vector<VkImage>> GetSwapchainImagesKHR(
        VkDevice device,
        VkSwapchainKHR swapchain);

    std::vector<VkQueueFamilyProperties2> GetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physical_device);

    VkPhysicalDeviceFeatures2 GetPhysicalDeviceFeatures2(VkPhysicalDevice physical_device);

    VkMemoryRequirements2 GetBufferMemoryRequirements2(
        VkDevice device,
        const VkBufferMemoryRequirementsInfo2 &requirements_info);

    result<> BindBufferMemory2(VkDevice device, const VkBindBufferMemoryInfo &bind_info);
    result<> BindBufferMemory2(VkDevice device, const std::vector<VkBindBufferMemoryInfo> &bind_infos);

    result<> BindImageMemory2(VkDevice device, const VkBindImageMemoryInfo &bind_info);
    result<> BindImageMemory2(VkDevice device, const std::vector<VkBindImageMemoryInfo> &bind_infos);

    VkPhysicalDeviceProperties2 GetPhysicalDeviceProperties2(VkPhysicalDevice physical_device);

    result<std::vector<char>> GetPipelineCacheData(VkDevice device, VkPipelineCache pipeline_cache);

    result<> ResetCommandBuffer(VkCommandBuffer command_buffer, VkCommandBufferResetFlags flags);
    result<> BeginCommandBuffer(VkCommandBuffer command_buffer, const VkCommandBufferBeginInfo &begin_info);
    result<> EndCommandBuffer(VkCommandBuffer command_buffer);

    result<void *> MapMemory2(VkDevice device, const VkMemoryMapInfo &map_info);
    result<> UnmapMemory2(VkDevice device, const VkMemoryUnmapInfo &unmap_info);

    result<VkSurfaceCapabilities2KHR> GetPhysicalDeviceSurfaceCapabilities2KHR(
        VkPhysicalDevice physical_device,
        const VkPhysicalDeviceSurfaceInfo2KHR &surface_info);
}
