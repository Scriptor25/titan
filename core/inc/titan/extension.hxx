#pragma once

#include <titan/api.hxx>
#include <titan/result.hxx>
#include <titan/typename.hxx>

#include <vector>

namespace core
{
    namespace xr
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
                return error<std::vector<T>>("xr::Enumerate<{}> => {}", typename_string<T>(), res);

            std::vector<T> elements(count, value);
            if (auto res = fn(args..., count, &count, reinterpret_cast<E *>(elements.data())))
                return error<std::vector<T>>("xr::Enumerate<{}> => {}", typename_string<T>(), res);

            return elements;
        }

        template<typename T = XrViewConfigurationType>
        auto EnumerateViewConfigurationTypes(
            XrInstance instance,
            XrSystemId system_id,
            const T &value = {})
        {
            return Enumerate<T, XrViewConfigurationType>(
                xrEnumerateViewConfigurations,
                value,
                instance,
                system_id);
        }

        template<typename T = XrViewConfigurationView>
        auto EnumerateViewConfigurationViews(
            XrInstance instance,
            XrSystemId system_id,
            XrViewConfigurationType view_configuration_type,
            const T &value = { .type = XR_TYPE_VIEW_CONFIGURATION_VIEW })
        {
            return Enumerate<T, XrViewConfigurationView>(
                xrEnumerateViewConfigurationViews,
                value,
                instance,
                system_id,
                view_configuration_type);
        }

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

        template<typename T = XrSwapchainImageBaseHeader>
        auto EnumerateSwapchainImages(
            XrSwapchain swapchain,
            const T &value = {})
        {
            return Enumerate<T, XrSwapchainImageBaseHeader>(
                xrEnumerateSwapchainImages,
                value,
                swapchain);
        }

        template<typename T = XrEnvironmentBlendMode>
        auto EnumerateEnvironmentBlendModes(
            XrInstance instance,
            XrSystemId system_id,
            XrViewConfigurationType view_configuration_type,
            const T &value = {})
        {
            return Enumerate<T, XrEnvironmentBlendMode>(
                xrEnumerateEnvironmentBlendModes,
                value,
                instance,
                system_id,
                view_configuration_type);
        }
    }

    namespace vk
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
                return error<std::vector<T>>("vk::Enumerate<{}> => {}", typename_string<T>(), res);

            std::vector<T> elements(count, value);
            if (auto res = fn(args..., &count, reinterpret_cast<E *>(elements.data())))
                return error<std::vector<T>>("vk::Enumerate<{}> => {}", typename_string<T>(), res);

            return elements;
        }

        template<typename T = VkSurfaceFormat2KHR>
        auto GetPhysicalDeviceSurfaceFormats2KHR(
            VkPhysicalDevice physical_device,
            const VkPhysicalDeviceSurfaceInfo2KHR &surface_info,
            const T &value = { .sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR })
        {
            return Enumerate<T, VkSurfaceFormat2KHR>(
                vkGetPhysicalDeviceSurfaceFormats2KHR,
                value,
                physical_device,
                &surface_info);
        }

        template<typename T = VkPresentModeKHR>
        auto GetPhysicalDeviceSurfacePresentModesKHR(
            VkPhysicalDevice physical_device,
            VkSurfaceKHR surface,
            const T &value = {})
        {
            return Enumerate<T, VkPresentModeKHR>(
                vkGetPhysicalDeviceSurfacePresentModesKHR,
                value,
                physical_device,
                surface);
        }

        template<typename T = VkImage>
        auto GetSwapchainImagesKHR(
            VkDevice device,
            VkSwapchainKHR swapchain,
            const T &value = {})
        {
            return Enumerate<T, VkImage>(
                vkGetSwapchainImagesKHR,
                value,
                device,
                swapchain);
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

        template<typename T = VkQueueFamilyProperties2>
        auto GetPhysicalDeviceQueueFamilyProperties(
            VkPhysicalDevice physical_device,
            const T &value = { .sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 })
        {
            return Get<T, VkQueueFamilyProperties2>(
                vkGetPhysicalDeviceQueueFamilyProperties2,
                value,
                physical_device);
        }
    }
}

XRAPI_ATTR XrResult XRAPI_CALL xrCreateDebugUtilsMessengerEXT(
    XrInstance instance,
    const XrDebugUtilsMessengerCreateInfoEXT *createInfo,
    XrDebugUtilsMessengerEXT *messenger);

XRAPI_ATTR XrResult XRAPI_CALL xrDestroyDebugUtilsMessengerEXT(
    XrInstance instance,
    XrDebugUtilsMessengerEXT messenger);

XRAPI_ATTR XrResult XRAPI_CALL xrCreateVulkanInstanceKHR(
    XrInstance instance,
    const XrVulkanInstanceCreateInfoKHR *createInfo,
    VkInstance *vulkanInstance,
    VkResult *vulkanResult);

XRAPI_ATTR XrResult XRAPI_CALL xrCreateVulkanDeviceKHR(
    XrInstance instance,
    const XrVulkanDeviceCreateInfoKHR *createInfo,
    VkDevice *vulkanDevice,
    VkResult *vulkanResult);

XRAPI_ATTR XrResult XRAPI_CALL xrGetVulkanGraphicsDevice2KHR(
    XrInstance instance,
    const XrVulkanGraphicsDeviceGetInfoKHR *getInfo,
    VkPhysicalDevice *vulkanPhysicalDevice);

XRAPI_ATTR XrResult XRAPI_CALL xrGetVulkanGraphicsRequirements2KHR(
    XrInstance instance,
    XrSystemId systemId,
    XrGraphicsRequirementsVulkanKHR *graphicsRequirements);
